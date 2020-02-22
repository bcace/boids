#ifndef wing_h
#define wing_h


struct Chord {
    float x, y, z; /* model CS */
    float chord;
    float angle;
};

struct Wing {
    Chord c1, c2;
};

#endif
