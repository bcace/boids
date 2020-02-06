#ifndef pick_h
#define pick_h

#include "vec.h"

#define MAX_BITS            32
#define CATEGORY_OFFSET     4
#define CATEGORY_MASK       0xf
#define MAX_CATEGORIES      16
#define MAX_SEGMENTS        (MAX_BITS - CATEGORY_OFFSET)


struct PickResult {
    bool hit;
    unsigned code;
    unsigned category_id;
    unsigned ids[MAX_SEGMENTS];
    float depth;

    PickResult();

    void clear();
};

/*
USAGE
    If there's a data model tree, we want to have a separate category for objects that
    use the same algorithm but different ids (indices) to traverse the tree and find
    the picked object.
*/

unsigned pick_register_category();
void pick_define_encoding(unsigned category_id, int size1);
void pick_define_encoding(unsigned category_id, int size1, int size2);
vec4 pick_encode(unsigned category_id, unsigned v0);
vec4 pick_encode(unsigned category_id, unsigned v0, unsigned v1);
vec4 pick_encode(unsigned category_id, unsigned v0, unsigned v1, unsigned v2);
void pick(int pick_radius, int w, int h, int near, int far, PickResult &result, bool get_depth=false);

#endif
