#include "warehouse.h"
#include "mantle.h"
#include "arena.h"
#include "graphics.h"
#include "mat.h"


Mantle mantle;

Arena warehouse_arena(100000);

void Warehouse::update_drawing_geometry() {
    if (!is_open)
        return;

    Part &part = parts[selected_part];

    warehouse_arena.clear();

    mantle.generate_from_former_array(warehouse_arena, part.formers.data, part.formers.count, vec3(0, 0, 0));
}

void Warehouse::draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir) {
    if (!is_open)
        return;

    mv_stack.push();
    mv_stack.translate(camera_pos + camera_dir * 10); // TODO: adapt distance to part size
    program.set_uniform_mat4(1, mv_stack.top());

    mantle.draw_triangles(program, mv_stack, vec4(0.45, 0.65, 1, 1));

    mv_stack.pop();
    program.set_uniform_mat4(1, mv_stack.top());
}

void Warehouse::draw_outlines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir) {
    if (!is_open)
        return;

    mv_stack.push();
    mv_stack.translate(camera_pos + camera_dir * 10); // TODO: adapt distance to part size
    program.set_uniform_mat4(1, mv_stack.top());

    mantle.draw_outlines(program, mv_stack, vec4(0.0, 0.0, 0.0, 1), camera_pos - camera_dir);

    mv_stack.pop();
    program.set_uniform_mat4(1, mv_stack.top());
}
