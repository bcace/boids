#ifndef ref_h
#define ref_h

/*
Pointer that can be turned into an id and back.
*/

template<typename T>
struct Ref {
    explicit Ref() : ptr(0) {}
    explicit Ref(int _id) : id(_id) {}
    explicit Ref(T *_ptr) : ptr(_ptr) {}

    union {
        int id;
        T *ptr;
    };

    bool is_null() {
        return ptr == 0;
    }
};

#endif
