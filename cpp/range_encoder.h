#ifndef _H_RANGE_ENCODER_
#define _H_RANGE_ENCODER_

#define OUTPUT_BITS 13

#include <bits/stdc++.h>
#include "CDFTable.h"

// struct BitStream;

struct BitStreamDynamic
{
    vector<unsigned char> s;
    int input_bias, input_ind, output_ind, output_bias;
    BitStreamDynamic(): input_bias(7), output_bias(7), input_ind(0), output_ind(0) {s.push_back(0);}
    // BitStreamDynamic(const BitStream &_s);
    void append(int bit) {
        if(bit != 0 && bit != 1) {
            cerr << "FATAL: Invalid bit: " << int(bit) << endl;
            exit(-1);
        }
        bit <<= input_bias;
        s[input_ind] |= bit;
        if(input_bias == 0) {
            s.push_back(0);
            input_bias = 7;
            input_ind ++;
        }
        else
            input_bias --;
    }
    unsigned char read_bit() {
        unsigned char result;
        if(output_ind >= s.size()) {
            result = 0;
        }
        else {
            result = s[output_ind];
            result &= (1<<output_bias);
            result = !(!result);
        }
        if(output_bias == 0) {
            output_ind ++;
            output_bias = 7;
        }
        else output_bias --;
        return result;
    }
    void reset_iter() {
        output_ind = 0;
        output_bias = 0;
    }
};

// struct BitStream
// {
//     int length;
//     char *s;
//     BitStream(const BitStreamDynamic &_s): length(_s.s.size()) {
//         s = new char[length];
//         memcpy(s, &(_s.s[0]), length);
//     }
// };

// BitStreamDynamic::BitStreamDynamic(const BitStream &_s): input_bias(7), output_bias(7), input_ind(0), output_ind(0), s(_s.s, _s.s + _s.length) {}

BitStreamDynamic encode_single_channel(int [], int, int, const CDFTable&);
vector<int> decode_single_channel(BitStreamDynamic, int, int, const CDFTable&);

#endif