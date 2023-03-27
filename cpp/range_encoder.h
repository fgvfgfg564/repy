#ifndef _H_RANGE_ENCODER_
#define _H_RANGE_ENCODER_

#include "bitstream.h"

BitStreamDynamic encode_single_channel(int [], int, int, const CDFTable&);
vector<int> decode_single_channel(BitStreamDynamic, int, int, const CDFTable&);

#endif