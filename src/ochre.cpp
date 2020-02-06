#include "ochre.h"
#include "vec.h"
#include <float.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


const int MAX_STATES      = 8;
const int MAX_GROUPS      = 32;
const int MAX_PHASES      = 32;
const int MAX_HANDLERS    = 32;
const int NULL_OFFSET     = 102400;

struct _f32_2 {
    float x, y;

    void zero() {
        x = y = 0.0f;
    }

    float length() {
        return sqrtf(x * x + y * y);
    }

    void scale(float s) {
        x *= s;
        y *= s;
    }

    void increase(_f32_2 *o) {
        x += o->x;
        y += o->y;
    }
};

struct _f32_3 {
    float x, y, z;

    void zero() {
        x = y = z = 0.0f;
    }

    float length() {
        return sqrtf(x * x + y * y + z * z);
    }

    void scale(float s) {
        x *= s;
        y *= s;
        z *= s;
    }

    void increase(_f32_3 *o) {
        x += o->x;
        y += o->y;
        z += o->z;
    }
};

struct _Group {
    int count, cap;
    void **agents;

    _Group() : count(0), cap(0), agents(0) {}

    void clear() {
        count = 0;
    }

    void add(void *n) {
        _check_cap(1);
        agents[count++] = n;
    }

    void _check_cap(int incr) {
        if (count + incr > cap) {
            cap = count + incr + 256;
            agents = (void **)realloc(agents, sizeof(void *) * cap);
        }
    }
};

struct OcNodeGroup : _Group {
    unsigned f_offset;
    unsigned p_offset;
    OcLayout layout;

    void reset_force() {
        if (f_offset == NULL_OFFSET)
            return;
        if (layout == OC_LAYOUT_F32_2) {
            for (int i = 0; i < count; ++i)
                ((_f32_2 *)((char *)agents[i] + f_offset))->zero();
        }
        else if (layout == OC_LAYOUT_F32_3) {
            for (int i = 0; i < count; ++i)
                ((_f32_3 *)((char *)agents[i] + f_offset))->zero();
        }
        else
            assert(false); /* unhandled layout */
    }

    float get_max_force(float max_f) {
        if (f_offset == NULL_OFFSET)
            return max_f;
        if (layout == OC_LAYOUT_F32_2) {
            for (int i = 0; i < count; ++i) {
                float f = ((_f32_2 *)((char *)agents[i] + f_offset))->length();
                if (f > max_f)
                    max_f = f;
            }
        }
        else if (layout == OC_LAYOUT_F32_3) {
            for (int i = 0; i < count; ++i) {
                float f = ((_f32_3 *)((char *)agents[i] + f_offset))->length();
                if (f > max_f)
                    max_f = f;
            }
        }
        else
            assert(false); /* unhandled layout */
        return max_f;
    }

    void normalize_force(float f_normalizer) {
        if (f_offset == NULL_OFFSET)
            return;
        if (layout == OC_LAYOUT_F32_2) {
            for (int i = 0; i < count; ++i)
                ((_f32_2 *)((char *)agents[i] + f_offset))->scale(f_normalizer);
        }
        else if (layout == OC_LAYOUT_F32_3) {
            for (int i = 0; i < count; ++i)
                ((_f32_3 *)((char *)agents[i] + f_offset))->scale(f_normalizer);
        }
        else
            assert(false); /* unhandled layout */
    }

    void apply_force() {
        if (f_offset == NULL_OFFSET)
            return;
        if (layout == OC_LAYOUT_F32_2) {
            for (int i = 0; i < count; ++i)
                ((_f32_2 *)((char *)agents[i] + p_offset))->increase(((_f32_2 *)((char *)agents[i] + f_offset)));
        }
        else if (layout == OC_LAYOUT_F32_3) {
            for (int i = 0; i < count; ++i)
                ((_f32_3 *)((char *)agents[i] + p_offset))->increase(((_f32_3 *)((char *)agents[i] + f_offset)));
        }
        else
            assert(false); /* unhandled layout */
    }
};

struct OcLinkGroup : _Group {};

enum _HandlerType { NODE, NODE_NODE, LINK, LINK_LINK, LINK_NODE };

struct _Handler {
    _HandlerType type;
    unsigned phase;

    union {
        struct {
            OcNodeGroup *group;
            VPTR_FUNC func;
        } node_act;
        struct {
            OcNodeGroup *group1, *group2;
            VPTR_VPTR_FUNC func;
        } node_interact;
        struct {
            OcLinkGroup *group;
            VPTR_FUNC func;
        } link_act;
        struct {
            OcLinkGroup *group1;
            OcLinkGroup *group2;
            VPTR_VPTR_FUNC func;
        } link_interact;
        struct {
            OcLinkGroup *link_group;
            OcNodeGroup *node_group;
            VPTR_VPTR_FUNC func;
        } link_node_interact;
    };
};

struct OcState {
    OcNodeGroup node_groups[MAX_GROUPS];
    int node_groups_count;
    OcLinkGroup link_groups[MAX_GROUPS];
    int link_groups_count;
    _Handler handlers[MAX_HANDLERS];
    int handlers_count;
    void *exec_context;

    OcState() : node_groups_count(0), link_groups_count(0), handlers_count(0), exec_context(0) {}

    void clear() {
        clear_data();
        node_groups_count = 0;
        link_groups_count = 0;
        handlers_count = 0;
    }

    void clear_data() {
        for (int i = 0; i < node_groups_count; ++i)
            node_groups[i].clear();
        for (int i = 0; i < link_groups_count; ++i)
            link_groups[i].clear();
    }

    OcNodeGroup *add_node_group(unsigned f_offset, unsigned p_offset, OcLayout layout) {
        assert(node_groups_count < MAX_GROUPS - 1);
        OcNodeGroup *group = node_groups + node_groups_count++;
        group->f_offset = f_offset;
        group->p_offset = p_offset;
        group->layout = layout;
        return group;
    }

    OcNodeGroup *add_inert_node_group() {
        return add_node_group(NULL_OFFSET, NULL_OFFSET, OC_LAYOUT_F32_3);
    }

    OcLinkGroup *add_link_group() {
        assert(link_groups_count < MAX_GROUPS - 1);
        OcLinkGroup *group = link_groups + link_groups_count++;
        return group;
    }

    void add_node_act(OcNodeGroup *node_group, VPTR_FUNC func, unsigned phase) {
        assert(handlers_count < MAX_HANDLERS - 1);
        _Handler &h = handlers[handlers_count++];
        h.type = NODE;
        h.phase = phase;
        h.node_act.group = node_group;
        h.node_act.func = func;
    }

    void add_node_interact(OcNodeGroup *node_group1, OcNodeGroup *node_group2, VPTR_VPTR_FUNC func, unsigned phase) {
        assert(handlers_count < MAX_HANDLERS - 1);
        _Handler &h = handlers[handlers_count++];
        h.type = NODE_NODE;
        h.phase = phase;
        h.node_interact.group1 = node_group1;
        h.node_interact.group2 = node_group2;
        h.node_interact.func = func;
    }

    void add_link_act(OcLinkGroup *link_group, VPTR_FUNC func, unsigned phase) {
        assert(handlers_count < MAX_HANDLERS - 1);
        _Handler &h = handlers[handlers_count++];
        h.type = LINK;
        h.phase = phase;
        h.link_act.group = link_group;
        h.link_act.func = func;
    }

    void add_link_interact(OcLinkGroup *link_group1, OcLinkGroup *link_group2, VPTR_VPTR_FUNC func, unsigned phase) {
        assert(handlers_count < MAX_HANDLERS - 1);
        _Handler &h = handlers[handlers_count++];
        h.type = LINK_LINK;
        h.phase = phase;
        h.link_interact.group1 = link_group1;
        h.link_interact.group2 = link_group2;
        h.link_interact.func = func;
    }

    void add_link_node_interact(OcLinkGroup *link_group, OcNodeGroup *node_group, VPTR_VPTR_FUNC func, unsigned phase) {
        assert(handlers_count < MAX_HANDLERS - 1);
        _Handler &h = handlers[handlers_count++];
        h.type = LINK_NODE;
        h.phase = phase;
        h.link_node_interact.link_group = link_group;
        h.link_node_interact.node_group = node_group;
        h.link_node_interact.func = func;
    }

    bool run(int iterations) {
        for (int iteration = 0; iteration < iterations; ++iteration) {

            /* reset node forces */

            for (int i = 0; i < node_groups_count; ++i)
                node_groups[i].reset_force();

            /* calculate interaction forces */

            unsigned phase = 0;
            int handlers_done = 0;
            while (handlers_done < handlers_count) {

                for (int i = 0; i < handlers_count; ++i) {
                    _Handler &h = handlers[i];

                    if (h.phase == phase)
                        ++handlers_done;
                    else
                        continue;

                    if (h.type == NODE) {
                        OcNodeGroup *group = h.node_act.group;
                        VPTR_FUNC func = h.node_act.func;

                        for (int j = 0; j < group->count; ++j)
                            func(group->agents[j], exec_context);
                    }
                    else if (h.type == NODE_NODE) {
                        OcNodeGroup *group1 = h.node_interact.group1;
                        OcNodeGroup *group2 = h.node_interact.group2;
                        VPTR_VPTR_FUNC func = h.node_interact.func;

                        if (group1 == group2) {
                            for (int j = 0; j < group1->count - 1; ++j)
                                for (int k = j + 1; k < group1->count; ++k)
                                    func(group1->agents[j], group1->agents[k], exec_context);
                        }
                        else {
                            for (int j = 0; j < group1->count; ++j)
                                for (int k = 0; k < group2->count; ++k)
                                    func(group1->agents[j], group2->agents[k], exec_context);
                        }
                    }
                    else if (h.type == LINK) {
                        OcLinkGroup *group = h.link_act.group;
                        VPTR_FUNC func = h.link_act.func;

                        for (int j = 0; j < group->count; ++j)
                            func(group->agents[j], exec_context);
                    }
                    else if (h.type == LINK_LINK) {
                        OcLinkGroup *group1 = h.link_interact.group1;
                        OcLinkGroup *group2 = h.link_interact.group2;
                        VPTR_VPTR_FUNC func = h.link_interact.func;

                        if (group1 == group2) {
                            for (int j = 0; j < group1->count - 1; ++j)
                                for (int k = j + 1; k < group1->count; ++k)
                                    func(group1->agents[j], group1->agents[k], exec_context);
                        }
                        else {
                            for (int j = 0; j < group1->count; ++j)
                                for (int k = 0; k < group2->count; ++k)
                                    func(group1->agents[j], group2->agents[k], exec_context);
                        }
                    }
                    else if (h.type == LINK_NODE) {
                        OcLinkGroup *link_group = h.link_node_interact.link_group;
                        OcNodeGroup *node_group = h.link_node_interact.node_group;
                        VPTR_VPTR_FUNC func = h.link_node_interact.func;

                        for (int j = 0; j < link_group->count; ++j)
                            for (int k = 0; k < node_group->count; ++k)
                                func(link_group->agents[j], node_group->agents[k], exec_context);
                    }
                    else
                        assert(false); /* unhandled handler type */
                }

                ++phase;
            }

            /* normalize forces */

            float max_f = 0.0f;
            for (int i = 0; i < node_groups_count; ++i)
                max_f = node_groups[i].get_max_force(max_f);

            if (max_f < 0.01)
                return false;

            // TODO: extract coefficients of this expression into API and document it
            float f_normalizer = (max_f > 0.5f) ? (0.5f / max_f) : max_f; /* tweak to tune relaxation speed */

            for (int i = 0; i < node_groups_count; ++i)
                node_groups[i].normalize_force(f_normalizer);

            /* finalize objects */

            for (int i = 0; i < node_groups_count; ++i)
                node_groups[i].apply_force();
        }

        return true;
    }
} states[MAX_STATES];
unsigned states_count = 0;

OcState *ochre_add_state() {
    assert(states_count < MAX_STATES - 1);
    return states + states_count++;
}

void ochre_set_exec_context(OcState *state, void *exec_context) {
    assert(state >= states && state < states + states_count);
    state->exec_context = exec_context;
}

OcNodeGroup *ochre_add_node_group(OcState *state, unsigned f_offset, unsigned p_offset, OcLayout layout) {
    assert(state >= states && state < states + states_count);
    return state->add_node_group(f_offset, p_offset, layout);
}

OcNodeGroup *ochre_add_inert_node_group(OcState *state) {
    assert(state >= states && state < states + states_count);
    return state->add_inert_node_group();
}

OcLinkGroup *ochre_add_link_group(OcState *state) {
    assert(state >= states && state < states + states_count);
    return state->add_link_group();
}

void ochre_add_node_action(OcState *state, OcNodeGroup *node_group, VPTR_FUNC func, unsigned phase) {
    assert(state >= states && state < states + states_count);
    state->add_node_act(node_group, func, phase);
}

void ochre_add_link_action(OcState *state, OcLinkGroup *link_group, VPTR_FUNC func, unsigned phase) {
    assert(state >= states && state < states + states_count);
    state->add_link_act(link_group, func, phase);
}

void ochre_add_node_interaction(OcState *state, OcNodeGroup *node_group1, OcNodeGroup *node_group2, VPTR_VPTR_FUNC func, unsigned phase) {
    assert(state >= states && state < states + states_count);
    state->add_node_interact(node_group1, node_group2, func, phase);
}

void ochre_add_link_interaction(OcState *state, OcLinkGroup *link_group1, OcLinkGroup *link_group2, VPTR_VPTR_FUNC func, unsigned phase) {
    assert(state >= states && state < states + states_count);
    state->add_link_interact(link_group1, link_group2, func, phase);
}

void ochre_add_link_node_interaction(OcState *state, OcLinkGroup *link_group, OcNodeGroup *node_group, VPTR_VPTR_FUNC func, unsigned phase) {
    assert(state >= states && state < states + states_count);
    state->add_link_node_interact(link_group, node_group, func, phase);
}

void ochre_clear_data(OcState *state) {
    assert(state >= states && state < states + states_count);
    state->clear_data();
}

void ochre_add_node(OcNodeGroup *node_group, void *node) {
    node_group->add(node);
}

void ochre_add_link(OcLinkGroup *link_group, void *link) {
    link_group->add(link);
}

bool ochre_run(OcState *state, unsigned iterations) {
    assert(state >= states && state < states + states_count);
    return state->run(iterations);
}
