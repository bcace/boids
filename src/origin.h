#ifndef origin_h
#define origin_h

/* All object ids (consecutive, starting from 0) must fit into OriginFlag. */
#define MAX_FUSELAGE_OBJECTS (sizeof(OriginFlag) * 8)


typedef short int OriginPart;
typedef unsigned long long int OriginFlag;

extern const OriginFlag ZERO_ORIGIN_FLAG;

struct Origin {
    OriginPart tail;
    OriginPart nose;
};

OriginFlag origin_index_to_flag(int index);

#endif
