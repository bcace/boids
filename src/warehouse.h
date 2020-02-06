#ifndef warehouse_h
#define warehouse_h

#include "part.h"
#include "array.h"

#define MAX_PARTS   64


struct Object;
struct mat4_stack;
struct ShaderProgram;

struct Warehouse {
    array<Part, MAX_PARTS> parts;

    int selected_part;
    bool is_open;

    Warehouse();

    void open();
    void close();
    void select_next_part();
    void select_prev_part();
    Object *make_selected_part(vec3 camera_pos, vec3 camera_dir);

    void update_drawing_geometry();
    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir);
    void draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir);
};

extern Warehouse warehouse;

#endif
