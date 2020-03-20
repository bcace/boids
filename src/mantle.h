#ifndef mantle_h
#define mantle_h

#define DRAW_VERTS_PER_POINT    16
#define DRAW_VERTS_PER_POLY     (4 * DRAW_VERTS_PER_POINT)


struct vec3;
struct vec4;
struct Arena;
struct Former;
struct Object;
struct Airfoil;
struct SkinFormer;
struct mat4_stack;
struct ShaderProgram;

struct Mantle {
    int sections_count;
    int verts_per_section;
    int verts_count;
    int indices_count;

    vec3 *verts;
    int *indices;
    bool draw_caps;

    Mantle();

    void update_data(); // TODO: make this private
    void generate_from_former_array(Arena &arena, Former *formers, int formers_count, vec3 obj_p);
    void generate_object_model(Arena &arena, Object *obj);
};

Arena &mantle_arena();
void mantle_clear_arena();

void mantle_draw_quads(Mantle *m, ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color);
void mantle_generate_from_airfoil(Mantle *m, Arena *arena, Airfoil *airfoil,
                                  float x, float y, float z);

#endif
