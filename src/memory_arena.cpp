#include "memory_arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


Arena::Arena(int _capacity) {
    capacity = _capacity;
    data = (char *)malloc(capacity);
    taken = 0;
    locked_stack = 0;
}

Arena::~Arena() {
    free(data);
}

void Arena::clear() {
    taken = 0;
    locked_stack = 0;
}

void Arena::unlock() {
    if (locked_stack > 0)
        taken -= lock_stack[--locked_stack];
}

char *Arena::alloc_bytes(int bytes, bool zero) {
    assert(taken + bytes < capacity);
    assert(locked_stack == 0);
    char *mem = data + taken;
    taken += bytes;
    if (zero)
        memset(mem, 0, bytes);
    return mem;
}

char *Arena::lock_bytes(int bytes) {
    assert(taken + bytes < capacity);
    char *mem = data + taken;
    taken += bytes;
    lock_stack[locked_stack++] = bytes;
    return mem;
}
