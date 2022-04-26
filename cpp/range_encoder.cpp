#include "range_encoder.h"
#include "CDFTable.h"

using namespace std;

#define SAFETY_BITS 0
#define OUTPUT_BITS 13

int start, range, prob_base, first_bit_base, prob_safe;
char bit_cache;
int bit_cache_cnt;
int cur, msg_ind, msg_ind_cache;
string msg;
vector<int> result;

void output_bit(char bit) {
    // cout<<"output bit: "<<int(bit)<<endl;
    switch(bit) {
        case 0:
            bit_cache <<= 1;
            bit_cache_cnt ++;
            break;
        case 1:
            bit_cache <<= 1;
            bit_cache |= 1;
            bit_cache_cnt ++;
            break;
        default:
            cerr << "FATAL: Invalid bit: " << int(bit) << endl;
            exit(0);
    }
    if(bit_cache_cnt == 8) {
        //cout<<"output: "<<int(bit_cache)<<endl;
        msg += bit_cache;
        bit_cache_cnt = 0;
    }
}

void readbit()
{
    int result;
    if(msg_ind >= msg.length()) {
        result = 0;
    }
    else {
        result = msg[msg_ind];
        result &= (1<<msg_ind_cache);
        result = !(!result);
    }
    // cout << "Read bit: " << result << ' ' << msg.length() << ' ' << msg_ind << endl;
    cur = (cur<<1) | result;
    if(msg_ind_cache == 0) {
        msg_ind ++;
        msg_ind_cache = 7;
    }
    else msg_ind_cache --;
}   

void emit_bits(bool encode=true) {
    while(start / first_bit_base == (start + range - 1) / first_bit_base) {
        // printf("%x, %x, %x\n", start, range, first_bit_base);
        int bit = start / first_bit_base;
        if(encode){
            // int bit_reverse = 0;
            // int cnt = 0;
            // do {
            //     bit_reverse = (bit_reverse << 1) | (bit & 1);
            //     bit >>= 1;
            //     cnt += 1;
            // } while(bit);
            // for(int i=0;i<cnt;i++){
            //     output_bit(bit_reverse & 1);
            //     bit_reverse >>= 1;
            // }
            output_bit(bit);
        }
        start = (start & (first_bit_base - 1)) << 1;
        if(!encode){
            cur = cur & (first_bit_base - 1);
            readbit();
        }
        range <<= 1;
        // printf(" - %x, %x, %x\n", start, range, first_bit_base);
    }
}

void align_range(int target, bool encode=true) {
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
        emit_bits(encode);
    }
}

void init_enc() {
    bit_cache_cnt = 0;
    msg = "";
}

long long ceiling_div(long long x, long long y)
{
    return (x+y-1)/y;
}

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

BitStream encode_single_channel(int latent[], int dim1, int dim2, const CDFTable &cdf)
{
    init_enc();
    start = 0;
    int precision = cdf.getPrecision();
    range = 1 << (precision + SAFETY_BITS + OUTPUT_BITS + 1);
    prob_safe = 1 << (precision + SAFETY_BITS);
    prob_base = 1 << precision;
    first_bit_base = 1 << (precision + SAFETY_BITS + OUTPUT_BITS);
    for(int idx=0;idx<dim1;idx++){
        for(int i=0;i<dim2;i++) {
            int val = latent[idx * dim2 + i];
            int prob_start = cdf(idx, val);
            int prob_end = cdf(idx, val+1);
            start = start + ceiling_div(1ll * range * prob_start, prob_base);
            range = ceiling_div(1ll * range * prob_end, prob_base) - ceiling_div(1ll * range * prob_start, prob_base);
            
            if(range == 0) {
                cerr << "FATAL: Probability equals to zero: val=" << val << endl;
                exit(0);
            }
            emit_bits();
            align_range(prob_safe);
        }
    }
    align_range(first_bit_base << 1);
    if(bit_cache_cnt != 0)msg += bit_cache << (8 - bit_cache_cnt);
    return BitStream(msg);
}

void init_dec()
{
    msg_ind = 0;
    msg_ind_cache = 7;
    start = 0;
    range = 1;
    cur = 0;
    msg = "";
    result.clear();
}

void emit_decoder_bits()
{
    while(start / first_bit_base == (start + range - 1) / first_bit_base) {
        int bit = start / first_bit_base;
        start = (start & (first_bit_base - 1)) << 1;
        cur = (cur & (first_bit_base - 1)) << 1;
        range <<= 1;
    }
}

const int* decode_single_channel_list(BitStream msg_stream, int dim1, int dim2, const ListCDFTableRaw *raw_cdf)
{
    ListCDFTable cdf(*raw_cdf);
    return decode_single_channel(msg_stream, dim1, dim2, cdf);
}

const int* decode_single_channel_gaussian(BitStream msg_stream, int dim1, int dim2, const GaussianCDFTableRaw *raw_cdf)
{
    GaussianCDFTable cdf(*raw_cdf);
    return decode_single_channel(msg_stream, dim1, dim2, cdf);
}

const int* decode_single_channel(BitStream msg_stream, int dim1, int dim2, const CDFTable &cdf)
{
    init_dec();
    int precision = cdf.getPrecision();
    msg = string(msg_stream.s, msg_stream.length);
    // cout << "Received msg length: " << msg.size() << " Bytes" << endl;
    prob_safe = 1 << (precision + SAFETY_BITS);
    prob_base = 1 << precision;
    first_bit_base = 1 << (precision + SAFETY_BITS + OUTPUT_BITS);

    for(int i=0;i<precision + SAFETY_BITS + OUTPUT_BITS + 1;i++) {
        readbit();
        range <<= 1;
    }
    // cout<<"cur="<<cur<<endl;
    for(int idx=0;idx<dim1;idx++) {
        // cout <<idx << endl;
        for(int i=0;i<dim2;i++) {
            int prob = (1ll * (cur - start) * prob_base) / range;
            int val = cdf.lookup(idx, prob);
            result.push_back(val);
            int prob_start = cdf(idx, val);
            int prob_end = cdf(idx, val+1);
            // cout<<prob<<'-'<<start<<','<<prob<<','<<range<<endl;
            start = start + ceiling_div(1ll * range * prob_start, prob_base);
            range = ceiling_div(1ll * range * prob_end, prob_base) - ceiling_div(1ll * range * prob_start, prob_base);
            // cout<<prob<<' '<<start<<'|'<<prob<<'|'<<range<<endl;
            emit_bits(false);
            align_range(prob_safe, false);
        }
    }
    return &(*result.begin());
}