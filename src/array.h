#ifndef array_h
#define array_h

#include "debug.h"


template<typename T, int S>
struct array {
    int count, size;
    T data[S];

    array() : count(0), size(S) {}

    void add(const T &v) {
        break_assert(count < S - 1);
        data[count++] = v;
    }

    T &add() {
        break_assert(count < S - 1);
        return data[count++];
    }

    void pop() {
        if (count > 0)
            --count;
    }

    T &insert(int index) {
        for (int i = count - 1; i >= index; --i)
            data[i + 1] = data[i];
        ++count;
        return data[index];
    }

    void remove_at(int i) {
        if (i < 0 || i >= count)
            return;
        --count;
        for (int j = i; j < count; ++j)
            data[j] = data[j + 1];
    }

    void clear() {
        count = 0;
    }

    T &operator[](int i) {
        break_assert(i >= 0 && i < count);
        return data[i];
    }

    T &first() {
        break_assert(count > 0);
        return data[0];
    }

    T &last() {
        break_assert(count > 0);
        return data[count - 1];
    }

    typedef int (*COMP_FUNC)(T &a, T &b);

    void sort(COMP_FUNC comp_func) {
        for (int i = 1; i < count; ++i) {

            /* find first smaller item below current item */
            int smaller_i = i - 1;
            for (; smaller_i >= 0 && comp_func(data[i], data[smaller_i]) == -1; --smaller_i);

            /* element to be moved */
            T temp = data[i];

            /* move elements between remove and insert points up one spot */
            for (int j = i - 1; j > smaller_i; --j)
                data[j + 1] = data[j];

            /* insert moved element */
            data[smaller_i + 1] = temp;
        }
    }

    bool contains(T &o, COMP_FUNC comp_func) {
        for (int i = 0; i < count; ++i)
            if (comp_func(o, data[i]) == 0)
                return true;
        return false;
    }
};

#endif
