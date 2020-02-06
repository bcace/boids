#include "serial.h"
#include "platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


Serial serial_make(char *_data, int _capacity) {
    Serial s;
    s.data = _data;
    s.read = 0;
    s.written = 0;
    s.capacity = _capacity;
    return s;
}

void serial_clear(Serial *s) {
    s->read = 0;
    s->written = 0;
}

void _write(Serial *s, void *v, int bytes) {
    assert(s->written + bytes < s->capacity);
    memcpy(s->data + s->written, v, bytes);
    s->written += bytes;
}

void serial_write_i32(Serial *s, int *v, int count) {
    _write(s, v, sizeof(int) * count);
}

void serial_write_f32(Serial *s, float *v, int count) {
    _write(s, v, sizeof(float) * count);
}

void serial_write_f64(Serial *s, double *v, int count) {
    _write(s, v, sizeof(double) * count);
}

void _read(Serial *s, void *v, int bytes) {
    assert(s->read + bytes < s->capacity);
    memcpy(v, s->data + s->read, bytes);
    s->read += bytes;
}

void serial_read_i32(Serial *s, int *v, int count) {
    return _read(s, v, sizeof(int) * count);
}

void serial_read_f32(Serial *s, float *v, int count) {
    return _read(s, v, sizeof(float) * count);
}

void serial_read_f64(Serial *s, double *v, int count) {
    return _read(s, v, sizeof(double) * count);
}

void serial_write_to_file(Serial *s, const char *path) {
    FILE *f = (FILE *)plat_fopen(path, "wb");
    fwrite(s->data, sizeof(char), s->written, f);
    fclose(f);
}

void serial_read_from_file(Serial *s, const char *path) {
    FILE *f = (FILE *)plat_fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    s->read = 0;
    s->written = ftell(f);
    assert(s->written < s->capacity);
    fseek(f, 0, SEEK_SET);
    size_t ignore = fread(s->data, sizeof(char), s->written, f);
    fclose(f);
}
