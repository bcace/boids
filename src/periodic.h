#ifndef periodic_h
#define periodic_h


/* Returns offset of i from beg if [beg, end] range contains i, otherwise returns -1. */
int period_offset_if_contains(int i, int beg, int end, int period);

/* Returns [beg, end] range elements count, including beg and end. */
int period_range_count(int beg, int end, int period);

/* Returns i incremented by one and wrapped around if it overflows. */
int period_incr(int i, int period);

/* Returns i decremented by one and wrapped around if it underflows. */
int period_decr(int i, int period);

/* Returns the shorter a - b difference, signed. */
int period_diff(int a, int b, int period);

#endif
