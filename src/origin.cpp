#include "origin.h"


const OriginFlag ZERO_ORIGIN_FLAG = 0ull;

OriginFlag origin_index_to_flag(int index) {
    return (OriginFlag)(1ull << index);
}
