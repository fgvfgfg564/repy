import numpy as np


class Codebook:
    def __init__(self, cdf, start) -> None:
        """
        cdf: np.array with shape [..., range+2]; ... may indicate external channels
        [..., 0] = precision(bits)
        [..., 1] = P(x<start) * 2^precision = 0
        [..., 2] = P(x<start+1) * 2^precision
        [..., 3] = P(x<start+2) * 2^precision
        ...
        [..., range+2] = P(x<start+range+1) * 2^precision = 1
        """
        self.cdf = cdf
        self.start = start
