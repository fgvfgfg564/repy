#ifndef _H_RANGE_ENCODER_
#define _H_RANGE_ENCODER_

#include <bits/stdc++.h>
#include "CDFTable.h"

struct BitStream
{
    int length;
    char *s;
    BitStream(std::string _s): length(_s.length()) {
        s = new char[length];
        memcpy(s, _s.c_str(), length);
    }
};

BitStream encode_single_channel(int [], int, int, const CDFTable&);
const int* decode_single_channel(BitStream, int, int, const CDFTable&);

#endif