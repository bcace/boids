#ifndef serial_h
#define serial_h


struct Serial {
    char *data;
    int read;
    int written;
    int capacity;
};

Serial serial_make(char *_data, int _capacity);
void serial_clear(Serial *s);

void serial_write_i32(Serial *s, int *v, int count);
void serial_write_f32(Serial *s, float *v, int count);
void serial_write_f64(Serial *s, double *v, int count);

void serial_read_i32(Serial *s, int *v, int count);
void serial_read_f32(Serial *s, float *v, int count);
void serial_read_f64(Serial *s, double *v, int count);

void serial_write_to_file(Serial *s, const char *path);
void serial_read_from_file(Serial *s, const char *path);

#endif
