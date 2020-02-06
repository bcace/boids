#ifndef vector_h
#define vector_h

#include <stdlib.h>
#include <assert.h>


template<typename T>
struct vector {
    int count, size;
    T *data;

    vector() : count(0), size(0), data(0) {}
    ~vector() { free(data); }

    void _reserve(int reserve_count) {
        if (size < (count + reserve_count)) {
            size *= 2;
            if (size < (count + reserve_count))
                size = count + reserve_count;
            data = (T *)realloc(data, sizeof(T) * size);
        }
    }

    void _reserve_for_insert(int reserve_count, int index) {
        if (index > count)
            index = count;
        _reserve(reserve_count);
        for (int i = count - 1; i >= index; --i)
            data[i + reserve_count] = data[i];
    }

    T &add() {
        _reserve(1);
        return data[count++];
    }

    void add(const T &v) {
        _reserve(1);
        data[count++] = v;
    }

    void reserve(int reserve_count) {
        _reserve(reserve_count);
        count += reserve_count;
    }

    T &first() {
        return data[0];
    }

    T &last() {
        return data[count - 1];
    }

    T &insert(int index) {
        _reserve_for_insert(1, index);
        ++count;
        return data[index];
    }

    void remove_at(int i) {
        if (i >= count) return;
        --count;
        for (int j = i; j < count; ++j)
            data[j] = data[j + 1];
    }

    void pop() {
        if (count > 0)
            --count;
    }

    void clear() {
        count = 0;
    }

    T &operator[](int i) { return data[i]; }

    vector &operator=(const vector &o) {
        count = o.count;
        size = o.size;
        if (o.data) {
            data = (T *)realloc(data, sizeof(T) * size);
            for (int i = 0; i < count; ++i)
                data[i] = o.data[i];
        }
        else {
            free(data);
            data = 0;
        }
        return *this;
    }

    void take(vector &o) {
        free(data);
        count = o.count;
        size = o.size;
        data = o.data;
        o.data = 0;
        o.count = o.size = 0;
    }

    int index_of(const T &d) {
        for (int i = 0; i < count; ++i)
            if (d == data[i])
                return i;
        return -1;
    }

    void remove_all(const T &d) {
        for (int i = index_of(d); i != -1; i = index_of(d))
            remove_at(i);
    }
};

#endif
