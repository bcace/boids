#ifndef airfoil_h
#define airfoil_h

#define AIRFOIL_MAX_BASE_COUNT  256
#define AIRFOIL_X_SUBDIVS       32
#define AIRFOIL_POINTS          (AIRFOIL_X_SUBDIVS * 2 + 1)

#define AIRFOIL_GENERATE_BASE   0


struct dvec;
struct tvec;
struct vec3;

struct AirfoilSide {
    float base, delta; /* y extents */
    unsigned char y[AIRFOIL_X_SUBDIVS]; /* y fractions of extents */
};

struct Airfoil {
    char name[16];
    AirfoilSide upper, lower;
};

double airfoil_get_subdiv_x(int i);

dvec airfoil_get_point(Airfoil *airfoil, int i);
void airfoil_init(Airfoil *airfoil, const char *name,
                  float u_base, float u_delta, unsigned char *u_y,
                  float l_base, float l_delta, unsigned char *l_y);
float airfoil_get_trailing_y_offset(Airfoil *a);
void airfoil_get_drawing_verts(Airfoil *a, vec3 *verts, double dihedral, double chord, double aoa, double x, double y, double z);
void airfoil_get_verts(Airfoil *a, tvec *u_verts, tvec *l_verts, double dihedral, double chord, double aoa, double dx, double dy, double dz);

void airfoil_generate_base(); /* this generates hardcoded airfoils */
void airfoil_init_base(); /* this initializes hardcoded airfoils */

extern Airfoil airfoils_base[AIRFOIL_MAX_BASE_COUNT];
extern int airfoils_base_count;

#endif
