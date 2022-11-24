from ctypes import *
from math import log2
from random import randint
import time
import numpy as np
import os
import warnings

from .utils import *

clib_path = os.path.join(os.path.split(__file__)[0], "cpp/libre.so")
clib = CDLL(clib_path)


class c_BitStream(Structure):
    _fields_ = ("length", c_int), ("s", POINTER(c_char))


class c_ListCDFTable(Structure):
    _fields_ = (
        ("_precision", c_int),
        ("_n", c_int),
        ("_cdf", POINTER(c_int)),
        ("_maxv", POINTER(c_int)),
        ("_minv", POINTER(c_int)),
        ("_max_cdf_length", c_int),
    )


class c_GaussianCDFTable(Structure):
    _fields_ = (
        ("_precision", c_int),
        ("_n", c_int),
        ("_mu", POINTER(c_double)),
        ("_sigma", POINTER(c_double)),
    )


class CDFTable:
    def get_ctype(self):
        raise NotImplementedError

    def encode(self, latent: np.ndarray):
        """
        Input shape: [num_channels, num_elements_per_channel]
        """
        latent = self.checkData(latent)
        self.f_encode.restype = c_BitStream
        dim0, dim1 = latent.shape
        if dim0 != self.num_channels:
            raise ValueError(f"Dimensions mismatch in dim0! Got {dim0} vs. self.num_channels={self.num_channels}")
        latent = latent.reshape((-1,))
        c_latent = np.ctypeslib.as_ctypes(latent)
        t = time.time()
        data = self.f_encode(c_latent, dim0, dim1, pointer(self.get_ctype()))
        print("encode_time =", time.time() - t)
        array_type = c_char * data.length
        data_carray = array_type.from_address(addressof(data.s.contents))
        data_bytearray = bytearray(data_carray)
        return data_bytearray

    def decode(self, data_bytearray: bytearray, dim1):
        self.f_decode.restype = POINTER(c_int)
        l = len(data_bytearray)
        array_type = c_char * l
        data_carray = array_type(*data_bytearray)
        data = c_BitStream(l, data_carray)
        t = time.time()
        c_latent_recon = self.f_decode(data, self.num_channels, dim1, pointer(self.get_ctype()))
        print("decode_time =", time.time() - t)
        recon = np.ctypeslib.as_array(c_latent_recon, (self.num_channels * dim1,))
        recon = recon.reshape((self.num_channels, dim1))
        return recon
    
    def checkData(self, inputs):
        if inputs.dtype not in (np.int8, np.int16, np.int32):
            raise ValueError(f"Unsupported data type: {inputs.dtype}, must be an integer type.")
        
        return inputs
        


class ListCDFTable(CDFTable):
    """
    Data structure to maintain a listed entropy bottleneck
    Input shapes:
    - cdf: [num_channels, ?]
    - minv, maxv: [num_channels]
    
    Format of a CDF table
     - _cdf[c, x] = P(X<x+minv) * 2^precision in channel c

    i.e.
    _cdf[0] = P(x<minv) * 2^precision = 0
    _cdf[1] = P(x<minv+1) * 2^precision
    ...
    _cdf[len-2] = P(x<maxv) * 2^precision
    _cdf[len-1] = P(x<maxv+1) * 2^precision = 2 ^ precision

    the length of _cdf equals to maxv - minv + 2
 
    """

    def __init__(
        self, cdf: np.array, maxv: np.array, minv: np.array, precision: int = 16
    ):
        checkDimension(cdf, 2)
        checkDimension(minv, 1)
        checkDimension(maxv, 1)
        checkDimensionMatch(cdf, minv, 0)
        checkDimensionMatch(cdf, maxv, 0)

        cdf = cdf.astype(np.int32)
        maxv = maxv.astype(np.int32)
        minv = minv.astype(np.int32)

        self.precision = precision
        self.num_channels = cdf.shape[0]
        self.max_cdf_length = cdf.shape[1]
        cdf = np.reshape(cdf, [-1])
        self.cdf = np.ctypeslib.as_ctypes(cdf)
        self.maxv = maxv
        self.minv = minv
        self.maxv_c = np.ctypeslib.as_ctypes(maxv)
        self.minv_c = np.ctypeslib.as_ctypes(minv)

        self.f_encode = clib._Z26encode_single_channel_listPiiiPK15ListCDFTableRaw
        self.f_decode = (
            clib._Z26decode_single_channel_list9BitStreamiiPK15ListCDFTableRaw
        )

    def get_ctype(self):
        return c_ListCDFTable(
            self.precision,
            self.num_channels,
            self.cdf,
            self.maxv_c,
            self.minv_c,
            self.max_cdf_length,
        )
    
    def checkData(self, inputs):
        inputs = super().checkData(inputs)
        inputs = inputs.transpose()
        less = np.less(inputs, self.minv)
        greater = np.greater(inputs, self.maxv)
        num_modifies = np.sum(less) + np.sum(greater)
        if num_modifies > 0:
            num_elements = inputs.shape[0] * inputs.shape[1]
            warnings.warn(f"Values out of range detected. {num_modifies} out of {num_elements} values have been clipped.", RuntimeWarning)
            inputs = np.where(less, self.minv, inputs)
            inputs = np.where(greater, self.maxv, inputs)
        inputs = inputs.transpose()
        return inputs



class GaussianCDFTable(CDFTable):
    """
    Data structure to maintain a gaussian entropy bottleneck
    """

    def __init__(self, mu, sigma, precision=16):
        checkDimension(mu, 1)
        checkDimension(sigma, 1)
        checkDimensionMatch(mu, sigma, 0)

        mu = mu.astype(np.float64)
        sigma = sigma.astype(np.float64)

        self.precision = precision
        self.mu = mu
        self.sigma = sigma
        self.num_channels = mu.shape[0]
        self.mu_c = np.ctypeslib.as_ctypes(mu)
        self.sigma_c = np.ctypeslib.as_ctypes(sigma)

        self.f_encode = (
            clib._Z30encode_single_channel_gaussianPiiiPK19GaussianCDFTableRaw
        )
        self.f_decode = (
            clib._Z30decode_single_channel_gaussian9BitStreamiiPK19GaussianCDFTableRaw
        )

    def get_ctype(self):
        return c_GaussianCDFTable(self.precision, self.num_channels, self.mu_c, self.sigma_c)
        
    def checkData(self, inputs):
        inputs = super().checkData(inputs)
        inputs = inputs.transpose()
        minv = np.floor(self.mu - 6*self.sigma)
        maxv = np.ceiling(self.mu + 6*self.sigma)
        less = np.less(inputs, minv)
        greater = np.greater(inputs, maxv)
        num_modifies = np.sum(less) + np.sum(greater)
        if num_modifies > 0:
            num_elements = inputs.shape[0] * inputs.shape[1]
            warnings.warn(f"Values out of range detected. {num_modifies} out of {num_elements} values have been clipped.", RuntimeWarning)
            inputs = np.where(less, minv, inputs)
            inputs = np.where(greater, maxv, inputs)
        inputs = inputs.transpose()
        return inputs


def encodeGaussian(
    latent: np.array, mu: np.array, sigma: np.array, precision: int = 16
) -> bytearray:
    """
    Encode a gaussian entropy bottleneck
    """
    cdf = GaussianCDFTable(mu, sigma, precision)
    return cdf.encode(latent)


def decodeGaussian(
    data: bytearray, length, mu: np.array, sigma: np.array, precision: int = 16
) -> np.array:
    """
    Decode a gaussian entropy bottleneck
    """
    cdf = GaussianCDFTable(mu, sigma, precision)
    return cdf.decode(data, length)
