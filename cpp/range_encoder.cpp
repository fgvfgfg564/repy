#include "range_encoder.h"
#include "CDFTable.h"

using namespace std;

inline void read_bit(int &cur, BitStreamDynamic &bits, int first_bit_base) {
    cur = (cur & (first_bit_base - 1)) << 1;
    unsigned char next_bit=bits.read_bit();
    cur |= next_bit;
}

// 若range开头结尾的首个bit一致，则将其输出(encode)/读入下一位(decode)并整体左移一位
void emit_initio_bits(int &start, int &range, int &cur, BitStreamDynamic &bits, int first_bit_base, bool encoding) {
    while(start / first_bit_base == (start + range - 1) / first_bit_base) {
        int bit = start / first_bit_base;
        start = (start & (first_bit_base - 1)) << 1;
        range <<= 1;
        if(encoding)
            bits.append(bit);
        else 
            read_bit(cur, bits, first_bit_base);
    }
}

// 如果range的长度小于字符集长度，会导致概率为0无法编码，因此裁剪一部分range使其能够扩充
void align_range(int &start, int &range, int &cur, int target, BitStreamDynamic &bits, int first_bit_base, bool encoding) {
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
        emit_initio_bits(start, range, cur, bits, first_bit_base, encoding);
    }
}

// Entrance functions for python

BitStreamDynamic encode_single_channel(int latent[], int dim1, int dim2, const CDFTable &cdf)
{
    BitStreamDynamic msg;
    int start = 0;
    int _;
    int precision = cdf.getPrecision();
    int range = 1 << (precision + OUTPUT_BITS+1);
    int prob_base = 1 << precision;
    int first_bit_base = 1 << (precision + OUTPUT_BITS);
    for(int idx=0;idx<dim1;idx++){
        for(int i=0;i<dim2;i++) {
            int val = latent[idx * dim2 + i];
            int prob_start = cdf(idx, val);     // 从概率分布表中提取值inx对应的概率区间
            int prob_end = cdf(idx, val+1);
            start = ((start + 1ll * range * prob_start)) >> precision;
            range = (1ll * range * (prob_end - prob_start)) >> precision;
            
            if(range == 0) {
                cerr << "FATAL: Probability equals to zero: val=" << val << endl;
                exit(-1);
            }
            emit_initio_bits(start, range, _, msg, first_bit_base, true);
            align_range(start, range, _, prob_base, msg, first_bit_base, true);
        }
    }
    // 输出当前range中剩余的值
    align_range(start, range, _, first_bit_base << 1, msg, first_bit_base, true);
    return msg;
}

vector<int> decode_single_channel(BitStreamDynamic msg, int dim1, int dim2, const CDFTable &cdf)
{
    int start = 0, range = 1, cur = 0;
    vector<int> result;
    result.clear();
    int precision = cdf.getPrecision();
    int prob_base = 1 << precision;
    int first_bit_base = 1 << (precision + OUTPUT_BITS);

    for(int i=0;i<precision + OUTPUT_BITS+1;i++) {
        read_bit(cur, msg, first_bit_base);
        range <<= 1;
    }
    for(int idx=0;idx<dim1;idx++) {
        for(int i=0;i<dim2;i++) {
            int prob = (1ll * (cur - start + 1) * prob_base - 1) / range;   // 从输入的数值反推其位于哪个值的概率区间内
            int val = cdf.lookup(idx, prob);
            result.push_back(val);
            int prob_start = cdf(idx, val);
            int prob_end = cdf(idx, val+1);
            start = ((start + 1ll * range * prob_start)) >> precision;
            range = (1ll * range * (prob_end - prob_start)) >> precision;
            emit_initio_bits(start, range, cur, msg, first_bit_base, false);    // 求出新的range，与编码端一致
            align_range(start, range, cur, prob_base, msg, first_bit_base, false);
        }
    }
    return result;
}