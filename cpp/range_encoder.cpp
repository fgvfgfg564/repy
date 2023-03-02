#include "range_encoder.h"
#include "CDFTable.h"

using namespace std;

int prob_base, first_bit_base, prob_safe;

inline void read_bit(int &cur, BitStreamDynamic &bits) {
    cur = (cur & (first_bit_base - 1)) << 1;
    unsigned char next_bit=bits.read_bit();
    cur |= next_bit;
}

// 若range开头结尾的首个bit一致，则将其输出(encode)/读入下一位(decode)并整体左移一位
void emit_initio_bits(int &start, int &range, int &cur, BitStreamDynamic &bits, bool encoding) {
    while(start / first_bit_base == (start + range - 1) / first_bit_base) {
        int bit = start / first_bit_base;
        start = (start & (first_bit_base - 1)) << 1;
        range <<= 1;
        if(encoding)
            bits.append(bit);
        else 
            read_bit(cur, bits);
    }
}

// 如果range的长度小于字符集长度，会导致概率为0无法编码，因此裁剪一部分range使其能够扩充
void align_range(int &start, int &range, int target, BitStreamDynamic &bits, bool encoding) {
    int _;
    while (range < target) {
        int range_l = first_bit_base - start;
        int range_r = range - range_l;
        if (range_l > range_r) {
            range = range_l;
        }
        else {
            start += range_l;
            range = range_r;
        }
        emit_initio_bits(start, range, _, bits, encoding);
    }
}

// Entrance functions for python

BitStream encode_single_channel(int latent[], int dim1, int dim2, const CDFTable &cdf)
{
    BitStreamDynamic msg;
    int start = 0;
    int _;
    int precision = cdf.getPrecision();
    int range = 1 << (precision + SAFETY_BITS + OUTPUT_BITS + 1);
    prob_safe = 1 << (precision + SAFETY_BITS);
    prob_base = 1 << precision;
    first_bit_base = 1 << (precision + SAFETY_BITS + OUTPUT_BITS);
    for(int idx=0;idx<dim1;idx++){
        for(int i=0;i<dim2;i++) {
            int val = latent[idx * dim2 + i];
            int prob_start = cdf(idx, val);
            int prob_end = cdf(idx, val+1);
            // cout << start << ' ' << range << ' ' << prob_start << ' ' << prob_end << endl;
            start = start + 1ll * range * prob_start / prob_base;
            range = 1ll * range * prob_end / prob_base - 1ll * range * prob_start / prob_base;
            
            if(range == 0) {
                cerr << "FATAL: Probability equals to zero: val=" << val << endl;
                exit(0);
            }
            emit_initio_bits(start, range, _, msg, true);
            align_range(start, range, prob_safe, msg, true);
        }
    }
    // 在当前range中挑选一个最短的数值输出
    align_range(start, range, first_bit_base << 1, msg, true);
    return BitStream(msg);
}

const int* decode_single_channel(BitStreamDynamic msg, int dim1, int dim2, const CDFTable &cdf)
{
    int start = 0, range = 1, cur = 0;
    vector<int> result;
    result.clear();
    int precision = cdf.getPrecision();
    prob_safe = 1 << (precision + SAFETY_BITS);
    prob_base = 1 << precision;
    first_bit_base = 1 << (precision + SAFETY_BITS + OUTPUT_BITS);

    for(int i=0;i<precision + SAFETY_BITS + OUTPUT_BITS + 1;i++) {
        read_bit(cur, msg);
        range <<= 1;
    }
    // cout<<"cur="<<cur<<endl;
    for(int idx=0;idx<dim1;idx++) {
        // cout <<idx << endl;
        for(int i=0;i<dim2;i++) {
            int prob = (1ll * (cur - start) * prob_base) / range;
            int val = cdf.lookup(idx, prob);
            // cout << start << ' ' << range << ' ' << prob << ' ' << val << endl;
            result.push_back(val);
            int prob_start = cdf(idx, val);
            int prob_end = cdf(idx, val+1);
            start = start + 1ll * range * prob_start / prob_base;
            range = 1ll * range * prob_end / prob_base - 1ll * range * prob_start / prob_base;
            emit_initio_bits(start, range, cur, msg, false);
            align_range(start, range, prob_safe, msg, false);
        }
    }
    return &(*result.begin());
}

/*********************************
 * Entrance functions for Python *
 *********************************/

BitStream encode_single_channel_list(int latent[], int dim1, int dim2, const ListCDFTableRaw *p_raw_cdf)
{
    ListCDFTable cdf(*p_raw_cdf);
    return encode_single_channel(latent, dim1, dim2, cdf);
}

BitStream encode_single_channel_gaussian(int latent[], int dim1, int dim2, const GaussianCDFTableRaw *raw_cdf)
{
    GaussianCDFTable cdf(*raw_cdf);
    return encode_single_channel(latent, dim1, dim2, cdf);
}

const int* decode_single_channel_list(BitStream msg, int dim1, int dim2, const ListCDFTableRaw *raw_cdf)
{
    ListCDFTable cdf(*raw_cdf);
    return decode_single_channel(msg, dim1, dim2, cdf);
}

const int* decode_single_channel_gaussian(BitStream msg, int dim1, int dim2, const GaussianCDFTableRaw *raw_cdf)
{
    GaussianCDFTable cdf(*raw_cdf);
    return decode_single_channel(msg, dim1, dim2, cdf);
}