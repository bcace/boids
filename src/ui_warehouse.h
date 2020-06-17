#ifndef warehouse_h
#define warehouse_h

#include "modeling_object.h"
#include "modeling_airfoil.h"

#define MAX_PARTS               64
#define MAX_WPROS               64
#define MAX_PROTO_NAME          128
#define MAX_PROTO_FORMERS       8


struct vec3;
struct Wing;
struct mat4_stack;
struct ShaderProgram;

struct ObjectProto {
    char name[MAX_PROTO_NAME];
    ObjectDef def;
};

void object_proto_init(ObjectProto *part, const char *name, float tail_endp_dx, float nose_endp_dx);
void object_proto_add_coll_former(ObjectProto *part, Shape shape, float x);
void object_proto_set_skin_formers(ObjectProto *part, Shape tail_shape, float tail_x, Shape nose_shape, float nose_x);
Object *object_proto_make_object(ObjectProto *part, vec3 p);

struct WProto {
    Airfoil airfoil;
};

enum WarehouseMode { WM_CLOSED, WM_OBJECT, WM_WING };

struct Warehouse {
    ObjectProto parts[MAX_PARTS];
    int parts_count;
    int selected_part;
    WProto wpros[MAX_WPROS];
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

// Wing *warehouse_make_selected_wing(Warehouse *wh, vec3 camera_pos, vec3 camera_dir);

extern Warehouse warehouse;

#endif
