#include "modeling_id.h"


static const int _BUCKET = MAX_ELEMS;

static inline unsigned long long int _to_subflag(int i) {
    return 1ull << i;
}

void flags_init(Flags *f) {
    f->f0 = f->f1 = 0ull;
}

void flags_add(Flags *f, int i) {
    if (i < _BUCKET)
        f->f0 |= _to_subflag(i);
    else
        f->f1 |= _to_subflag(i - _BUCKET);
}

void flags_add_flags(Flags *f, Flags *o) {
    f->f0 |= o->f0;
    f->f1 |= o->f1;
}

bool flags_contains(Flags *f, int i) {
    if (i < _BUCKET)
        return f->f0 & _to_subflag(i);
    else
        return f->f1 & _to_subflag(i - _BUCKET);
}

bool flags_has_anything_other_than(Flags *f, Flags *o) {
    return (f->f0 & ~o->f0) || (f->f1 & ~o->f1);
}

bool flags_and(Flags *a, Flags *b) {
    return (a->f0 & b->f0) || (a->f1 & b->f1);
}

bool flags_is_empty(Flags *f) {
    return f->f0 == 0ull && f->f1 == 0ull;
}

Flags flags_zero() {
    Flags f;
    f.f0 = f.f1 = 0ull;
    return f;
}

Flags flags_make(int i) {
    Flags f;
    f.f0 = f.f1 = 0ull;
    flags_add(&f, i);
    return f;
}
