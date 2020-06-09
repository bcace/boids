#ifndef model_h
#define model_h

#include "modeling_element.h"

#define MAX_FUSELAGES   32


struct vec3;
struct Wing;
struct Arena;
struct Object;

struct Panel {
    int v1, v2, v3, v4; /* vertex indices */
    int prev, next, tail, nose; /* neighbor panel indices */
    float vx, vy, vz; /* airspeed components */
};

struct Model {
    Object *objects[MAX_ELEMS];
    int objects_count;
    Wing *wings[MAX_ELEMS];
    int wings_count;

    vec3 *skin_verts;
    int skin_verts_count;
    Panel *panels;
    int panels_count;

#if DRAW_CORRS
    vec3 *corr_verts;
    vec3 *corr_colors;
    int corrs_count;
#endif

    Model();
    ~Model();
};

void model_clear(Model *m);
void model_add_object(Model *m, Object *o);
void model_add_wing(Model *m, Wing *w);
void model_deselect_all(Model *m);
bool model_move_selected(Model *m, vec3 move_xyz, vec3 target_yz);
bool model_delete_selected(Model *m);

void model_serial_dump(Model *model, const char *path);
void model_serial_load(Model *model, const char *path);
void model_serial_dump_mesh(Model *model, const char *path);

void model_collision_init();
bool model_collision_run(Model *model, Arena *arena, bool dragging);
void model_loft(Arena *arena, Model *model);

#ifdef NDEBUG
    #define model_assert(__model__, __expr__, __label__) ((void)0)
#else
    void _model_assert_func(struct Model *model, bool expr, const char *label);

    #define model_assert(__model__, __expr__, __label__) _model_assert_func((__model__), (__expr__), (__label__))
#endif

#endif
