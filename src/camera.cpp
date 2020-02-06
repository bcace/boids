#include "camera.h"
#include "platform.h"
#include "vec.h"
#include "mat.h"

#define MOVE_VELOCITY   0.03
#define SCROLL_VELOCITY 0.06
#define ROTATE_VELOCITY 0.002


Camera::Camera(float _near, float _far, float _fov) : near(_near), far(_far), fov(_fov), pos(10, 10, 10), dir(-1, -1, -1), vel(0, 0, 0) {
    dir.normalize();
    move_f = move_b = move_l = move_r = move_u = move_d = false;
    rot_x = rot_y = 0.0f;
}

void Camera::on_keys(int key, int action) {
    if (key == PLATFORM_KEY_W)
        move_f = action != PLATFORM_RELEASE;
    else if (key == PLATFORM_KEY_A)
        move_l = action != PLATFORM_RELEASE;
    else if (key == PLATFORM_KEY_S)
        move_b = action != PLATFORM_RELEASE;
    else if (key == PLATFORM_KEY_D)
        move_r = action != PLATFORM_RELEASE;
}

void Camera::on_mousepos(float x, float y) {
    Mouse &mouse = plat_get_mouse();
    rot_x = mouse.pos.x - x;
    rot_y = mouse.pos.y - y;
}

void Camera::on_scroll(float x, float y) {
    move_u = y > 0;
    move_d = y < 0;
}

void Camera::update() {
    bool rotated = false;

    if (rot_x != 0.0f || rot_y != 0.0f) {
        mat3 r;
        r.set_rotation(rot_x * ROTATE_VELOCITY, vec3(0, 0, 1));
        dir = r * dir;
        r.set_rotation(rot_y * ROTATE_VELOCITY, vec3(0, 0, 1).cross(dir));
        dir = r * dir;
        dir.normalize();
        rot_x = rot_y = 0.0f;
        rotated = true;
    }

    if (move_f)
        vel += dir * MOVE_VELOCITY;
    else if (move_b)
        vel -= dir * MOVE_VELOCITY;
    if (move_u) {
        vel += vec3(0, 0, SCROLL_VELOCITY);
        move_u = false;
    }
    else if (move_d) {
        vel -= vec3(0, 0, SCROLL_VELOCITY);
        move_d = false;
    }
    vec3 right = dir.cross(vec3(0, 0, 1));
    right.normalize();
    if (move_l)
        vel -= right * MOVE_VELOCITY;
    else if (move_r)
        vel += right * MOVE_VELOCITY;
    vel *= 0.87;

    pos += vel;
    moved = rotated || vel.length() > 0.0001f;
}
