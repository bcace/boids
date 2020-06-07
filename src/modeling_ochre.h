#ifndef ochre_api
#define ochre_api


struct OcState;
struct OcNodeGroup;
struct OcLinkGroup;

enum OcLayout {
    OC_LAYOUT_F32_2,
    OC_LAYOUT_F32_3,
};

typedef void (*VPTR_FUNC)(void *e, void *exec_context);
typedef void (*VPTR_VPTR_FUNC)(void *e1, void *e2, void *exec_context);

OcState *ochre_add_state();

void ochre_set_exec_context(OcState *state, void *exec_context);
OcNodeGroup *ochre_add_node_group(OcState *state, unsigned f_offset, unsigned p_offset, OcLayout layout);
OcNodeGroup *ochre_add_inert_node_group(OcState *state);
OcLinkGroup *ochre_add_link_group(OcState *state);

void ochre_add_node_action(OcState *state, OcNodeGroup *node_group, VPTR_FUNC func, unsigned phase);
void ochre_add_link_action(OcState *state, OcLinkGroup *link_group, VPTR_FUNC func, unsigned phase);
void ochre_add_node_interaction(OcState *state, OcNodeGroup *node_group1, OcNodeGroup *node_group2, VPTR_VPTR_FUNC func, unsigned phase);
void ochre_add_link_interaction(OcState *state, OcLinkGroup *link_group1, OcLinkGroup *link_group2, VPTR_VPTR_FUNC func, unsigned phase);
void ochre_add_link_node_interaction(OcState *state, OcLinkGroup *link_group, OcNodeGroup *node_group, VPTR_VPTR_FUNC func, unsigned phase);

void ochre_clear_data(OcState *state);
void ochre_add_node(OcNodeGroup *node_group, void *node);
void ochre_add_link(OcLinkGroup *link_group, void *link);

bool ochre_run(OcState *state, unsigned iterations);

bool ochre_interact_prism_and_prism(int count1, struct vec2 *nodes1, float min1, float max1,
                                    int count2, struct vec2 *nodes2, float min2, float max2,
                                    struct vec3 &f);

#define OFFSETOF(__type__, __var__) ((char *)&((__type__ *)1)->__var__ - (char *)((__type__ *)1))

#endif
