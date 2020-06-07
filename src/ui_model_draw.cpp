#include "modeling_model.h"
#include "modeling_object.h"
#include "modeling_wing.h"
#include "ui_graphics.h"
#include "ui_pick.h"
#include "memory_arena.h"
#include "modeling_config.h"
#include <math.h>
#include <float.h>
#include <assert.h>


// TODO: part of analysis?

static Arena skin_vert_colors_arena(10000000);

void model_update_skin_verts_values(Model *model, SkinVertColorSource source) {
    skin_vert_colors_arena.clear();
    float *values = skin_vert_colors_arena.alloc<float>(model->skin_verts_count, true);

    if (source == NO_SOURCE) {
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
