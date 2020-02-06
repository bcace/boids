#include "config.h"


int SHAPE_CURVE_SAMPLES = MIN_SHAPE_CURVE_SAMPLES;
double STRUCTURAL_MARGIN = 0.1;
double MESH_ALPHA = 0.1;
int FUSELAGE_SUBDIVS_COUNT = 16;


void config_decrease_sections_count() {
    if (FUSELAGE_SUBDIVS_COUNT > MIN_FUSELAGE_SUBDIVS_COUNT)
        FUSELAGE_SUBDIVS_COUNT /= 2;
}

void config_increase_sections_count() {
    if (FUSELAGE_SUBDIVS_COUNT < MAX_FUSELAGE_SUBDIVS_COUNT)
        FUSELAGE_SUBDIVS_COUNT *= 2;
}

void config_decrease_shape_samples() {
    if (SHAPE_CURVE_SAMPLES > MIN_SHAPE_CURVE_SAMPLES) {
        SHAPE_CURVE_SAMPLES /= 2;
        FUSELAGE_SUBDIVS_COUNT /= 2;
    }
}

void config_increase_shape_samples() {
    if (SHAPE_CURVE_SAMPLES < MAX_SHAPE_CURVE_SAMPLES) {
        SHAPE_CURVE_SAMPLES *= 2;
        FUSELAGE_SUBDIVS_COUNT *= 2;
    }
}

void config_decrease_structural_margin() {
    if (STRUCTURAL_MARGIN > MIN_STRUCTURAL_MARGIN) {
        STRUCTURAL_MARGIN /= 1.2;
        if (STRUCTURAL_MARGIN < MIN_STRUCTURAL_MARGIN)
            STRUCTURAL_MARGIN = MIN_STRUCTURAL_MARGIN;
    }
}

void config_increase_structural_margin() {
    if (STRUCTURAL_MARGIN < MAX_STRUCTURAL_MARGIN) {
        STRUCTURAL_MARGIN *= 1.2;
        if (STRUCTURAL_MARGIN > MAX_STRUCTURAL_MARGIN)
            STRUCTURAL_MARGIN = MAX_STRUCTURAL_MARGIN;
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
