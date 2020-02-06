#ifndef config_h
#define config_h

#define MIN_SHAPE_CURVE_SAMPLES     2
#define MAX_SHAPE_CURVE_SAMPLES     8

#define MIN_STRUCTURAL_MARGIN       0.01
#define MAX_STRUCTURAL_MARGIN       2.0

#define MAX_FUSELAGE_SUBDIVS_COUNT  128
#define MIN_FUSELAGE_SUBDIVS_COUNT  8

#define LONGITUDINAL_SMOOTHNESS     0.5


extern int SHAPE_CURVE_SAMPLES;
extern double STRUCTURAL_MARGIN;
extern double MESH_ALPHA;
extern int FUSELAGE_SUBDIVS_COUNT;

void config_decrease_sections_count();
void config_increase_sections_count();

void config_decrease_shape_samples();
void config_increase_shape_samples();

void config_decrease_structural_margin();
void config_increase_structural_margin();

void config_decrease_mesh_triangle_edge_transparency();
void config_increase_mesh_triangle_edge_transparency();

//#define BOIDS_USE_APAME

#endif
