#ifndef airfoil_h
#define airfoil_h

#define AIRFOIL_X_SUBDIVS       32
#define AIRFOIL_POINTS          (AIRFOIL_X_SUBDIVS * 2 + 1)
#define AIRFOIL_GENERATE_BASE   0


struct dvec;

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
void airfoil_init_from_base(Airfoil *airfoil,
                            float u_base, float u_delta, unsigned char *u_y,
                            float l_base, float l_delta, unsigned char *l_y);

void airfoil_generate_base();

#endif
