#include "object.h"
#include "graphics.h"
#include "pick.h"
#include "mat.h"


vec4 _get_handle_color(Handle *handle, void *hovered_pickable) {
    if (handle->selected) {
        if (handle == hovered_pickable)
            return vec4(0.8, 0.7, 0.0, 1);
        else
            return vec4(1.0, 0.9, 0.0, 1);
    }
    else {
        if (handle == hovered_pickable)
            return vec4(0.25, 0.7, 0.0, 1); // vec4(0.6, 0.6, 0.6, 1);
        else
            return vec4(0.4, 0.9, 0.0, 1); // vec4(0.8, 0.8, 0.8, 1);
    }
}

vec4 _get_object_color(Object *object, void *hovered_pickable) {
    if (object->selected) {
        if (object == hovered_pickable)
            return vec4(0.8, 0.7, 0.0, 1);
        else
            return vec4(1.0, 0.9, 0.0, 1);
    }
    else {
        if (object == hovered_pickable)
            return vec4(0.25, 0.7, 0.0, 1.0); // vec4(0.6, 0.6, 0.6, 1.0);
        else
            return vec4(0.4, 0.9, 0.0, 1.0); // vec4(0.8, 0.8, 0.8, 1.0);
    }
}

void Object::draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, void *hovered_pickable) {
    vec4 color = _get_object_color(this, hovered_pickable);
    model_mantle.draw_triangles(program, mv_stack, color);
}

void Object::draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos) {
    model_mantle.draw_outlines(program, mv_stack, vec4(0.2, 0.2, 0.2, 1), camera_pos);
}

int cube_triangle_indices[] = {
    0, 1, 2,
    0, 2, 3,
    5, 4, 7,
    5, 7, 6,
    4, 0, 3,
    4, 3, 7,
    1, 5, 6,
    1, 6, 2,
    3, 2, 6,
    3, 6, 7,
    0, 5, 1,
    0, 4, 5,
};

void Object::draw_handles(ShaderProgram &program, mat4_stack &mv_stack, void *hovered_pickable) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < SHAPE_CURVES; ++j) {
            Handle &handle = handles[i][j];
            program.set_uniform_vec4(2, _get_handle_color(&handle, hovered_pickable));
            program.set_data<vec3>(0, HANDLE_VERTS, handles[i][j].verts);
            graph_draw_triangles_indexed(36, cube_triangle_indices);
        }
    }
}

void Object::pick_handles(ShaderProgram &program, mat4_stack &mv_stack, unsigned pick_category, int object_index) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < SHAPE_CURVES; ++j) {
            program.set_uniform_vec4(2, pick_encode(pick_category, object_index, i, j));
            program.set_data<vec3>(0, HANDLE_VERTS, handles[i][j].verts);
            graph_draw_triangles_indexed(36, cube_triangle_indices);
        }
    }
}
