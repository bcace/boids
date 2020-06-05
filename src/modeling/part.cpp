#include "part.h"
#include "object.h"
#include "../platform.h"
#include <string.h>
#include <assert.h>


void part_init(Part *part, const char *name, float tail_endp_dx, float nose_endp_dx, bool tail_opening, bool nose_opening) {
    assert(tail_endp_dx > 0.01);
    assert(nose_endp_dx > 0.01);
    assert(strlen(name) < MAX_PROTO_NAME);
#ifdef PLATFORM_WIN
    strcpy_s(part->name, name);
#else
    strcpy(part->name, name);
#endif
    part->formers_count = 0;
    part->tail_endp_dx = tail_endp_dx;
    part->nose_endp_dx = nose_endp_dx;
    part->tail_opening = tail_opening;
    part->nose_opening = nose_opening;
}

void part_add_coll_former(Part *part, Shape shape, float x) {
    assert(part->formers_count == 0 || x > part->formers[part->formers_count - 1].x); // new former must be in front of the old one
    Former *f = part->formers + part->formers_count++;
    f->shape = shape;
    f->x = x;
}

void part_set_skin_formers(Part *part, Shape tail_shape, float tail_x, Shape nose_shape, float nose_x) {
    part->tail_skin_former.shape = tail_shape;
    part->tail_skin_former.x = tail_x;
    part->nose_skin_former.shape = nose_shape;
    part->nose_skin_former.x = nose_x;
}

Object *part_make_object(Part *part, vec3 p) {
    Object *o = new Object();
    o->p = p;

    for (int i = 0; i < part->formers_count; ++i)
        o->formers[i] = part->formers[i];
    o->formers_count = part->formers_count;

    o->tail_skin_former = part->tail_skin_former;
    o->nose_skin_former = part->nose_skin_former;
    o->tail_endp_dx = part->tail_endp_dx;
    o->nose_endp_dx = part->nose_endp_dx;

    object_finish(o);

    return o;
}
