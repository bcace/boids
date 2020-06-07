#ifndef ui_model_h
#define ui_model_h

#include "modeling_model.h"
#include "ui_mantle.h"

#define MAX_MODEL_MANTLES   100
#define DRAW_CORRS          0 /* just for debugging */


struct mat4_stack;
struct PickResult;
struct ShaderProgram;

struct UiModel {
    Model model;
    Mantle o_mantles[MAX_MODEL_MANTLES];
    Mantle w_mantles[MAX_MODEL_MANTLES];
};

void ui_model_init(UiModel *ui_model);
void ui_model_update_mantles(UiModel *ui_model);

void ui_model_init_drawing();
void ui_model_draw_mantles(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result);
void ui_model_draw_lines(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result);
void ui_model_draw_skin_triangles(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack, PickResult &pick_result);

void ui_model_draw_for_picking(UiModel *ui_model, ShaderProgram &program, mat4_stack &mv_stack);
bool ui_model_maybe_drag_selection(UiModel *ui_model, PickResult *pick_result, bool ctrl_pressed);

#if DRAW_CORRS
void ui_model_draw_corrs(UiModel *ui_model, ShaderProgram &program);
#endif

#endif
