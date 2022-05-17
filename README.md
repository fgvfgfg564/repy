# Python Range Encoder
A lightweight Python Range Encoder for NumPy-based arrays.
## Install
```
pip install -r requirements.txt
cd cpp
make clean && make
```
## Usage
### Encode a tensor with a CDF function represented by a list
```py
from repy import ListCDFTable

cdf = ListCDFTable(cdf, maxv, minv, precision)
dim0, dim1 = tensor.shape
bin = cdf.encode(tensor)
tensor_recon = cdf.decode(bin, dim1)
```

This procedure assumes that each elemenet under the same `dim0` shares the same CDF function. Therefore, while decoding, only `dim1` needs to be provided.

- `precision`: int scalar within range [1, 16]. Default: 16.
- `maxv`: the maximum possible value in any tensor to be encoded, with shape `(num_channels)`
- `minv`: the minimum possible value in any tensor to be encoded, with shape `(num_channels)`
- `cdf`: an array containing possibility for each number in each channel. 
  - `cdf` array has shape `(num_channels, k)`
  - `cdf[c]` represents the provided cumulative distribution in channel `c`. To be precise, `cdf[c][t] := P(x<t+minv[c])*2^precision` in channel `c`.
  - For all `c`, `cdf[c][0]` equals to 0.
  - For all `c`, `cdf[c][maxv[c]-minv[c]+1]` equals to `2^precision`

### Encode a tensor with a Gaussian CDF function
```py
from repy import encodeGaussian, decodeGaussian

dim0, dim1 = tensor.shape
bin = encodeGaussian(tensor, mu, sigma, precision)
tensor_recon = decodeGaussian(bin, dim1, mu, sigma, precision)
```

This procedure also assumes that each elemenet under the same `dim0` shares the same CDF function. Therefore, while decoding, only `dim1` needs to be provided.

- `mu` and `sigma` should have the same shape with the tensor to be encoded.