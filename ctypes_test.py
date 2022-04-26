from ctypes import *
from math import log2
from random import randint
from time import localtime, time
from tkinter.messagebox import NO
from sympy import Point
import tensorflow as tf
import numpy as np

clib = CDLL("cpp/libre.so")


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
            raise ValueError(f"Dimensions mismatch! Got {dim1} vs. self.n={self.n}")
        latent = latent.reshape((-1,))
        c_latent = (c_int * (dim1 * dim2))(*latent)
        return self.f_encode(c_latent, dim1, dim2, pointer(self.get_ctype()))

    def decode(self, bitstream: c_BitStream, dim2):
        self.f_decode.restype = POINTER(c_int)
        c_latent_recon = self.f_decode(
            bitstream, self.n, dim2, pointer(self.get_ctype())
        )
        recon = np.ctypeslib.as_array(c_latent_recon, (self.n * dim2,))
        recon = recon.reshape((self.n, dim2))
        return recon


class ListCDFTable(CDFTable):
    def __init__(self, precision, n, cdf, maxv, minv, max_cdf_length):
        self.precision = precision
        self.n = n
        cdf = np.reshape(cdf, [-1])
        self.cdf = np.ctypeslib.as_ctypes(cdf)
        self.maxv = np.ctypeslib.as_ctypes(maxv)
        self.minv = np.ctypeslib.as_ctypes(minv)
        self.max_cdf_length = max_cdf_length
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
    def __init__(self, precision, n, mu, sigma):
        self.precision = precision
        self.n = n
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


def test(latent, cdf):
    start_time = time()
    result = cdf.encode(latent)
    encode_time = time()
    recon = cdf.decode(result, latent.shape[1])
    decode_time = time()
    return (
        np.all(latent == recon),
        result.length,
        decode_time - encode_time,
        encode_time - start_time,
    )


def test_gaussian():
    precision = 16
    shape = (192, 1920 * 1080 // 256)
    mu_rng = 256
    sigma_rng = 12
    while True:
        mu = np.random.random(size=shape[0]) * mu_rng
        sigma = np.random.random(size=shape[0]) * sigma_rng + 0.3
        latent = np.random.normal(mu, sigma, (shape[1], shape[0]))
        latent = np.clip(latent, mu - 3 * sigma, mu + 3 * sigma)
        latent = np.transpose(latent, (1, 0))
        latent = latent.astype(np.int32)

        esti = 1
        # for i in range(shape[0]):
        #     for j in range(shape[1]):
        #         ind = latent[i, j] - minv[i]
        #         prob = cdfs[i, ind + 1] - cdfs[i, ind]
        #         bits = precision - log2(prob)
        #         esti += bits

        cdfTable = GaussianCDFTable(precision, shape[0], mu, sigma)
        result, real, enc_t, dec_t = test(latent, cdfTable)
        print(
            ("Passed!" if result else "Failed!")
            + f"\tEnc = {enc_t:.3f}s; Dec = {dec_t:.3f}s; Esti = {esti}; Real = {real * 8}; Overflow = {100 * (real * 8 / esti - 1):.4f}%"
        )
        if not result:
            break


def test_list():
    precision = 16
    shape = (192, 1920 * 1080 // 256)
    rng = 256
    while True:
        minv = np.random.randint(-rng, 0, size=[shape[0]], dtype=np.int32)
        maxv = minv + rng

        latent = np.ndarray(shape=shape, dtype=np.int32)
        for i in range(shape[0]):
            latent[i, :] = np.random.randint(minv[i], maxv[i], size=shape[1])
        cdfs = np.random.randint(
            0, 2 ** precision - rng, size=[shape[0], rng + 2], dtype=np.int32
        )
        cdfs[:, 0] = 0
        cdfs[:, 1] = 2 ** precision - rng - 1
        for i in range(shape[0]):
            cdfs[i] = np.sort(cdfs[i])
        for i in range(rng + 2):
            cdfs[:, i] += i

        esti = 0
        for i in range(shape[0]):
            for j in range(shape[1]):
                ind = latent[i, j] - minv[i]
                prob = cdfs[i, ind + 1] - cdfs[i, ind]
                bits = precision - log2(prob)
                esti += bits

        cdfTable = ListCDFTable(
            precision, shape[0], cdfs, maxv, minv, max_cdf_length=rng + 2
        )
        result, real, enc_t, dec_t = test(latent, cdfTable)
        print(
            ("Passed!" if result else "Failed!")
            + f"\tEnc = {enc_t:.3f}s; Dec = {dec_t:.3f}s; Esti = {esti}; Real = {real * 8}; Overflow = {100 * (real * 8 / esti - 1):.4f}%"
        )
        if not result:
            break


if __name__ == "__main__":
    test_gaussian()
