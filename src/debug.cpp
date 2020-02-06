#include "debug.h"
#include "model.h"
#include <assert.h>


#ifndef NDEBUG

/* Gives you opportunity to place a breakpoint on assert. */
void _break_assert_func(bool expr) {
    if (!expr) {
        assert(false);
    }
}

/* Serializes model before asserting. Label is used as dump file name. */
void _model_assert_func(Model *model, bool expr, const char *label) {
    if (!expr) {
        model_serialize(model, label);
        assert(false);
    }
}

#endif
