#ifndef arena_h
#define arena_h

#define MAX_LOCK_STACK 8


struct Arena {
    char *data;
    int capacity;
    int taken;
    int lock_stack[MAX_LOCK_STACK];
    int locked_stack; // TODO: rename to lock_stack_level

    Arena(int _capacity);
    ~Arena();

    void clear();
    void unlock();
    char *alloc_bytes(int bytes, bool zero);
    char *lock_bytes(int bytes);

    template<typename T>
    T *alloc(int count=1, bool zero=false) {
        return (T *)alloc_bytes(sizeof(T) * count, zero);
    }

    template<typename T>
    T *lock(int count=1) {
        return (T *)lock_bytes(sizeof(T) * count);
    }

    template<typename T>
    T *rest() {
        return (T *)(data + taken);
    }
};

#endif
