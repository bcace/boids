#include "modeling/model.h"
#include "modeling/object.h"
#include "modeling/wing.h"
#include "graphics.h"
#include "pick.h"
#include "arena.h"
#include "modeling/config.h"
#include <math.h>
#include <float.h>
#include <assert.h>


unsigned int wing_pick_category;
unsigned int object_model_pick_category;
unsigned int object_handle_pick_category;

static vec4 _wing_color(Wing *w, void *hovered_pickable) {
    if (w->selected) {
        if (w == hovered_pickable)
            return vec4(0.8f, 0.7f, 0.0f, 1.0f);
        else
            return vec4(1.0f, 0.9f, 0.0f, 1.0f);
    }
    else {
        if (w == hovered_pickable)
            return vec4(0.25f, 0.7f, 0.0f, 1.0f);
        else
            return vec4(0.4f, 0.9f, 0.0f, 1.0f);
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

void Model::draw_triangles(ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result) {
    void *hovered_pickable = decode_pick_result(pick_result);

    for (int i = 0; i < objects_count; ++i) {
        Object *o = objects[i];
        mantle_draw_quads(&o->model_mantle, program, mv_stack, _object_color(o, hovered_pickable));
    }

    for (int i = 0; i < wings_count; ++i) {
        Wing *w = wings[i];
        mantle_draw_quads(&w->mantle, program, mv_stack, _wing_color(w, hovered_pickable));
    }
}

void Model::draw_skin_triangles(ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result) {
    program.set_data<vec3>(0, skin_verts_count, skin_verts);
    program.set_data<float>(1, skin_verts_count, skin_verts_values);
    graph_draw_triangles_indexed(skin_trias_count * 3, skin_trias);
    graph_draw_quads_indexed(skin_quads_count * 4, skin_quads);
}

void Model::draw_lines(ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result) {
    graph_line_width(2);

    graph_set_polygon_line_mode(true);

    program.set_uniform_vec4(2, vec4(0.0f, 0.0f, 0.0f, MESH_ALPHA));
    program.set_data<vec3>(0, skin_verts_count, skin_verts);
    graph_draw_triangles_indexed(skin_trias_count * 3, skin_trias);
    graph_draw_quads_indexed(skin_quads_count * 4, skin_quads);

    for (int i = 0; i < objects_count; ++i) {
        Object *o = objects[i];
        mantle_draw_quads(&o->model_mantle, program, mv_stack, vec4(0.0f, 0.0f, 0.0f, MESH_ALPHA));
    }

    for (int i = 0; i < wings_count; ++i) {
        Wing *w = wings[i];
        mantle_draw_quads(&w->mantle, program, mv_stack, vec4(0.0f, 0.0f, 0.0f, MESH_ALPHA));
    }

    graph_set_polygon_line_mode(false);
}

#if DRAW_CORRS
void Model::draw_corrs(ShaderProgram &program) {
    graph_line_width(4);
    program.set_data<vec3>(0, corrs_count * 2, corr_verts);
    program.set_data<vec3>(1, corrs_count * 2, corr_colors);
    graph_draw_lines(corrs_count * 2);
}
#endif

void Model::pick(ShaderProgram &program, mat4_stack &mv_stack) {
    graph_enable_blend(false);
    graph_enable_smooth_line(false);

    for (int i = 0; i < objects_count; ++i)
        mantle_draw_quads(&objects[i]->model_mantle, program, mv_stack, pick_encode(object_model_pick_category, i));

    for (int i = 0; i < wings_count; ++i)
        mantle_draw_quads(&wings[i]->mantle, program, mv_stack, pick_encode(wing_pick_category, i));

    graph_enable_blend(true);
    graph_enable_smooth_line(true);
}

void *Model::decode_pick_result(PickResult &result) {
    if (!result.hit)
        return 0;
    if (result.category_id == wing_pick_category)
        return wings[result.ids[0]];
    if (result.category_id == object_model_pick_category)
        return objects[result.ids[0]];
    if (result.category_id == object_handle_pick_category)
        return &objects[result.ids[0]]->handles[result.ids[1]][result.ids[2]];
    return 0;
}

bool Model::maybe_drag_selection(PickResult &pick_result, bool ctrl_pressed) {
    if (!ctrl_pressed)
        deselect_all();

    void *hovered_pickable = decode_pick_result(pick_result);
    if (hovered_pickable == 0)
        return false;

    if (pick_result.category_id == wing_pick_category) {
        Wing *w = (Wing *)hovered_pickable;
        w->selected = !w->selected;
        if (w->selected) {
            for (int i = 0; i < wings_count; ++i)
                wing_reset_target_position(wings[i]);
            return true;
        }
    }
    else if (pick_result.category_id == object_model_pick_category) {
        Object *o = (Object *)hovered_pickable;
        o->selected = !o->selected;
        if (o->selected) {
            for (int i = 0; i < objects_count; ++i)
                objects[i]->reset_drag_p();
            return true;
        }
    }

    return false;
}

void init_model_draw() {
    const int OBJECTS_SIZE = 6;
    assert((1 << OBJECTS_SIZE) >= MAX_ELEMS);

    object_handle_pick_category = pick_register_category();
    object_model_pick_category = pick_register_category();
    wing_pick_category = pick_register_category();

    pick_define_encoding(object_handle_pick_category, OBJECTS_SIZE, 2); /* object, tail/nose, handle */
}

void model_update_object_mantles(Model *model) {
    for (int i = 0; i < model->objects_count; ++i)
        object_update_mantle(model->objects[i]);
    for (int i = 0; i < model->wings_count; ++i) {
        Wing *w = model->wings[i];
        mantle_generate_from_airfoil(&w->mantle, &mantle_arena(), &w->airfoil, w->x, w->y, w->z);
    }
}

static Arena skin_vert_colors_arena(10000000);

void model_update_skin_verts_values(Model *model, SkinVertColorSource source) {
    skin_vert_colors_arena.clear();
    float *values = skin_vert_colors_arena.alloc<float>(model->skin_verts_count, true);

    if (source == NO_SOURCE) { // if we don't want to show any value
        for (int i = 0; i < model->skin_verts_count; ++i)
            values[i] = 10.0f;
    }
    else {
        int *panels = skin_vert_colors_arena.alloc<int>(model->skin_verts_count, true);

        if (source == VX) {
            for (int i = 0; i < model->panels_count; ++i) {
                Panel *panel = model->panels + i;
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
            assert(false); // unhandled source type

        float min = FLT_MAX;
        float max = -FLT_MAX;
        for (int i = 0; i < model->skin_verts_count; ++i) {
            values[i] /= panels[i];
            if (values[i] < min)
                min = values[i];
            if (values[i] > max)
                max = values[i];
        }

        float range = max - min;
        for (int i = 0; i < model->skin_verts_count; ++i)
            values[i] = (values[i] - min) / range;
    }

    model->skin_verts_values = values;
}
