from ctypes import *
from math import log2
from random import randint
import time
import numpy as np
import os

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
        self.f_encode.restype = c_BitStream
        dim1, dim2 = latent.shape
        if dim1 != self.n:
            raise ValueError(f"Dimensions mismatch in dim0! Got {dim1} vs. self.n={self.n}")
        latent = latent.reshape((-1,))
        c_latent = np.ctypeslib.as_ctypes(latent)
        t = time.time()
        data = self.f_encode(c_latent, dim1, dim2, pointer(self.get_ctype()))
        print("encode_time =", time.time() - t)
        array_type = c_char * data.length
        data_carray = array_type.from_address(addressof(data.s.contents))
        data_bytearray = bytearray(data_carray)
        return data_bytearray

    def decode(self, data_bytearray: bytearray, dim2):
        self.f_decode.restype = POINTER(c_int)
        l = len(data_bytearray)
        array_type = c_char * l
        data_carray = array_type(*data_bytearray)
        data = c_BitStream(l, data_carray)
        t = time.time()
        c_latent_recon = self.f_decode(data, self.n, dim2, pointer(self.get_ctype()))
        print("decode_time =", time.time() - t)
        recon = np.ctypeslib.as_array(c_latent_recon, (self.n * dim2,))
        recon = recon.reshape((self.n, dim2))
        return recon


class ListCDFTable(CDFTable):
    """
    Data structure to maintain a listed entropy bottleneck
    """

    def __init__(
        self, cdf: np.array, maxv: np.array, minv: np.array, precision: int = 16
    ):
        checkDimension(cdf, 2)
        checkDimension(minv, 1)
        checkDimension(maxv, 1)
        checkDimensionMatch(cdf, minv, 0)
        checkDimensionMatch(cdf, maxv, 0)
        self.precision = precision
        self.n = cdf.shape[0]
        self.max_cdf_length = cdf.shape[1]
        cdf = np.reshape(cdf, [-1])
        self.cdf = np.ctypeslib.as_ctypes(cdf)
        self.maxv = np.ctypeslib.as_ctypes(maxv)
        self.minv = np.ctypeslib.as_ctypes(minv)
        self.f_encode = clib._Z26encode_single_channel_listPiiiPK15ListCDFTableRaw
        self.f_decode = (
            clib._Z26decode_single_channel_list9BitStreamiiPK15ListCDFTableRaw
        )

    def get_ctype(self):
        return c_ListCDFTable(
            self.precision,
            self.n,
            self.cdf,
            self.maxv,
            self.minv,
            self.max_cdf_length,
        )


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
        self.n = mu.shape[0]
        self.mu = np.ctypeslib.as_ctypes(mu)
        self.sigma = np.ctypeslib.as_ctypes(sigma)
        self.f_encode = (
            clib._Z30encode_single_channel_gaussianPiiiPK19GaussianCDFTableRaw
        )
        self.f_decode = (
            clib._Z30decode_single_channel_gaussian9BitStreamiiPK19GaussianCDFTableRaw
        )

    def get_ctype(self):
        return c_GaussianCDFTable(self.precision, self.n, self.mu, self.sigma)


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
