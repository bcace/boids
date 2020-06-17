#include "ui_warehouse.h"
#include "ui_graphics.h"
#include "ui_mantle.h"
#include "modeling_config.h"
#include "memory_arena.h"
#include "math_mat.h"


Mantle mantle;

Arena warehouse_arena(100000);

void Warehouse::update_drawing_geometry() {
    if (is_object) {
        OProto *proto = o_protos + selected_proto;
        warehouse_arena.clear();
        mantle_generate_from_former_array(&mantle, &warehouse_arena, proto->def.formers, proto->def.formers_count, 0.0f, 0.0f, 0.0f);
    }
    else {
        Airfoil *airfoil = airfoils_base + selected_proto;
        warehouse_arena.clear();
        mantle_generate_from_airfoil(&mantle, &warehouse_arena, airfoil, 0.0f, 0.0f, 0.0f);
    }
}

void Warehouse::draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir) {
    if (!is_open)
        return;

    mv_stack.push();
    mv_stack.translate(camera_pos + camera_dir * 10); // TODO: adapt distance to part size
    program.set_uniform_mat4(1, mv_stack.top());

    mantle_draw_quads(&mantle, program, mv_stack, vec4(0.45f, 0.65f, 1.0f, 1.0f));

    mv_stack.pop();
    program.set_uniform_mat4(1, mv_stack.top());
}

void Warehouse::draw_lines(ShaderProgram &program, mat4_stack &mv_stack, vec3 camera_pos, vec3 camera_dir) {
    if (!is_open)
        return;

    mv_stack.push();
    mv_stack.translate(camera_pos + camera_dir * 10); // TODO: adapt distance to part size
    program.set_uniform_mat4(1, mv_stack.top());

    graph_set_polygon_line_mode(true);
    mantle_draw_quads(&mantle, program, mv_stack, vec4(0.0f, 0.0f, 0.0f, MESH_ALPHA));
    graph_set_polygon_line_mode(false);

    mv_stack.pop();
    program.set_uniform_mat4(1, mv_stack.top());
}
