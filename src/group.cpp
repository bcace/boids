#include "group.h"
#include <string.h>
#include <assert.h>


void group_objects(int size, void *objs, int count, GROUP_FUNC func, GroupMaker *maker) {
    assert(count <= GROUP_MAX_OBJS);

    maker->count = 0;

    /* propagate group ids */

    int group_ids[GROUP_MAX_OBJS]; /* maps object index to group id */
    for (int i = 0; i < GROUP_MAX_OBJS; ++i)
        group_ids[i] = -1;

    int next_available_group_id = 0;

    for (int a_i = 0; a_i < count; ++a_i) {
        void *a_o = (char *)objs + a_i * size;

        if (group_ids[a_i] == -1)
            group_ids[a_i] = next_available_group_id++;

        for (int b_i = a_i + 1; b_i < count; ++b_i) {
            void *b_o = (char *)objs + b_i * size;

            if (!func(a_o, b_o))
                continue;

            if (group_ids[b_i] == -1)
                group_ids[b_i] = group_ids[a_i];
            else if (group_ids[b_i] != group_ids[a_i]) {
                int b_group_id = group_ids[b_i];
                for (int c_i = 0; c_i < count; ++c_i)
                    if (group_ids[c_i] == b_group_id)
                        group_ids[c_i] = group_ids[a_i];
            }
        }
    }

    /* create blank groups from group ids */

    Group *group_map[GROUP_MAX_OBJS]; /* maps group id to group */
    memset(group_map, 0, sizeof(Group *) * GROUP_MAX_OBJS);

    for (int i = 0; i < count; ++i) {
        Group *g = group_map[group_ids[i]];
        if (g == 0) {
            g = maker->groups + maker->count++;
            group_map[group_ids[i]] = g;
            g->count = 0;
            g->obj_indices = 0;
        }
        ++g->count;
    }

    /* initialize groups' indices pointer */

    int offset = 0;
    for (int i = 0; i < maker->count; ++i) {
        Group *g = maker->groups + i;
        g->obj_indices = maker->storage + offset;
        offset += g->count;
        g->count = 0; /* reset so object indices can be set */
    }

    /* set object indices */

    for (int i = 0; i < count; ++i) {
        Group *g = group_map[group_ids[i]];
        g->obj_indices[g->count++] = i;
    }
}
