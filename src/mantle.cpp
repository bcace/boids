#include "mantle.h"
#include "graphics.h"
#include "arena.h"
#include "vec.h"


Mantle::Mantle() : verts(0), verts_count(0),
                   indices(0), indices_count(0),
                   sections_count(0), verts_per_section(0),
                   draw_caps(false) {}

void mantle_draw_quads(Mantle *m, ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color) {
    program.set_uniform_vec4(2, color);
    program.set_data<vec3>(0, m->verts_count, m->verts);
    graph_draw_quads_indexed(m->indices_count, m->indices);
}
