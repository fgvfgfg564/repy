#include "bitstream.h"
#include "CDFTable.h"

using namespace std;

inline void read_bit(int &cur, BitStreamDynamicIterator &bits, int first_bit_base) {
    cur = (cur & (first_bit_base - 1)) << 1;
    unsigned char next_bit=bits.next();
    cur |= next_bit;
}

inline int emit_bit(int &start, int &range, int first_bit_base) {
    assert(start / first_bit_base == (start + range - 1) / first_bit_base);
    int bit = start / first_bit_base;
    start = (start & (first_bit_base - 1)) << 1;
    range <<= 1;
    return bit;
}

// 若range开头结尾的首个bit一致，则将其输出并整体左移一位
void emit_initio_bits_encode(int &start, int &range, BitStreamDynamic &bits, int first_bit_base) {
    while(start / first_bit_base == (start + range - 1) / first_bit_base) {
        int bit = emit_bit(start, range, first_bit_base);
        bits.append(bit);
    }
}

// 若range开头结尾的首个bit一致，则读入下一位并整体左移一位
void emit_initio_bits_decode(int &start, int &range, int &cur, BitStreamDynamicIterator &bits, int first_bit_base) {
    while(start / first_bit_base == (start + range - 1) / first_bit_base) {
        int bit = emit_bit(start, range, first_bit_base);
        read_bit(cur, bits, first_bit_base);
    }
}

inline void align_range(int &start, int &range, int first_bit_base) {
    int range_l = first_bit_base - start;
    int range_r = range - range_l;
    if (range_l > range_r) {
        range = range_l;
    }
    else {
        start += range_l;
        range = range_r;
    }
}

// 如果range的长度小于字符集长度，会导致概率为0无法编码，因此裁剪一部分range使其能够扩充
void align_range_encode(int &start, int &range, int target, BitStreamDynamic &bits, int first_bit_base) {
    while (range < target) {
        align_range(start, range, first_bit_base);
        emit_initio_bits_encode(start, range, bits, first_bit_base);
    }
}

void align_range_decode(int &start, int &range, int &cur, int target, BitStreamDynamicIterator &bits, int first_bit_base) {
    while (range < target) {
        align_range(start, range, first_bit_base);
        emit_initio_bits_decode(start, range, cur, bits, first_bit_base);
    }
}


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
            // cout << prob_start << ' ' << prob_end << ' ' <<start<<' ' <<range<<endl;
            start = start + (((1ll * range * prob_start)) >> precision);
            range = (1ll * range * (prob_end - prob_start)) >> precision;
            
            if(range == 0) {
                cerr << "FATAL: Probability equals to zero: val=" << val << endl;
                exit(-1);
            }
            emit_initio_bits_encode(start, range, msg, first_bit_base);
            align_range_encode(start, range, prob_base, msg, first_bit_base);
        }
    }
    // 编码结束，剩余range一定横跨 1<<first_bit_base 点，在码流中额外给出一个1
    msg.append(1);
    return msg;
}

vector<int> decode_single_channel(BitStreamDynamic msg, int dim1, int dim2, const CDFTable &cdf)
{
    auto msg_iter = msg.iter();
    int start = 0, range = 1, cur = 0;
    vector<int> result;
    result.clear();
    int precision = cdf.getPrecision();
    int prob_base = 1 << precision;
    int first_bit_base = 1 << (precision + OUTPUT_BITS);

    for(int i=0;i<precision + OUTPUT_BITS+1;i++) {
        read_bit(cur, msg_iter, first_bit_base);
        range <<= 1;
    }
    for(int idx=0;idx<dim1;idx++) {
        for(int i=0;i<dim2;i++) {
            // 从输入的数值反推其位于哪个值的概率区间内
            int prob = (1ll * (cur - start + 1) * prob_base - 1) / range;   
            int val = cdf.lookup(idx, prob);
            result.push_back(val);
            int prob_start = cdf(idx, val);
            int prob_end = cdf(idx, val+1);
            // cout << prob_start << ' ' << prob_end << ' ' <<start<<',' <<range<<','<<cur<<' ' << prob << endl;
            // 求出新的range，与编码端一致
            start = start + (((1ll * range * prob_start)) >> precision);
            range = (1ll * range * (prob_end - prob_start)) >> precision;
            emit_initio_bits_decode(start, range, cur, msg_iter, first_bit_base);   
            align_range_decode(start, range, cur, prob_base, msg_iter, first_bit_base);
        }
    }
    return result;
}