import numpy as np
from ctypes import *

clib = CDLL("libRangeEncoder.so")

class c_BitStream(Structure):
    _fields_ = ("length", c_int), ("s", POINTER(c_char))

f_encode = clib._Z21encode_single_channelPiiS_ii
f_encode.restype = c_BitStream

f_decode = clib._Z21decode_single_channel9BitStreamiPiii
f_decode.restype = POINTER(c_int)

class RangeEncoder:
    def __init__(self, cdf, start) -> None:
        """
        cdf: np.ndarray with shape=[num_channels, range+3] & dtype=np.int32;
        start: int
        [..., 0] = precision(bits) (1~16)
        [..., 1] = P(x<start) * 2^precision = 0
        [..., 2] = P(x<start+1) * 2^precision
        [..., 3] = P(x<start+2) * 2^precision
        ...
        [..., range+2] = P(x<start+range+1) * 2^precision = 1
        """

        self.cdf = cdf
        self.start = start.reshape([-1])
        self.range = self.cdf.shape[-1]-2
    
    def encode(self, latent: np.ndarray):
        """
        latent: np.ndarray with shape=[..., num_channels] & dtype = np.int32
        """
        shape_latent = latent.shape
        shape_cdf = self.cdf.shape
        if shape_latent[-1] != shape_cdf[0]:
            raise ValueError(f"The channel dimensions should match! Find cdf:{shape_cdf}/latent:{shape_latent}")
        
        latent = latent.reshape([-1, shape_latent[-1]])
        latent = latent.clip(self.start, self.start+self.range)
        latent = latent.transpose(1, 0)

        for channel_cdf, channel_latent in zip(self.cdf, latent):
            bits = self._encode_single_channel(channel_cdf, channel_latent)

    def _encode_single_channel(self, cdf, latent):
        size = latent.shape[0]
        c_latent = np.ctypeslib.as_ctypes(latent)
        c_size = c_int(size)
        c_cdf = np.ctypeslib.as_ctypes(cdf)
        c_cdf_maxv = 
        c_minv = c_int(self.start)