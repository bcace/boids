#include "debug.h"

#ifndef NDEBUG

#include "modeling/model.h"
#include <assert.h>
#include <string.h>


/* Gives you opportunity to place a breakpoint on assert. */
void _break_assert_func(bool expr) {
    if (!expr) {
        assert(false);
    }
}

/* Serializes model before asserting. Label is used as dump file name. */
void _model_assert_func(Model *model, bool expr, const char *label) {
    if (!expr) {
        char path[512];
        strcpy_s(path, label);
        strcat_s(path, ".dump");
        model_serialize(model, path);
        assert(false);
    }
}

#endif
