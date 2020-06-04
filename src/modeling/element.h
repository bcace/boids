#ifndef element_h
#define element_h


/* Used to determine flags overhead needed for storing clone id flags. */
#define MAX_SYMMETRIES 2

/* Maximum number of elements (objects, wings) in a model. */
#define MAX_ELEMS (sizeof(FlagPart) * 8)

/* Maximum number of element references (original objects and their symmetry clones). */
#define MAX_ELEM_REFS (MAX_ELEMS * MAX_SYMMETRIES)


/* Object or wing id, unique for the model. */
typedef short int Id;

/* Same as above in flag form, and only for original objects (no symmetry clones). */
typedef unsigned long long int FlagPart;

/* Id flags for model elements. */
struct Flags {
    union {
        FlagPart data[MAX_SYMMETRIES];
        struct { /* aliases for convenience, only valid if MAX_SYMMETRIES is 2 */
            FlagPart f0, f1;
        };
    };
};

/* Ids of tailwise and nosewise objects. */
struct Ids {
    Id tail;
    Id nose;
};

void flags_init(Flags *f);
void flags_add(Flags *f, int i);
void flags_add_flags(Flags *f, Flags *o);
bool flags_contains(Flags *f, int i);
bool flags_and(Flags *a, Flags *b);
bool flags_has_anything_other_than(Flags *f, Flags *o);
bool flags_is_empty(Flags *f);
Flags flags_zero();
Flags flags_make(int i);

#endif
