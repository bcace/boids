#ifndef warehouse_h
#define warehouse_h

#include "modeling_shape.h"
#include "modeling_airfoil.h"
#include "math_vec.h"

#define MAX_PARTS               64
#define MAX_WPROS               64
#define MAX_PROTO_NAME          128
#define MAX_PROTO_FORMERS       8


struct Wing;
struct Object;
struct mat4_stack;
struct ShaderProgram;

struct Part {
    char name[MAX_PROTO_NAME];

    /* data copied onto objects */
    // TODO: think about making this a separate structure, shared between Object and Part
    // TODO: don't use array here
    Former formers[MAX_PROTO_FORMERS]; /* initial collision formers */
    int formers_count;
    Former tail_skin_former, nose_skin_former;
    float tail_endp_dx, nose_endp_dx; /* endpoint distances from respective skin formers */

    bool tail_opening, nose_opening;
};

void part_init(Part *part, const char *name, float tail_endp_dx, float nose_endp_dx, bool tail_opening, bool nose_opening);
void part_add_coll_former(Part *part, Shape shape, float x);
void part_set_skin_formers(Part *part, Shape tail_shape, float tail_x, Shape nose_shape, float nose_x);
Object *part_make_object(Part *part, vec3 p);


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
