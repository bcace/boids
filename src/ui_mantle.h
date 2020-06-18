#ifndef mantle_h
#define mantle_h


struct vec3;
struct vec4;
struct Arena;
struct Former;
struct WFormer;
struct Airfoil;
struct mat4_stack;
struct ShaderProgram;

struct Mantle {
    int sections_count;
    int verts_per_section;
    int verts_count;
    int indices_count;

    vec3 *verts;
    int *indices;

    Mantle();
};

Arena &mantle_arena();
void mantle_clear_arena();

void mantle_generate_from_former_array(Mantle *m, Arena *arena, Former *formers, int formers_count, float x, float y, float z);
void mantle_generate_from_wing_formers(Mantle *m, Arena *arena, WFormer *r_former, WFormer *t_former, float x, float y, float z);
void mantle_draw_quads(Mantle *m, ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color);

#endif
