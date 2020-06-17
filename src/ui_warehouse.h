#ifndef warehouse_h
#define warehouse_h

#include "modeling_object.h"
#include "modeling_wing.h"

#define MAX_PARTS               64
#define MAX_WPROS               64
#define MAX_PROTO_NAME          128
#define MAX_PROTO_FORMERS       8


struct vec3;
struct mat4_stack;
struct ShaderProgram;

struct OProto {
    char name[MAX_PROTO_NAME];
    ObjectDef def;
};

struct WProto {
    char name[MAX_PROTO_NAME];
    WingDef def;
};

struct Warehouse {
    OProto o_protos[MAX_PARTS];
    int o_protos_count;
    WProto w_protos[MAX_WPROS];
    int w_protos_count;
    int selected_proto; /* object or wing index, depening on is_wing_selected */
    bool is_object;
    bool is_open;

    Warehouse();

    void open();
    void close();
    void select_next_part();
    void select_prev_part();

    void update_drawing_geometry();
    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir);
    void draw_lines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir);
};

Object *warehouse_make_selected_object(vec3 camera_pos, vec3 camera_dir);

// Wing *warehouse_make_selected_wing(Warehouse *wh, vec3 camera_pos, vec3 camera_dir);

extern Warehouse warehouse;

#endif
