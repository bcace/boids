#ifndef constants_h
#define constants_h

/* This module sohuld contain an absolute minimum of constants that get
used throughout the entire project and should be true compile-time constants. */

#define PI                        3.1415926535897932384626433832795
#define TAU                       6.283185307179586476925286766559
#define SHAPE_CURVES              4
#define MIN_CURVE_SUBDIVS         2
#define MAX_CURVE_SUBDIVS         8
#define MAX_ENVELOPE_SHAPES       32
#define MAX_SHAPE_SUBDIVS         (MAX_CURVE_SUBDIVS * SHAPE_CURVES)
#define MAX_ENVELOPE_POINTS       (MAX_ENVELOPE_SHAPES * MAX_SHAPE_SUBDIVS)

#endif
