#ifndef debug_h
#define debug_h


#ifdef NDEBUG
    #define break_assert(__expr__) ((void)0)
    #define model_assert(__model__, __expr__, __label__) ((void)0)
#else
    void _break_assert_func(bool expr);
    void _model_assert_func(struct Model *model, bool expr, const char *label);

    #define break_assert(__expr__) _break_assert_func((__expr__))
    #define model_assert(__model__, __expr__, __label__) _model_assert_func((__model__), (__expr__), (__label__))
#endif

#endif
