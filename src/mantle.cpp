#include "mantle.h"
#include "graphics.h"
#include "arena.h"
#include "vec.h"


Mantle::Mantle() : tri_verts(0), tri_indices(0), out_verts(0), out_norms1(0), out_norms2(0),
                   tri_verts_count(0), tri_indices_count(0), out_verts_count(0) {}

void mantle_update_storage(Mantle *mantle, Arena *arena, int sections_count, int verts_per_section) {
    mantle->sections_count = sections_count;
    mantle->verts_per_section = verts_per_section;

    mantle->tri_verts_count = sections_count * verts_per_section;
    mantle->tri_indices_count = (sections_count - 1) * verts_per_section * 2 * 3;
    mantle->out_verts_count = ((sections_count * 2 - 1) * verts_per_section * 2);

    mantle->tri_verts = arena->alloc<vec3>(mantle->tri_verts_count);
    mantle->tri_indices = arena->alloc<int>(mantle->tri_indices_count);
    mantle->out_verts = arena->alloc<vec3>(mantle->out_verts_count);
    mantle->out_norms1 = arena->alloc<vec3>(mantle->out_verts_count);
    mantle->out_norms2 = arena->alloc<vec3>(mantle->out_verts_count);
}

inline int Mantle::wrap(int i, int j) {
    if (j < 0)
        j += verts_per_section;
    if (i < 0)
        i = 0;
    if (i >= sections_count)
        i = sections_count - 1;
    return i * verts_per_section + (j % verts_per_section);
}

void Mantle::update_data() {

    /* triangle indices */
    int *tri_index = tri_indices;

    for (int i = 1; i < sections_count; ++i) {
        for (int j = 0; j < verts_per_section; ++j) {
            *tri_index++ = wrap(i - 1, j);
            *tri_index++ = wrap(i, j);
            *tri_index++ = wrap(i, j + 1);

            *tri_index++ = wrap(i - 1, j);
            *tri_index++ = wrap(i, j + 1);
            *tri_index++ = wrap(i - 1, j + 1);
        }
    }

    /* outlines */
    vec3 *out_vert = out_verts;
    vec3 *out_norm1 = out_norms1;
    vec3 *out_norm2 = out_norms2;

    for (int i = 0; i < sections_count; ++i) {
        for (int j = 0; j < verts_per_section; ++j) {
            vec3 curr = tri_verts[wrap(i, j)];
            vec3 tail = tri_verts[wrap(i - 1, j)];
            vec3 nose = tri_verts[wrap(i + 1, j)];
            vec3 next = tri_verts[wrap(i, j + 1)];
            vec3 tail_next = tri_verts[wrap(i - 1, j + 1)];
            vec3 tail_prev = tri_verts[wrap(i - 1, j - 1)];
            vec3 curr_next = tri_verts[wrap(i, j + 1)];
            vec3 curr_prev = tri_verts[wrap(i, j - 1)];
            vec3 nose_next = tri_verts[wrap(i + 1, j + 1)];
            vec3 curr_tail = curr - tail;
            vec3 next_curr = next - curr;

            /* longitudinal */
            if (i > 0) {
                *out_vert++ = tail;
                *out_norm1++ = curr_tail.cross(tail_next - tail);
                *out_norm2++ = (tail_prev - tail).cross(curr_tail);

                *out_vert++ = curr;
                *out_norm1++ = curr_tail.cross(curr_next - curr);
                *out_norm2++ = (curr_prev - curr).cross(curr_tail);
            }

            /* transversal */
            {
                *out_vert++ = curr;
                if (i == 0)
                    *out_norm1++ = -next_curr.cross(tail - curr); // vec3(1, 0, 0);
                else
                    *out_norm1++ = next_curr.cross(tail - curr);
                if (i == sections_count - 1)
                    *out_norm2++ = -(nose - curr).cross(next_curr); // vec3(-1, 0, 0);
                else
                    *out_norm2++ = (nose - curr).cross(next_curr);

                *out_vert++ = next;
                if (i == 0)
                    *out_norm1++ = -next_curr.cross(tail_next - next); // vec3(1, 0, 0);
                else
                    *out_norm1++ = next_curr.cross(tail_next - next);
                if (i == sections_count - 1)
                    *out_norm2++ = -(nose_next - next).cross(next_curr); // vec3(-1, 0, 0);
                else
                    *out_norm2++ = (nose_next - next).cross(next_curr);
            }
        }
    }
}

void Mantle::draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color) {
    program.set_uniform_vec4(2, color);
    program.set_data<vec3>(0, tri_verts_count, tri_verts);
    graph_draw_triangles_indexed(tri_indices_count, tri_indices);
}

void Mantle::draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, const vec4 &color, const vec3 &camera_pos) {
    graph_enable_depth_mask(false);

    program.set_uniform_vec4(2, color);
    program.set_uniform_vec3(3, camera_pos);

    program.set_data<vec3>(0, out_verts_count, out_verts);
    program.set_data<vec3>(1, out_verts_count, out_norms1);
    program.set_data<vec3>(2, out_verts_count, out_norms2);
    graph_draw_lines(out_verts_count, 0);

    graph_enable_depth_mask(true);
}
