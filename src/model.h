#ifndef model_h
#define model_h

#include "element.h"

#define MAX_FUSELAGES   32

#define DRAW_CORRS      0 /* just for debugging */


struct vec3;
struct vec4;
struct Wing;
struct Arena;
struct Serial;
struct Object;
struct PickResult;
struct mat4_stack;
struct ShaderProgram;

enum SkinVertColorSource {
    NO_SOURCE, VX, VY, VZ, V
};

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

    /* drawing */
    int *skin_trias;
    int skin_trias_count;
    int *skin_quads;
    int skin_quads_count;
    float *skin_verts_values;

#if DRAW_CORRS
    vec3 *corr_verts;
    vec3 *corr_colors;
    int corrs_count;
#endif

    Panel *panels;
    int panels_count;

    Model();
    ~Model();

    void deselect_all();
    bool move_selected(vec3 move_xyz, vec3 target_yz);
    bool delete_selected();

    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result);
    void draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos);
    void draw_skin_triangles(ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result);
    void draw_skin_outlines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos);
    void draw_lines(ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result);
#if DRAW_CORRS
    void draw_corrs(ShaderProgram &program);
#endif
    void pick(ShaderProgram &program, mat4_stack &mv_stack);
    void *decode_pick_result(PickResult &result);

    bool maybe_drag_selection(PickResult &pick_result, bool ctrl_pressed);
};

void model_clear(Model *m);
void model_add_object(Model *m, Object *o);
void model_add_wing(Model *m, Wing *w);

void model_serialize(Model *model, const char *path);
void model_deserialize(Model *model, const char *path);
void model_dump_mesh(Model *model, const char *path);

void model_init_ochre_state();
void init_model_draw();

void model_update_object_mantles(Model *model);

void model_update_skin_verts_values(Model *model, SkinVertColorSource source);

bool model_collide(Model *model, Arena *arena, bool dragging);

void model_loft(Arena *arena, Model *model);

#endif
