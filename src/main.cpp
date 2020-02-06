#include "model.h"
#include "object.h"
#include "warehouse.h"
#include "loft.h"
#include "mesh.h"
#include "platform.h"
#include "graphics.h"
#include "camera.h"
#include "pick.h"
#include "drag.h"
#include "serial.h"
#include "vector.h"
#include "shape.h"
#define VEC_IMPL
#include "vec.h"
#define MAT_IMPL
#include "mat.h"
#define INTERP_IMPL
#include "interp.h"
#include "ochre.h"
#include "arena.h"
#include "apame.h"
#include <assert.h>
#include <stdio.h>


ShaderProgram program, outline_program, shaded_program, valued_program, colored_program;
mat4 projection;
mat4_stack mv_stack;

Camera camera(1.0, 1000.0, 0.8);
Drag drag;

Model model;
Serial serial;
PickResult pick_result;

vec3 crosshair_verts[4];

Arena arena(1000000);


void _recalculate_model() {
    mantle_clear_arena();
    model_update_object_mantles(&model);
    loft_model(&arena, &model);
#ifdef BOIDS_USE_APAME
    boids_apame_run(&model);
    model_update_skin_verts_values(&model, VX);
#else
    model_update_skin_verts_values(&model, NO_SOURCE);
#endif
}

void _mousebutton_callback(int button, int action, int mods) {
    if (button == PLATFORM_LEFT) {
        if (action == PLATFORM_PRESS) {
            if (warehouse.is_open) {
                model.add_object(warehouse.make_selected_part(camera.pos, camera.dir));
                _recalculate_model();
            }
            if (model.maybe_drag_selection(pick_result, (mods & PLATFORM_MOD_CTRL) != 0))
                drag.begin(camera.pos, camera.dir, pick_result.depth);
        }
        else
            drag.end();
    }
    else if (button == PLATFORM_RIGHT) {
        if (action == PLATFORM_PRESS) {
            if (!warehouse.is_open)
                warehouse.open();
        }
    }
}

void _mousepos_callback(float x, float y) {
    camera.on_mousepos(x, y);
}

void _scroll_callback(float x, float y) {
    if (warehouse.is_open) {
        if (y > 0)
            warehouse.select_next_part();
        else
            warehouse.select_prev_part();
    }
    else
        camera.on_scroll(x, y);
}

void _key_callback(int key, int mods, int action) {
    if (mods & PLATFORM_MOD_CTRL) {
        if (action == PLATFORM_RELEASE) {
            if (key == PLATFORM_KEY_S)
                model_serialize(&model, "model_dump");
            else if (key == PLATFORM_KEY_L) {
                model_deserialize(&model, "cannot_find_divider_in_mesh_pass_5");
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_D)
                model_dump_mesh(&model, "model_mesh_dump");
        }
    }
    else {
        if (action == PLATFORM_RELEASE) {
            if (key == PLATFORM_KEY_ESCAPE)
                warehouse.close();
            else if (key == PLATFORM_KEY_DELETE) {
                if (model.delete_selected()) {
                    _recalculate_model();
                    pick_result.clear();
                }
            }
            else if (key == PLATFORM_KEY_O) {
                config_decrease_shape_samples();
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_P) {
                config_increase_shape_samples();
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_K) {
                config_decrease_structural_margin();
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_L) {
                config_increase_structural_margin();
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_N) {
                config_decrease_mesh_triangle_edge_transparency();
            }
            else if (key == PLATFORM_KEY_M) {
                config_increase_mesh_triangle_edge_transparency();
            }
            else if (key == PLATFORM_KEY_B) {
                mesh_verts_merge_margin(true);
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_V) {
                mesh_verts_merge_margin(false);
                _recalculate_model();
            }
            else if (key == PLATFORM_KEY_R) {
                _recalculate_model();
            }
        }

        camera.on_keys(key, action);
    }
}

void _main_loop_func() {
    vec2 screen = plat_get_screen();

    // setup viewport

    graph_viewport(0, 0, screen.x, screen.y);

    // setup matrices

    camera.update();

    mat4 perspective, lookat;
    graph_perspective(perspective, camera.fov, screen.x / screen.y, camera.near, camera.far);
    graph_lookat(lookat, camera.pos, camera.dir, vec3(0, 0, 1));

    projection = perspective * lookat;

    mv_stack.set_identity();

    // setup shader program

    program.use();
    program.set_uniform_mat4(0, projection);
    graph_enable_depth_test(true);

    // move objects

    bool reloft = false;

    if (drag.dragging && camera.moved) {
        vec3 move = drag.drag(camera.pos, camera.dir);
        if (model.move_selected(move, drag.target_yz))
            reloft = true;
    }

    // step colliders

    if (model.collide(&arena, drag.dragging))
        reloft = true;

    // reloft fuselages

    if (reloft)
        _recalculate_model();

    // pick objects

    if (!drag.dragging && camera.moved) {
        graph_clear(vec3(1, 1, 1));
        model.pick(program, mv_stack);
        pick(5, screen.x, screen.y, camera.near, camera.far, pick_result, true);
    }

    // drawing

    {
        // draw fuselage skin

        graph_clear(vec3(0.9, 0.9, 0.9));

        valued_program.use();
        valued_program.set_uniform_mat4(0, projection);
        valued_program.set_uniform_mat4(1, mv_stack.top());

        model.draw_skin_triangles(valued_program, mv_stack, pick_result);

#if DRAW_CORRS
        colored_program.use();
        colored_program.set_uniform_mat4(0, projection);
        colored_program.set_uniform_mat4(1, mv_stack.top());

        model.draw_corrs(colored_program);
#endif

        program.use();
        program.set_uniform_mat4(0, projection);
        program.set_uniform_mat4(1, mv_stack.top());

        model.draw_lines(program, mv_stack, pick_result);

        // graph_clear_depth();

        // draw objects

        shaded_program.use();
        shaded_program.set_uniform_mat4(0, projection);
        shaded_program.set_uniform_mat4(1, mv_stack.top());
        model.draw_triangles(shaded_program, mv_stack, pick_result);
        warehouse.draw_triangles(shaded_program, mv_stack, camera.pos, camera.dir);

        outline_program.use();
        outline_program.set_uniform_mat4(0, projection);
        outline_program.set_uniform_mat4(1, mv_stack.top());

        model.draw_outlines(outline_program, mv_stack, camera.pos - camera.dir);
        // model.draw_skin_outlines(outline_program, mv_stack, camera.pos - camera.dir);
        warehouse.draw_outlines(outline_program, mv_stack, camera.pos, camera.dir);

        // draw headup

        program.use();

        graph_enable_depth_test(false);
        graph_line_width(2);

        mat4 headup_projection;
        graph_ortho(headup_projection, 0, screen.x, 0, screen.y, -100, 100);

        mat4 headup_modelview;
        headup_modelview.set_identity();

        program.set_uniform_mat4(0, headup_projection);
        program.set_uniform_mat4(1, headup_modelview);

        program.set_uniform_vec4(2, vec4(0, 0, 0, 1));
        program.set_data<vec3>(0, 4, crosshair_verts);
        graph_draw_lines(4);

        plat_swap_buffers();
    }

    plat_sleep(10);
    plat_poll_events();
}

int main() {
    model_init_ochre_state();

    // create window

    plat_create_window("Boids", 1800, 1000, false);

    program.init("src/glsl/default.vert", "src/glsl/default.frag");
    program.define_in_float(3); // position
    program.define_uniform("projection");
    program.define_uniform("modelview");
    program.define_uniform("color");

#if DRAW_CORRS
    colored_program.init("src/glsl/colored.vert", "src/glsl/colored.frag");
    colored_program.define_in_float(3); // position
    colored_program.define_in_float(3); // color
    colored_program.define_uniform("projection");
    colored_program.define_uniform("modelview");
#endif

    shaded_program.init("src/glsl/shaded.vert", "src/glsl/shaded.frag");
    shaded_program.define_in_float(3); // position
    shaded_program.define_uniform("projection");
    shaded_program.define_uniform("modelview");
    shaded_program.define_uniform("color");

    valued_program.init("src/glsl/valued.vert", "src/glsl/valued.frag");
    valued_program.define_in_float(3); // position
    valued_program.define_in_float(1); // value
    valued_program.define_uniform("projection");
    valued_program.define_uniform("modelview");

    outline_program.init("src/glsl/outline.vert", "src/glsl/default.frag");
    outline_program.define_in_float(3); // position
    outline_program.define_in_float(3); // norm1
    outline_program.define_in_float(3); // norm2
    outline_program.define_uniform("projection");
    outline_program.define_uniform("modelview");
    outline_program.define_uniform("color");
    outline_program.define_uniform("camera_pos");

    plat_set_cursor_hidden(true);
    plat_set_key_callback(_key_callback);
    plat_set_mousepos_callback(_mousepos_callback);
    plat_set_scroll_callback(_scroll_callback);
    plat_set_mousebutton_callback(_mousebutton_callback);

    vec2 screen = plat_get_screen();
    crosshair_verts[0] = vec3(screen.x * 0.5 - 10.0, screen.y * 0.5, 0.0);
    crosshair_verts[1] = vec3(screen.x * 0.5 + 10.0, screen.y * 0.5, 0.0);
    crosshair_verts[2] = vec3(screen.x * 0.5, screen.y * 0.5 - 10.0, 0.0);
    crosshair_verts[3] = vec3(screen.x * 0.5, screen.y * 0.5 + 10.0, 0.0);

    init_model_draw();

    // run window

    plat_run_window(_main_loop_func);

    return 0;
}
