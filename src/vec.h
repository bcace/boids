#ifndef vec_h
#define vec_h


struct vec2 {
    union {
        struct {
            float x, y;
        };
        struct {
            float a, b;
        };
        float v[2];
    };
};

struct vec3 {
    union {
        struct {
            float x, y, z;
        };
        struct {
            float r, g, b;
        };
        float v[3];
    };

    vec3();
    vec3(float x, float y, float z);
    vec3(const struct vec2 &v, float z);
    vec3(float x, const struct vec2 &v);

    void set();
    void set(float _x, float _y, float _z);
    vec3 &normalize();
    float length();
    float dot(vec3 v);
    vec3 cross(vec3 v);
    vec3 operator-();
    vec3 operator+(vec3 v);
    vec3 operator-(vec3 v);
    vec3 operator*(float v);
    vec3 operator/(float v);
    vec3 &operator+=(vec3 v);
    vec3 &operator-=(vec3 v);
    vec3 &operator*=(float v);
    vec3 &operator*=(vec3 v);
    bool operator==(vec3 v);
    bool operator!=(vec3 v);
};

struct vec4 {
    union {
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
        float v[4];
    };

    vec4();
    vec4(float _x, float _y, float _z, float _w);
    vec4(vec3 v, float _w);
};

struct box3 {
    vec3 min, max;

    box3();
    box3(float minx, float miny, float minz, float maxx, float maxy, float maxz);

    void include(float x, float y, float z);
    void include(vec3 v);
    void include(box3 o);
    bool intersects(box3 o);
    box3 &grow(vec3 v);
    box3 &grow(float x, float y, float z);
};

struct ivec2 {
    union {
        struct {
            int x, y;
        };
        struct {
            int i, j;
        };
        int v[2];
    };

    ivec2();
    ivec2(int _i, int _j);
};

#endif
