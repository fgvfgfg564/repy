#include "bitstream.h"

BitStreamDynamicIterator BitStreamDynamic::iter() {
    return BitStreamDynamicIterator(this);
}
BitStreamDynamic::BitStreamDynamic(const BitStream &_bits): s(_bits.s, _bits.s+_bits.length), bias(7), ind(_bits.length) {}