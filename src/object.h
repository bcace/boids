#ifndef object_h
#define object_h

#include "ref.h"
#include "vec.h"
#include "shape.h"
#include "mantle.h"
#include "constants.h"

#define MAX_OBJECT_FORMERS      8
#define MAX_SKIN_FORMERS        4

#define COLL_VERTS_PER_CURVE    4
#define COLL_VERTS_PER_PRISM    (SHAPE_CURVES * COLL_VERTS_PER_CURVE)

#define HANDLE_VERTS            8
#define HANDLE_SIZE             0.02f


struct Arena;
struct CollPrism;
struct mat4_stack;
struct ShaderProgram;

enum SkinFormerConstraints {
    sfcNone,
    sfcBothSymmetric,
    sfcVertSymmetric,
};

struct HandleConstraints {
    int symm_x, symm_y; /* index of handle that this point is symmetric with wrt specified axis */
    bool move_x, move_y; /* whether a point should move along specified axis */
};

struct Handle {
    vec3 verts[HANDLE_VERTS]; /* model CS */
    bool selected;
};

struct Object {
    /* serialize */
    vec3 p;
    Former formers[MAX_OBJECT_FORMERS]; /* collision formers */
    int formers_count; /* collision formers */
    Former tail_skin_former, nose_skin_former; /* skin formers, object CS */
    float tail_endp_dx, nose_endp_dx;

    /* collision */
    vec3 f;
    CollPrism *prisms; /* allocated in arena */
    int prisms_count;
    box2 bounds; /* y-z bounds */
    float min_x, max_x; /* model CS */
    float min_y, max_y; /* model CS, collision formers */
    float min_z, max_z; /* model CS, collision formers */
    float coll_min_x, coll_max_x; /* with refs */

    /* control */
    bool selected;
    vec3 drag_p;

    /* drawing */
    Mantle model_mantle;
    Handle handles[2][SHAPE_CURVES]; /* [tail/nose][index] */

    void move(vec3 dp);
    void reset_drag_p();
    void deselect_all_handles();

    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, void *hovered_pickable);
    void draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos);
    void draw_handles(ShaderProgram &program, mat4_stack &mv_stack, void *hovered_pickable);
    void pick_handles(ShaderProgram &program, mat4_stack &mv_stack, unsigned pick_category, int object_index);
};

void object_finish(Object *o);
void object_update_extents(Object *o);

bool object_should_be_centered(Object *o);
bool object_should_be_mirrored(Object *o);

void object_clear_dynamic_arena();

void object_update_mantle(Object *o);

#endif
