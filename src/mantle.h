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
    void update_data(); // TODO: make this private

    void generate_from_former_array(Arena &arena, Former *formers, int formers_count, vec3 obj_p);
    void generate_object_model(Arena &arena, Object *obj);

    void draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color);
    void draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color, const vec3 &camera_pos);
};

Arena &mantle_arena();
void mantle_clear_arena();

void mantle_update_storage(Mantle *mantle, Arena *arena,
                           int sections_count, int verts_per_section);
void mantle_generate_from_airfoil(Mantle *mantle, Arena *arena, Airfoil *airfoil,
                                  float x, float y, float z);

#endif
