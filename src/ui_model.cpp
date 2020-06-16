#include "ui_model.h"
#include "ui_pick.h"
#include "ui_graphics.h"
#include "modeling_object.h"
#include "modeling_config.h"
#include "memory_arena.h"
#include <assert.h>
#include <float.h>


unsigned int object_model_pick_category;

void ui_model_update_mantles(UiModel *ui_model) {
    Model *m = &ui_model->model;

    mantle_clear_arena();

    for (int i = 0; i < m->objects_count; ++i) {
        Object *o = m->objects[i];
        Mantle *o_mantle = ui_model->o_mantles + i;
        mantle_generate_from_former_array(o_mantle, &mantle_arena(), o->def.formers, o->def.formers_count, o->p.x, o->p.y, o->p.z);
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
    graph_draw_triangles_indexed(ui_model->skin_trias_count * 3, ui_model->skin_trias);
    graph_draw_quads_indexed(ui_model->skin_quads_count * 4, ui_model->skin_quads);

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
    program.set_data<float>(1, m->skin_verts_count, ui_model->skin_verts_values);
    graph_draw_triangles_indexed(ui_model->skin_trias_count * 3, ui_model->skin_trias);
    graph_draw_quads_indexed(ui_model->skin_quads_count * 4, ui_model->skin_quads);
}

bool ui_model_maybe_drag_selection(UiModel *ui_model, PickResult *pick_result, bool ctrl_pressed) {
    Model *m = &ui_model->model;

    if (!ctrl_pressed)
        model_deselect_all(m);

    void *hovered_pickable = _decode_pick_result(m, pick_result);
    if (hovered_pickable == 0)
        return false;

    if (pick_result->category_id == object_model_pick_category) {
        Object *o = (Object *)hovered_pickable;
        o->selected = !o->selected;
        if (o->selected) {
            for (int i = 0; i < m->objects_count; ++i)
                object_reset_drag_p(m->objects[i]);
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

static Arena trias_arena(10000000);
static Arena quads_arena(10000000);
static Arena values_arena(10000000);

/* make triangles and quads for drawing from generated panels */
void ui_model_update_skin(UiModel *ui_model, SkinVertColorSource source) {
    Model *m = &ui_model->model;

    trias_arena.clear();
    ui_model->skin_trias = trias_arena.rest<int>();
    ui_model->skin_trias_count = 0;

    quads_arena.clear();
    ui_model->skin_quads = quads_arena.rest<int>();
    ui_model->skin_quads_count = 0;

    for (int i = 0; i < m->panels_count; ++i) {
        Panel *p = m->panels + i;
        if (p->v4 == -1) {  /* triangle */
            int *idx = trias_arena.alloc<int>(3); // TODO: think about pre-allocating this
            *idx++ = p->v1;
            *idx++ = p->v2;
            *idx++ = p->v3;
            ++ui_model->skin_trias_count;
        }
        else {              /* quad */
            int *idx = quads_arena.alloc<int>(4); // TODO: think about pre-allocating this
            *idx++ = p->v1;
            *idx++ = p->v2;
            *idx++ = p->v3;
            *idx++ = p->v4;
            ++ui_model->skin_quads_count;
        }
    }

    /* update values */

    values_arena.clear();
    float *values = values_arena.alloc<float>(m->skin_verts_count, true);

    if (source == NO_SOURCE) {
        for (int i = 0; i < m->skin_verts_count; ++i)
            values[i] = 10.0f;
    }
    else {
        int *panels = values_arena.alloc<int>(m->skin_verts_count, true);

        if (source == VX) {
            for (int i = 0; i < m->panels_count; ++i) {
                Panel *panel = m->panels + i;
                values[panel->v1] += panel->vx;
                ++panels[panel->v1];
                values[panel->v2] += panel->vx;
                ++panels[panel->v2];
                values[panel->v3] += panel->vx;
                ++panels[panel->v3];
                if (panel->v4 != -1) {
                    values[panel->v4] += panel->vx;
                    ++panels[panel->v4];
                }
            }
        }
        else if (source == VY) {
            // ...
        }
        else if (source == VZ) {
            // ...
        }
        else if (source == V) {
            // ...
        }
        else
            assert(false); /* unhandled source type */

        float min = FLT_MAX;
        float max = -FLT_MAX;
        for (int i = 0; i < m->skin_verts_count; ++i) {
            values[i] /= panels[i];
            if (values[i] < min)
                min = values[i];
            if (values[i] > max)
                max = values[i];
        }

        float range = max - min;
        for (int i = 0; i < m->skin_verts_count; ++i)
            values[i] = (values[i] - min) / range;
    }

    ui_model->skin_verts_values = values;
}
