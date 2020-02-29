#ifndef config_h
#define config_h

extern double LONGITUDINAL_SMOOTHNESS;
extern int SHAPE_CURVE_SAMPLES;
extern double STRUCTURAL_MARGIN;
extern double MESH_ALPHA;
extern float ONE_SIDE_MERGE_DELAY;
extern float TWO_SIDE_MERGE_DELAY;
extern float ONE_MINUS_ONE_SIDE_MERGE_DELAY;
extern float ONE_MINUS_TWO_SIDE_MERGE_DELAY;

void config_decrease_shape_samples();
void config_increase_shape_samples();

void config_decrease_structural_margin();
void config_increase_structural_margin();

void config_decrease_mesh_triangle_edge_transparency();
void config_increase_mesh_triangle_edge_transparency();

void config_decrease_merge_interpolation_delay();
void config_increase_merge_interpolation_delay();

//#define BOIDS_USE_APAME

#endif
