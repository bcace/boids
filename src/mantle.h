#ifndef mantle_h
#define mantle_h

#include "shape.h"

#define DRAW_VERTS_PER_POINT    16
#define DRAW_VERTS_PER_POLY     (SHAPE_CURVES * DRAW_VERTS_PER_POINT)


struct vec3;
struct vec4;
struct Arena;
struct Object;
struct SkinFormer;
struct mat4_stack;
struct ShaderProgram;

struct Mantle {
    int sections_count;
    int tri_verts_count;
    int tri_indices_count;
    int out_verts_count;

    vec3 *tri_verts;
    int *tri_indices;
    vec3 *out_verts;
    vec3 *out_norms1;
    vec3 *out_norms2;

    Mantle();

    int wrap(int i, int j); // TODO: make this private
    void update_storage(Arena &arena, int _sections_count); // TODO: make this private
    void update_data(); // TODO: make this private

    void generate_from_former_array(Arena &arena, Former *formers, int formers_count, vec3 obj_p);
    void generate_from_skin_formers(Arena &arena, SkinFormer *tail_skin_former, vec3 tail_obj_p, SkinFormer *nose_skin_former, vec3 nose_obj_p);
    void generate_object_model(Arena &arena, Object *obj);
    void generate_object_skin(Arena &arena, Object *obj);

    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color);
    void draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color, const vec3 &camera_pos);
};

Arena &mantle_arena();
void mantle_clear_arena();

#endif
