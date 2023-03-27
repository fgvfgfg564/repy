#ifndef _H_RANGE_ENCODER_
#define _H_RANGE_ENCODER_

#define OUTPUT_BITS 13

#include <bits/stdc++.h>
#include "CDFTable.h"

struct BitStreamDynamicIterator;

struct BitStreamDynamic
{
    vector<unsigned char> s;
    int bias, ind;
    BitStreamDynamic(): bias(7), ind(0) {s.push_back(0);}
    // BitStreamDynamic(const BitStream &_s);
    int length() {
        return ind*8+(7-bias);
    }
    void append(int bit) {
        if(bit != 0 && bit != 1) {
            cerr << "FATAL: Invalid bit: " << int(bit) << endl;
            exit(-1);
        }
        bit <<= bias;
        s[ind] |= bit;
        if(bias == 0) {
            s.push_back(0);
            bias = 7;
            ind ++;
        }
        else
            bias --;
    }

    BitStreamDynamicIterator iter();
};

struct BitStreamDynamicIterator
{
    BitStreamDynamic *_b;
    int ind, bias;
    BitStreamDynamicIterator(BitStreamDynamic *b): _b(b), ind(0), bias(7) {}
    unsigned char next() {
        unsigned char result;
        if(ind > _b->ind) {
            result = 0;
        }
        else {
            result = _b->s[ind];
            result &= (1<<bias);
            result = !(!result);
        }
        if(bias == 0) {
            ind ++;
            bias = 7;
        }
        else bias --;
        return result;
    }
};

BitStreamDynamicIterator BitStreamDynamic::iter() {
    return BitStreamDynamicIterator(this);
}

// struct BitStream
// {
//     int length;
//     char *s;
//     BitStream(const BitStreamDynamic &_s): length(_s.s.size()) {
//         s = new char[length];
//         memcpy(s, &(_s.s[0]), length);
//     }
// };

BitStreamDynamic encode_single_channel(int [], int, int, const CDFTable&);
vector<int> decode_single_channel(BitStreamDynamic, int, int, const CDFTable&);

#endif