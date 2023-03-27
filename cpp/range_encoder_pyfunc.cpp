#include "range_encoder.h"

static vector<int> result;

BitStream encode_listcdf_pyfunc(int latent[], int dim0, int dim1, ListCDFTableRaw *cdf_raw) {
    ListCDFTable cdf(*cdf_raw);
    BitStreamDynamic bits = encode_single_channel(latent, dim0, dim1, cdf);
    BitStream bits_raw(bits);
    return bits_raw;
}

int* decode_listcdf_pyfunc(BitStream bits_raw, int dim0, int dim1, ListCDFTableRaw *cdf_raw) {
    ListCDFTable cdf(*cdf_raw);
    BitStreamDynamic bits(bits_raw);
    result = decode_single_channel(bits, dim0, dim1, cdf);
    return &(result[0]);
}

BitStream encode_gaussian_pyfunc(int latent[], int dim0, int dim1, GaussianCDFTableRaw *cdf_raw) {
    GaussianCDFTable cdf(*cdf_raw);
    BitStreamDynamic bits = encode_single_channel(latent, dim0, dim1, cdf);
    BitStream bits_raw(bits);
    return bits_raw;
}

int* decode_gaussian_pyfunc(BitStream bits_raw, int dim0, int dim1, GaussianCDFTableRaw *cdf_raw) {
    GaussianCDFTable cdf(*cdf_raw);
    BitStreamDynamic bits(bits_raw);
    result = decode_single_channel(bits, dim0, dim1, cdf);
    return &(result[0]);
}