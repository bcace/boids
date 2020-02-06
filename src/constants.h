#ifndef constants_h
#define constants_h

// TODO: rename this file

#define PI  3.1415926535897932384626433832795
#define TAU 6.283185307179586476925286766559

#define SHAPE_CURVES                    4
#define SHAPE_CURVES_2                  (SHAPE_CURVES / 2)
#define SHAPE_MIN_CURVE_SUBDIVS         2
#define SHAPE_MAX_CURVE_SUBDIVS         8
#define SHAPE_MAX_ENVELOPE_SHAPES       32
#define SHAPE_MAX_SHAPE_SUBDIVS         (SHAPE_MAX_CURVE_SUBDIVS * SHAPE_CURVES)
#define SHAPE_MAX_ENVELOPE_POINTS       (SHAPE_MAX_ENVELOPE_SHAPES * SHAPE_MAX_SHAPE_SUBDIVS)
#define MAX_FUSELAGE_OBJECTS            32
#define MAX_ORIGINS                     (MAX_FUSELAGE_OBJECTS * 2)

#define ZERO_ORIGIN_FLAG                0ull
#define ORIGIN_PART_TO_FLAG(__part)     (OriginFlags)(1ull << (__part))

typedef short int OriginPart;
typedef unsigned long long int OriginFlags; /* make sure MAX_ORIGINS fits into sizeof(OriginFlags) */

struct Origin {
    OriginPart tail;
    OriginPart nose;
};

#endif
