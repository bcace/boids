#include "config.h"
#include "constants.h"


static const float _MAX_ONE_SIDE_MERGE_DELAY = 0.5f;
static const float _MAX_TWO_SIDE_MERGE_DELAY = 0.3f;
static float _MERGE_DELAY_FACTOR = 1.0f;

static const double _MIN_STRUCTURAL_MARGIN = 0.01;
static const double _MAX_STRUCTURAL_MARGIN = 2.0;

double LONGITUDINAL_SMOOTHNESS = 0.5;

int SHAPE_CURVE_SAMPLES = MAX_CURVE_SUBDIVS;

double STRUCTURAL_MARGIN = 0.1;

double MESH_ALPHA = 0.1;

float ONE_SIDE_MERGE_DELAY = _MAX_ONE_SIDE_MERGE_DELAY;
float ONE_MINUS_ONE_SIDE_MERGE_DELAY = 1.0f - ONE_SIDE_MERGE_DELAY;
float TWO_SIDE_MERGE_DELAY = _MAX_TWO_SIDE_MERGE_DELAY;
float ONE_MINUS_TWO_SIDE_MERGE_DELAY = 1.0f - TWO_SIDE_MERGE_DELAY;

void config_decrease_shape_samples() {
    if (SHAPE_CURVE_SAMPLES > MIN_CURVE_SUBDIVS)
        SHAPE_CURVE_SAMPLES /= 2;
}

void config_increase_shape_samples() {
    if (SHAPE_CURVE_SAMPLES < MAX_CURVE_SUBDIVS)
        SHAPE_CURVE_SAMPLES *= 2;
}

void config_decrease_structural_margin() {
    if (STRUCTURAL_MARGIN > _MIN_STRUCTURAL_MARGIN) {
        STRUCTURAL_MARGIN /= 1.2;
        if (STRUCTURAL_MARGIN < _MIN_STRUCTURAL_MARGIN)
            STRUCTURAL_MARGIN = _MIN_STRUCTURAL_MARGIN;
    }
}

void config_increase_structural_margin() {
    if (STRUCTURAL_MARGIN < _MAX_STRUCTURAL_MARGIN) {
        STRUCTURAL_MARGIN *= 1.2;
        if (STRUCTURAL_MARGIN > _MAX_STRUCTURAL_MARGIN)
            STRUCTURAL_MARGIN = _MAX_STRUCTURAL_MARGIN;
    }
}

void config_decrease_mesh_triangle_edge_transparency() {
    MESH_ALPHA -= 0.1;
    if (MESH_ALPHA < 0.0)
        MESH_ALPHA = 0.0;
}

void config_increase_mesh_triangle_edge_transparency() {
    MESH_ALPHA += 0.1;
    if (MESH_ALPHA > 1.0)
        MESH_ALPHA = 1.0;
}

void _update_merge_delays() {
    ONE_SIDE_MERGE_DELAY = _MAX_ONE_SIDE_MERGE_DELAY * _MERGE_DELAY_FACTOR;
    TWO_SIDE_MERGE_DELAY = _MAX_TWO_SIDE_MERGE_DELAY * _MERGE_DELAY_FACTOR;
    ONE_MINUS_ONE_SIDE_MERGE_DELAY = 1.0f - ONE_SIDE_MERGE_DELAY;
    ONE_MINUS_TWO_SIDE_MERGE_DELAY = 1.0f - TWO_SIDE_MERGE_DELAY;
}

void config_decrease_merge_interpolation_delay() {
    _MERGE_DELAY_FACTOR -= 0.1f;
    if (_MERGE_DELAY_FACTOR < 0.0f)
        _MERGE_DELAY_FACTOR = 0.0f;
    _update_merge_delays();
}

void config_increase_merge_interpolation_delay() {
    _MERGE_DELAY_FACTOR += 0.1f;
    if (_MERGE_DELAY_FACTOR > 1.0f)
        _MERGE_DELAY_FACTOR = 1.0f;
    _update_merge_delays();
}
