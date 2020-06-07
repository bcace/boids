#ifndef group_h
#define group_h

#define GROUP_MAX_OBJS (1 << (sizeof(unsigned char) * 8))


typedef bool (*GROUP_FUNC)(void *, void *);

struct Group {
    int count; /* number of objects in this group */
    unsigned char *obj_indices; /* points into storage */
};

struct GroupMaker {
    Group groups[GROUP_MAX_OBJS];
    int count;
    unsigned char storage[GROUP_MAX_OBJS];
};

/* Given an array of objects and grouping function, returns information on how many groups there are
and how many objects are in each groups and which ones they are. */
void group_objects(int size, void *objs, int count, GROUP_FUNC func, GroupMaker *maker);

#endif
