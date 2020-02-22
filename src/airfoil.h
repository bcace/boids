#ifndef airfoil_h
#define airfoil_h

#define AIRFOIL_SUBDIVS     32
#define AIRFOIL_POINTS      (AIRFOIL_SUBDIVS * 2 + 1)
#define AIRFOIL_MAKING_MODE 0


struct dvec;

struct AirfoilSide {
    float base, delta; /* y extents */
    unsigned char y[AIRFOIL_SUBDIVS]; /* y fractions of extents */
};

struct Airfoil {
    AirfoilSide upper;
    AirfoilSide lower;
};

double airfoil_get_subdiv_x(int i);
dvec airfoil_get_point(Airfoil *airfoil, int i);

Airfoil airfoil_parse_selig(const char *path, int count);

#endif
