#ifndef warehouse_h
#define warehouse_h

#include "modeling_part.h"
#include "modeling_airfoil.h"

#define MAX_PARTS   64
#define MAX_WPROS   64


struct Wing;
struct Object;
struct mat4_stack;
struct ShaderProgram;

/* Wing prototype. */
struct Wpro {
    Airfoil airfoil;
};

enum WarehouseMode { WM_CLOSED, WM_OBJECT, WM_WING };

struct Warehouse {
    Part parts[MAX_PARTS];
    int parts_count;
    int selected_part;
    Wpro wpros[MAX_WPROS];
    int wpros_count;
    int selected_wpro;
    WarehouseMode mode;

    Warehouse();

    void open(bool wings);
    void close();
    void select_next_part();
    void select_prev_part();
    Object *make_selected_part(vec3 camera_pos, vec3 camera_dir);

    void update_drawing_geometry();
    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir);
    void draw_lines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir);
};

Wing *warehouse_make_selected_wing(Warehouse *wh, vec3 camera_pos, vec3 camera_dir);

extern Warehouse warehouse;

#endif
