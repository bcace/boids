#ifndef group_h
#define group_h

#define GROUP_MAX_OBJS ((1 << (sizeof(unsigned char) * 8)) - 1)


typedef bool (*GROUP_FUNC)(void *, void *);

struct Group {
    int count;
    unsigned char *obj_indices; /* points into storage */
};

struct GroupMaker {
    Group groups[GROUP_MAX_OBJS];
    int count;
    unsigned char storage[GROUP_MAX_OBJS];
};

void group_objects(int size, void *objs, int count, GROUP_FUNC func, GroupMaker *maker);

#endif
