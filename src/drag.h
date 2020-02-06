#ifndef drag_h
#define drag_h

#include "vec.h"


struct Drag {
    bool dragging;
    float travel;
    float hold_distance;
    vec3 origin_pos;
    vec3 target_xyz;
    vec3 target_yz;

    Drag();

    void begin(vec3 camera_pos, vec3 camera_dir, float pick_depth);
    vec3 drag(vec3 camera_pos, vec3 camera_dir);
    void end();
};

#endif
