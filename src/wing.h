#ifndef wing_h
#define wing_h


/*
LE - leading edge.
TE - trailing edge.
LS - leading fuselage station.
TS - trailing fuselage station.
L - line segment between two anchors.

LS position is anchor plus half of chord.
To determine LE intersection with LS:
- Determine the plane using both anchors and a point that's unit distance from anchor and rotated by AOA around vector that's a projection of L in the x = anchor.x plane.
- Determine intersection line between that plane and x = LS.x plane.
- Intersect this line with LS envelope polygon.
*/

struct Anchor {
    float x, y, z; /* model CS */

    /* following values are ideal, real values will be somewhat different */
    float chord; /* distance between leading and trailing edge fuselage stations */
    float angle;
};

struct Wing {
    Anchor a1, a2;
};

#endif
