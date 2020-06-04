#include "apame.h"
#include "modeling/model.h"
#include "arena.h"
#include "vec.h"

#ifdef BOIDS_USE_APAME

#include "apame/main.h"
#include <iostream>


static Arena apame_arena(100000000);

void boids_apame_run(Model *model) {

    apame_arena.clear();

    int PARAMS_INT[numIntParams];
    PARAM_METH = 0;
    PARAM_VELO = 2;

    float PARAMS_FLT[numFltParams];
    PARAM_SPED = 1.0f;
    PARAM_DENS = 1.225f;
    PARAM_PRES = 101325.0f;
    PARAM_MACH = 0.0f;
    PARAM_SPAN = 1.0f;
    PARAM_MEAC = 1.0f;
    PARAM_AREA = 1.0f;
    PARAM_ORIX = 0.0f;
    PARAM_ORIY = 0.0f;
    PARAM_ORIZ = 0.0f;
    PARAM_COPL = 0.0f;
    PARAM_COLL = 0.0000001f;
    PARAM_FARF = 5.0f;

    const int cases_count = 1;
    float angles_of_attack[] = {0.0f};
    float sideslip_angles[] = {0.0f};

    // calculate node and panel counts

    int nodes_count = model->skin_verts_count;
    int body_panels_count = model->panels_count;

    // fill node coordinates

    float *node_coords = apame_arena.alloc<float>(nodes_count * 3);
    float *node = node_coords;
    for (int i = 0; i < model->skin_verts_count; ++i) {
        *node++ = model->skin_verts[i].x;
        *node++ = model->skin_verts[i].y;
        *node++ = model->skin_verts[i].z;
    }

    // fill body panels

    int *body_panels = apame_arena.alloc<int>(body_panels_count * numPanelParams);
    int *PANELS = body_panels;
    int PANEL_INDEX = 0;

    for (int i = 0; i < model->panels_count; ++i) {
        Panel *panel = model->panels + i;

        if (panel->v4 == -1) {
            NUM_PANEL_NODES = 3;
            PANEL_NODES[0] = panel->v1;
            PANEL_NODES[1] = panel->v2;
            PANEL_NODES[2] = panel->v3;
        }
        else {
            NUM_PANEL_NODES = 4;
            PANEL_NODES[0] = panel->v1;
            PANEL_NODES[1] = panel->v2;
            PANEL_NODES[2] = panel->v3;
            PANEL_NODES[3] = panel->v4;
        }

        if (panel->tail == -1 && panel->nose == -1) {
            NUM_PANEL_NGBRS = 2;
            PANEL_NGBRS[0] = panel->prev;
            PANEL_NGBRS[1] = panel->next;
        }
        else if (panel->tail == -1) {
            NUM_PANEL_NGBRS = 3;
            PANEL_NGBRS[0] = panel->prev;
            PANEL_NGBRS[1] = panel->next;
            PANEL_NGBRS[2] = panel->nose;
        }
        else if (panel->nose == -1) {
            NUM_PANEL_NGBRS = 3;
            PANEL_NGBRS[0] = panel->prev;
            PANEL_NGBRS[1] = panel->next;
            PANEL_NGBRS[2] = panel->tail;
        }
        else {
            NUM_PANEL_NGBRS = 4;
            PANEL_NGBRS[0] = panel->prev;
            PANEL_NGBRS[1] = panel->next;
            PANEL_NGBRS[2] = panel->tail;
            PANEL_NGBRS[3] = panel->nose;
        }

        PANEL_STATE = 0;
        ++PANEL_INDEX;
    }

    // fill wake panels

    int wake_panels_count = 0;
    int *wake_panels = apame_arena.alloc<int>(wake_panels_count * 4);

    int CALC_REQUESTS[numCalcRequests] = {0};
    REQUEST_COEF = 1;
    REQUEST_PRES = 1;

    // allocate memory

    float *matrixA = apame_arena.alloc<float>(body_panels_count * body_panels_count);
    float *matrixB = apame_arena.alloc<float>(body_panels_count * body_panels_count);
    int *ipiv = apame_arena.alloc<int>(body_panels_count);
    float *SCALAR_RESULTS = apame_arena.alloc<float>(numScalarResults * cases_count);
    float *FIELD_RESULTS = apame_arena.alloc<float>(numFieldResults * cases_count * body_panels_count);
    float *bodyGridData = apame_arena.alloc<float>(numGridData * body_panels_count);
    float *wakeGridData = apame_arena.alloc<float>(numGridData * wake_panels_count);

    // call ApameCore calculation routine

    runApame(
        false,
        false,
        "job-name",
        PARAMS_INT,
        PARAMS_FLT,
        cases_count,
        angles_of_attack,
        sideslip_angles,
        nodes_count,
        node_coords,
        body_panels_count,
        body_panels,
        wake_panels_count,
        wake_panels,
        0,
        CALC_REQUESTS,
        SCALAR_RESULTS,
        FIELD_RESULTS,
        bodyGridData,
        wakeGridData,
        matrixA,
        matrixB,
        ipiv
    );

    int numBodyPanels = model->panels_count;
    int numCases = cases_count;

    for (int i = 0; i < model->panels_count; ++i) {
        Panel *panel = model->panels + i;
        panel->vx = FIELD_VELX[i];
        panel->vy = FIELD_VELY[i];
        panel->vz = FIELD_VELZ[i];
    }

    // std::cout << std::endl << "forces:" << std::endl;
    // for (int i=0; i<3; i++){
    //     for (int caseIndex=0; caseIndex<cases_count; caseIndex++)
    //         std::cout << SCALAR_CFOR[caseIndex*3+i] << " ";
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl << "moments:" << std::endl;
    // for (int i=0; i<3; i++){
    //     for (int caseIndex=0; caseIndex<cases_count; caseIndex++)
    //         std::cout << SCALAR_CMOM[caseIndex*3+i] << " ";
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;
}

#endif
