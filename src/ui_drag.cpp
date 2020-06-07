#include "ui_drag.h"


Drag::Drag() : dragging(false) {}

void Drag::begin(vec3 camera_pos, vec3 camera_dir, float pick_depth) {
    dragging = true;
    travel = 0.0f;
    hold_distance = pick_depth;
    origin_pos = target_xyz = target_yz = camera_pos + camera_dir * hold_distance;
}

vec3 Drag::drag(vec3 camera_pos, vec3 camera_dir) {
    vec3 move;

    vec3 new_xyz = camera_pos + camera_dir * hold_distance;
    move = new_xyz - target_xyz;

    float dx = origin_pos.x - camera_pos.x;
    float r = dx / camera_dir.x; // TODO: do something when camera_dir.x == 0
    vec3 new_yz = camera_pos + camera_dir * r;

    if (travel < 0.2f) {
        travel += move.length();
        move = vec3(0, 0, 0);
    }
    else {
        target_xyz = new_xyz;
        target_yz = new_yz;
    }

    return move;
}

void Drag::end() {
    dragging = false;
}
