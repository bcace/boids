#ifndef camera_h
#define camera_h

#include "math_vec.h"


struct Camera {
    float near, far, fov;
    vec3 pos, dir, vel;
    bool move_f, move_b, move_l, move_r, move_u, move_d;
    float rot_x, rot_y;
    bool moved;

    Camera(float _near, float _far, float _fov);

    void on_keys(int key, int action);
    void on_mousepos(float x, float y);
    void on_scroll(float x, float y);
    void update();
};

#endif
