#include "ui_window.h"
#include "ui_model.h"
#include "ui_graphics.h"
#include "ui_camera.h"
#include "ui_pick.h"
#include "ui_drag.h"
#include "modeling_object.h"
#include "modeling_warehouse.h"
#include "modeling_shape.h"
#include "modeling_ochre.h"
#include "modeling_airfoil.h"
#include "modeling_config.h"
#include "memory_arena.h"
#include "math_vec.h"
#include "math_mat.h"
#include "proc_apame.h"
#include "platform.h"
#include "serial.h"
#include <assert.h>
#include <stdio.h>


ShaderProgram program, shaded_program, valued_program, colored_program;
mat4 projection;
mat4_stack mv_stack;

Camera camera(1.0, 1000.0, 0.8);
Drag drag;

UiModel ui_model;
Serial serial;
PickResult pick_result;

vec3 crosshair_verts[4];

Arena arena(20000000);


void _recalculate_model() {
    model_loft(&arena, &ui_model.model);
    ui_model_update_mantles(&ui_model);
    SkinVertColorSource source = NO_SOURCE;
#ifdef BOIDS_USE_APAME
    boids_apame_run(&ui_model.model);
    source = VX;
#endif
    ui_model_update_skin(&ui_model, source);
}

void _mousebutton_callback(int button, int action, int mods) {
    if (button == WINDOW_LEFT) {
        if (action == WINDOW_PRESS) {
            if (warehouse.mode == WM_OBJECT) {
                model_add_object(&ui_model.model, warehouse.make_selected_part(camera.pos, camera.dir));
                _recalculate_model();
            }
            else if (warehouse.mode == WM_WING) {
                model_add_wing(&ui_model.model, warehouse_make_selected_wing(&warehouse, camera.pos, camera.dir));
                _recalculate_model();
            }
            if (ui_model_maybe_drag_selection(&ui_model, &pick_result, (mods & WINDOW_MOD_CTRL) != 0))
                drag.begin(camera.pos, camera.dir, pick_result.depth);
        }
        else
            drag.end();
    }
    else if (button == WINDOW_RIGHT) {
        if (action == WINDOW_PRESS) {
            if (!warehouse.mode == WM_OBJECT)
                warehouse.open(mods & WINDOW_MOD_CTRL);
        }
    }
}

void _mousepos_callback(float x, float y) {
    camera.on_mousepos(x, y);
}

void _scroll_callback(float x, float y) {
    if (warehouse.mode != WM_CLOSED) {
        if (y > 0)
            warehouse.select_next_part();
        else
            warehouse.select_prev_part();
    }
    else
        camera.on_scroll(x, y);
}

void _key_callback(int key, int mods, int action) {
    if (mods & WINDOW_MOD_CTRL) {
        if (action == WINDOW_RELEASE) {
            if (key == WINDOW_KEY_S)
                model_serial_dump(&ui_model.model, "model.dump");
            else if (key == WINDOW_KEY_L) {
                model_serial_load(&ui_model.model, "model.dump");
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_D)
                model_serial_dump_mesh(&ui_model.model, "model.mesh_dump");
        }
    }
    else {
        if (action == WINDOW_RELEASE) {
            if (key == WINDOW_KEY_ESCAPE)
                warehouse.close();
            else if (key == WINDOW_KEY_DELETE) {
                if (model_delete_selected(&ui_model.model)) {
                    _recalculate_model();
                    pick_result.clear();
                }
            }
            else if (key == WINDOW_KEY_O) {
                config_decrease_shape_samples();
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_P) {
                config_increase_shape_samples();
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_K) {
                config_decrease_structural_margin();
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_L) {
                config_increase_structural_margin();
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_N) {
                config_decrease_mesh_triangle_edge_transparency();
            }
            else if (key == WINDOW_KEY_M) {
                config_increase_mesh_triangle_edge_transparency();
            }
            else if (key == WINDOW_KEY_B) {
                mesh_verts_merge_margin(true);
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_V) {
                mesh_verts_merge_margin(false);
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_H) {
                config_decrease_merge_interpolation_delay();
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_J) {
                config_increase_merge_interpolation_delay();
                _recalculate_model();
            }
            else if (key == WINDOW_KEY_R) {
                _recalculate_model();
            }
        }

        camera.on_keys(key, action);
    }
}

void _main_loop_func() {

    /* setup viewport */

    graph_viewport(0, 0, window.w, window.h);

    /* setup matrices */

    camera.update();

    mat4 perspective, lookat;
    graph_perspective(perspective, camera.fov, window.w / window.h, camera.near, camera.far);
    graph_lookat(lookat, camera.pos, camera.dir, vec3(0, 0, 1));

    projection = perspective * lookat;

    mv_stack.set_identity();

    /* setup shader program */

    program.use();
    program.set_uniform_mat4(0, projection);
    graph_enable_depth_test(true);

    /* move objects */

    bool reloft = false;

    if (drag.dragging && camera.moved) {
        vec3 move = drag.drag(camera.pos, camera.dir);
        if (model_move_selected(&ui_model.model, move, drag.target_yz))
            reloft = true;
    }

    /* step colliders */

    if (model_collision_run(&ui_model.model, &arena, drag.dragging))
        reloft = true;

    /* reloft fuselages */

    if (reloft)
        _recalculate_model();

    /* pick objects */

    if (!drag.dragging && camera.moved) {
        graph_clear(vec3(1, 1, 1));
        ui_model_draw_for_picking(&ui_model, program, mv_stack);
        pick(5, window.w, window.h, camera.near, camera.far, pick_result, true);
    }

    /* drawing */

    {
        /* draw fuselage skin */

        graph_clear(vec3(0.9, 0.9, 0.9));

        valued_program.use();
        valued_program.set_uniform_mat4(0, projection);
        valued_program.set_uniform_mat4(1, mv_stack.top());

        ui_model_draw_skin_triangles(&ui_model, valued_program, mv_stack, pick_result);

#if DRAW_CORRS
        colored_program.use();
        colored_program.set_uniform_mat4(0, projection);
        colored_program.set_uniform_mat4(1, mv_stack.top());

        ui_model_draw_corrs(&ui_model, colored_program);
#endif

        /* graph_clear_depth(); */

        /* draw objects */

        shaded_program.use();
        shaded_program.set_uniform_mat4(0, projection);
        shaded_program.set_uniform_mat4(1, mv_stack.top());
        ui_model_draw_mantles(&ui_model, shaded_program, mv_stack, pick_result);
        warehouse.draw_triangles(shaded_program, mv_stack, camera.pos, camera.dir);

        /* draw lines */

        program.use();
        program.set_uniform_mat4(0, projection);
        program.set_uniform_mat4(1, mv_stack.top());

        ui_model_draw_lines(&ui_model, program, mv_stack, pick_result);
        warehouse.draw_lines(program, mv_stack, camera.pos, camera.dir);

        /* draw headup */

        program.use();

        graph_enable_depth_test(false);
        graph_line_width(2);

        mat4 headup_projection;
        graph_ortho(headup_projection, 0, window.w, 0, window.h, -100, 100);

        mat4 headup_modelview;
        headup_modelview.set_identity();

        program.set_uniform_mat4(0, headup_projection);
        program.set_uniform_mat4(1, headup_modelview);

        program.set_uniform_vec4(2, vec4(0, 0, 0, 1));
        program.set_data<vec3>(0, 4, crosshair_verts);
        graph_draw_lines(4);

        window_swap_buffers();
    }

    platform_sleep(10);
    window_poll_events();
}

int main() {

#if AIRFOIL_GENERATE_BASE
    airfoil_generate_base();
    return 0;
#else
    airfoil_init_base();
#endif

    model_collision_init();

    /* create window */

    window_create("Boids", 1800, 1000, false);

    program.init("src/shaders/default.vert", "src/shaders/default.frag");
    program.define_in_float(3); /* position */
    program.define_uniform("projection");
    program.define_uniform("modelview");
    program.define_uniform("color");

#if DRAW_CORRS
    colored_program.init("src/shaders/colored.vert", "src/shaders/colored.frag");
    colored_program.define_in_float(3); /* position */
    colored_program.define_in_float(3); /* color */
    colored_program.define_uniform("projection");
    colored_program.define_uniform("modelview");
#endif

    shaded_program.init("src/shaders/shaded.vert", "src/shaders/shaded.frag");
    shaded_program.define_in_float(3); /* position */
    shaded_program.define_uniform("projection");
    shaded_program.define_uniform("modelview");
    shaded_program.define_uniform("color");

    valued_program.init("src/shaders/valued.vert", "src/shaders/valued.frag");
    valued_program.define_in_float(3); /* position */
    valued_program.define_in_float(1); /* value */
    valued_program.define_uniform("projection");
    valued_program.define_uniform("modelview");

    window_set_cursor_hidden(true);
    window_set_key_callback(_key_callback);
    window_set_mousepos_callback(_mousepos_callback);
    window_set_scroll_callback(_scroll_callback);
    window_set_mousebutton_callback(_mousebutton_callback);

    crosshair_verts[0] = vec3(window.w * 0.5f - 10.0f, window.h * 0.5f, 0.0f);
    crosshair_verts[1] = vec3(window.w * 0.5f + 10.0f, window.h * 0.5f, 0.0f);
    crosshair_verts[2] = vec3(window.w * 0.5f, window.h * 0.5f - 10.0f, 0.0f);
    crosshair_verts[3] = vec3(window.w * 0.5f, window.h * 0.5f + 10.0f, 0.0f);

    ui_model_init_drawing();

    /* run window */

    window_run(_main_loop_func);

    return 0;
}
