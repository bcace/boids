#include "ui_model.h"
#include "ui_pick.h"
#include "ui_graphics.h"
#include "modeling_object.h"
#include "modeling_config.h"


unsigned int object_model_pick_category;

void ui_model_update_mantles(UiModel *ui_model) {
    Model *m = &ui_model->model;

    mantle_clear_arena();

    for (int i = 0; i < m->objects_count; ++i) {
        Object *o = m->objects[i];
        Mantle *o_mantle = ui_model->o_mantles + i;
        mantle_generate_from_former_array(o_mantle, &mantle_arena(), o->formers, o->formers_count, o->p.x, o->p.y, o->p.z);
    }
}

static vec4 _object_color(Object *o, void *hovered_pickable) {
    if (o->selected) {
        if (o == hovered_pickable)
            return vec4(0.8f, 0.7f, 0.0f, 1.0f);
        else
            return vec4(1.0f, 0.9f, 0.0f, 1.0f);
    }
    else {
        if (o == hovered_pickable)
            return vec4(0.25f, 0.7f, 0.0f, 1.0f);
        else
            return vec4(0.4f, 0.9f, 0.0f, 1.0f);
    }
}

void *_decode_pick_result(Model *m, PickResult *result) {
    if (!result->hit)
        return 0;
    if (result->category_id == object_model_pick_category)
        return m->objects[result->ids[0]];
    return 0;
}

void ui_model_draw_mantles(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result) {
    Model *m = &ui_model->model;

    void *hovered_pickable = _decode_pick_result(m, &pick_result);

    for (int i = 0; i < m->objects_count; ++i) {
        Object *o = m->objects[i];
        Mantle *o_mantle = ui_model->o_mantles + i;
        mantle_draw_quads(o_mantle, program, mv_stack, _object_color(o, hovered_pickable));
    }
}


void ui_model_draw_for_picking(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack) {
    Model *m = &ui_model->model;

    graph_enable_blend(false);
    graph_enable_smooth_line(false);

    for (int i = 0; i < m->objects_count; ++i) {
        Mantle *o_mantle = ui_model->o_mantles + i;
        mantle_draw_quads(o_mantle, program, mv_stack, pick_encode(object_model_pick_category, i));
    }

    graph_enable_blend(true);
    graph_enable_smooth_line(true);
}

void ui_model_draw_lines(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result) {
    Model *m = &ui_model->model;

    graph_line_width(2);
    graph_set_polygon_line_mode(true);

    program.set_uniform_vec4(2, vec4(0.0f, 0.0f, 0.0f, MESH_ALPHA));

    program.set_data<vec3>(0, m->skin_verts_count, m->skin_verts);
    graph_draw_triangles_indexed(m->skin_trias_count * 3, m->skin_trias);
    graph_draw_quads_indexed(m->skin_quads_count * 4, m->skin_quads);

    for (int i = 0; i < m->objects_count; ++i) {
        Object *o = m->objects[i];
        Mantle *o_mantle = ui_model->o_mantles + i;
        mantle_draw_quads(o_mantle, program, mv_stack, vec4(0.0f, 0.0f, 0.0f, MESH_ALPHA));
    }

    graph_set_polygon_line_mode(false);
}

void ui_model_draw_skin_triangles(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result) {
    Model *m = &ui_model->model;
    program.set_data<vec3>(0, m->skin_verts_count, m->skin_verts);
    program.set_data<float>(1, m->skin_verts_count, m->skin_verts_values);
    graph_draw_triangles_indexed(m->skin_trias_count * 3, m->skin_trias);
    graph_draw_quads_indexed(m->skin_quads_count * 4, m->skin_quads);
}

bool ui_model_maybe_drag_selection(UiModel *ui_model, PickResult *pick_result, bool ctrl_pressed) {
    Model *m = &ui_model->model;

    if (!ctrl_pressed)
        m->deselect_all();

    void *hovered_pickable = _decode_pick_result(m, pick_result);
    if (hovered_pickable == 0)
        return false;

    if (pick_result->category_id == object_model_pick_category) {
        Object *o = (Object *)hovered_pickable;
        o->selected = !o->selected;
        if (o->selected) {
            for (int i = 0; i < m->objects_count; ++i)
                m->objects[i]->reset_drag_p();
            return true;
        }
    }

    return false;
}

void ui_model_init_drawing() {
    object_model_pick_category = pick_register_category();
}

#if DRAW_CORRS
void ui_model_draw_corrs(UiModel *ui_model, ShaderProgram &program) {
    Model *m = &ui_model->model;
    graph_line_width(4);
    program.set_data<vec3>(0, m->corrs_count * 2, m->corr_verts);
    program.set_data<vec3>(1, m->corrs_count * 2, m->corr_colors);
    graph_draw_lines(m->corrs_count * 2);
}
#endif

