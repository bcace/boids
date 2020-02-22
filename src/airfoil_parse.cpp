#include "airfoil.h"
#include "dvec.h"
#include "platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>


const int _MAX_FILE_SIZE = 10000;
const int _MAX_IN_POINTS = 256;
const int _FRACTION_COUNT = (1 << (sizeof(unsigned char) * 8)) - 1;

static char *_find_newline(char *c) {
    while (*c != '\n' && *c != '\r')
        ++c;
    return c;
}

static char *_eat_whitespace(char *c) {
    while (*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n')
        ++c;
    return c;
}

static char *_eat_number(char *c, double *n) {
    char *b = c;
    while (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r')
        ++c;
    *n = atof(b);
    return c;
}

static unsigned char _get_y_fraction(AirfoilSide *side, double y) {
    int v = (int)round((y - side->base) / side->delta);
    if (v > _FRACTION_COUNT)
        return _FRACTION_COUNT;
    else if (v < 0)
        return 0;
    else
        return v;
}

void _parse_selig_airfoil(Airfoil *a, const char *path, int count) {
    static char *text = 0;
    static dvec *verts = 0;

    if (text == 0) {
        text = (char *)malloc(_MAX_FILE_SIZE);
        verts = (dvec *)malloc(sizeof(dvec) * _MAX_IN_POINTS);
    }

    /* read airfoil file */

    FILE *f = (FILE *)plat_fopen(path, "r");
    fread(text, 1, _MAX_FILE_SIZE, f);
    fclose(f);

    char *c = _find_newline(text); /* skip first line */
    for (int i = 0; i < count; ++i) {
        c = _eat_whitespace(c);
        c = _eat_number(c, &verts[i].x);
        c = _eat_whitespace(c);
        c = _eat_number(c, &verts[i].y);
    }

    /* find leading point and x extents */

    double min_x = DBL_MAX;
    double max_x = -DBL_MAX;
    int lead_i = -1;

    for (int i = 0; i < count; ++i) {
        if (verts[i].x < min_x) {
            lead_i = i;
            min_x = verts[i].x;
        }
        if (verts[i].x > max_x)
            max_x = verts[i].x;
    }

    /* normalize airfoil */

    double scale = 1.0 / (max_x - min_x);
    double lead_y = verts[lead_i].y;

    for (int i = 0; i < count; ++i) {
        verts[i].x = (verts[i].x - min_x) * scale;
        verts[i].y = (verts[i].y - lead_y) * scale;
    }

    /* find y extents */

    double min_u = DBL_MAX;
    double max_u = -DBL_MAX;

    for (int i = 0; i < lead_i; ++i) {
        if (verts[i].y < min_u)
            min_u = verts[i].y;
        if (verts[i].y > max_u)
            max_u = verts[i].y;
    }

    double min_l = DBL_MAX;
    double max_l = -DBL_MAX;

    for (int i = lead_i + 1; i < count; ++i) {
        if (verts[i].y < min_l)
            min_l = verts[i].y;
        if (verts[i].y > max_l)
            max_l = verts[i].y;
    }

    /* resample airfoil */

    a->upper.base = (float)min_u;
    a->upper.delta = (float)((max_u - min_u) / _FRACTION_COUNT);
    a->lower.base = (float)min_l;
    a->lower.delta = (float)((max_l - min_l) / _FRACTION_COUNT);

    int prev_u = lead_i;
    int prev_l = lead_i;

    for (int i = 0; i < AIRFOIL_SUBDIVS - 1; ++i) {
        double x = airfoil_get_subdiv_x(i);

        while (verts[prev_u - 1].x < x)
            --prev_u;

        {
            dvec prev_p = verts[prev_u];
            dvec next_p = verts[prev_u - 1];
            double t = (x - prev_p.x) / (next_p.x - prev_p.x);
            double y = prev_p.y + (next_p.y - prev_p.y) * t;
            a->upper.y[i] = _get_y_fraction(&a->upper, y);
        }

        while (verts[prev_l + 1].x < x)
            ++prev_l;

        {
            dvec prev_p = verts[prev_l];
            dvec next_p = verts[prev_l + 1];
            double t = (x - prev_p.x) / (next_p.x - prev_p.x);
            double y = prev_p.y + (next_p.y - prev_p.y) * t;
            a->lower.y[i] = _get_y_fraction(&a->lower, y);
        }
    }

    a->upper.y[AIRFOIL_SUBDIVS - 1] = _get_y_fraction(&a->upper, verts[0].y);
    a->lower.y[AIRFOIL_SUBDIVS - 1] = _get_y_fraction(&a->lower, verts[count - 1].y);
}

void _print_airfoil_side_y(FILE *f, AirfoilSide *side, const char *label) {

    fprintf(f,
"        unsigned char %s_y[] = { "
    , label);

    for (int i = 0; i < AIRFOIL_SUBDIVS; ++i)
        fprintf(f, "%d, ", side->y[i]);

    fprintf(f,
"};\n");
}

struct _FileInfo {
    const char *path;
    int count;
};

void _print_airfoil(FILE *f, _FileInfo *info, int index) {
    Airfoil a;
    _parse_selig_airfoil(&a, info->path, info->count);

    fprintf(f,
"\n    /* %s */\n"
"    {\n", info->path);

    _print_airfoil_side_y(f, &a.upper, "u");
    _print_airfoil_side_y(f, &a.lower, "l");

    fprintf(f,
"\n"
"        airfoil_init_from_base(airfoils + %d,\n"
"                               %g, %g, u_y,\n"
"                               %g, %g, l_y);\n"
"\n"
"        ++count;\n"
"    }\n",
    index,
    a.upper.base, a.upper.delta,
    a.lower.base, a.lower.delta);
}

void airfoil_generate_base() {

    _FileInfo infos[] = {
        { "airfoils/naca23012.dat", 61 },
        { "airfoils/naca23015_clean.dat", 35 },
        { "airfoils/naca23018_clean.dat", 35 },
        { "airfoils/b737a.dat", 45 },
        { "airfoils/b737b.dat", 45 },
        { "airfoils/b737c.dat", 45 },
        { "airfoils/b737d.dat", 45 },
    };

    int infos_count = sizeof(infos) / sizeof(_FileInfo);

    FILE *f = (FILE *)plat_fopen("src/airfoil_base.cpp", "w");

    fprintf(f,
"#include \"airfoil.h\"\n\n\n"
"int airfoil_init_base(Airfoil *airfoils) {\n"
"    int count = 0;\n"
    );

    for (int i = 0; i < infos_count; ++i)
        _print_airfoil(f, infos + i, i);

    fprintf(f,
"\n"
"    return count;\n"
"}\n"
    );

    fclose(f);
}
