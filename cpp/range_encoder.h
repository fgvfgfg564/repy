#ifndef _H_RANGE_ENCODER_
#define _H_RANGE_ENCODER_

#define SAFETY_BITS 0
#define OUTPUT_BITS 13

#include <bits/stdc++.h>
#include "CDFTable.h"

struct BitStream;

struct BitStreamDynamic
{
    vector<unsigned char> s;
    int input_cnt, input_ind, output_cnt, output_ind;
    BitStreamDynamic(): input_ind(0), output_ind(7), input_cnt(0), output_cnt(0) {s.push_back(0);}
    BitStreamDynamic(const BitStream &_s);
    void append(int bit) {
        if(bit != 0 && bit != 1) {
            cerr << "FATAL: Invalid bit: " << int(bit) << endl;
            exit(0);
        }
        s[input_ind] <<= 1;
        s[input_ind] |= bit;
        input_cnt ++;
        if(input_cnt == 8) {
            s.push_back(0);
            input_cnt = 0;
            input_ind ++;
        }
    }
    unsigned char read_bit() {
        unsigned char result;
        if(output_cnt >= s.size()) {
            result = 0;
        }
        else {
            result = s[output_cnt];
            result &= (1<<output_ind);
            result = !(!result);
        }
        if(output_ind == 0) {
            output_cnt ++;
            output_ind = 7;
        }
        else output_ind --;
        return result;
    }
    void reset_iter() {
        output_cnt = 0;
        output_ind = 0;
    }
};

struct BitStream
{
    int length;
    char *s;
    BitStream(const BitStreamDynamic &_s): length(_s.s.size()) {
        s = new char[length];
        memcpy(s, &(_s.s[0]), length);
    }
};

BitStreamDynamic::BitStreamDynamic(const BitStream &_s): input_ind(0), output_ind(7), input_cnt(0), output_cnt(0), s(_s.s, _s.s + _s.length) {}

BitStream encode_single_channel(int [], int, int, const CDFTable&);
const int* decode_single_channel(BitStreamDynamic, int, int, const CDFTable&);

#endif