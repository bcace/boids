#include "math_periodic.h"


int period_offset_if_contains(int i, int beg, int end, int period) {
    if (beg <= end)
        return (i >= beg && i <= end) ? (i - beg) : -1;
    if (i <= end)
        return i + period - beg;
    if (i >= beg)
        return i - beg;
    return -1;
}

int period_range_count(int beg, int end, int period) {
    if (beg <= end)
        return end - beg + 1;
    else
        return end + period - beg + 1;
}

int period_incr(int i, int period) {
    return (i == (period - 1)) ? 0 : (i + 1);
}

int period_decr(int i, int period) {
    return (i == 0) ? (period - 1) : (i - 1);
}

int period_diff(int a, int b, int period) {
    int d = a - b;
    if (d > period / 2)
        return d - period;
    if (d < -period / 2)
        return d + period;
    return d;
}
