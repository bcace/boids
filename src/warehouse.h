#ifndef warehouse_h
#define warehouse_h

#include "part.h"

#define MAX_PARTS   64


struct Object;
struct mat4_stack;
struct ShaderProgram;

enum WarehouseMode { wmClosed, wmObject, wmWing };

struct Warehouse {
    Part parts[MAX_PARTS];
    int parts_count;
    int selected_part;
    WarehouseMode mode;

    Warehouse();

    void open(bool wings);
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
