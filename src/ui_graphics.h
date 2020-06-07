#ifndef graphics_h
#define graphics_h


struct ShaderProgram {
    unsigned prog, vao;
    unsigned vbos[32];
    int vbo_count;
    unsigned uniforms[32];
    int uniforms_count;

    void init(const char *vert_src, const char *frag_src);
    void use();

    void define_uniform(const char *name);
    void set_uniform_int(int uniform_index, int v);
    void set_uniform_float(int uniform_index, float v);
    void set_uniform_vec3(int uniform_index, const struct vec3 &vec);
    void set_uniform_vec4(int uniform_index, const struct vec4 &vec);
    void set_uniform_mat4(int uniform_index, const struct mat4 &mat);

    void _define_in(int components, int type);
    void define_in_float(int components);

    void _set_data(int vbo_index, int count, void *data, int size);
    template<typename T> void set_data(int vbo_index, int count, void *data) {
        _set_data(vbo_index, count, data, sizeof(T));
    }
};

void graph_clear(vec3 color);
void graph_clear_depth();
void graph_line_width(float width);
void graph_viewport(int x, int y, int w, int h);
void graph_read_pixels(int x, int y, int w, int h, unsigned char *rgba);
void graph_read_depth(int x, int y, int w, int h, float *rgba);
void graph_set_polygon_line_mode(bool line_mode);

void graph_ortho(struct mat4 &m, float left, float right, float bottom, float top, float near, float far);
void graph_perspective(struct mat4 &m, float fov, float aspect, float near, float far);
void graph_lookat(struct mat4 &m, vec3 pos, vec3 fwd, vec3 up);

void graph_enable_blend(bool enable);
void graph_enable_smooth_line(bool enable);
void graph_enable_smooth_polygon(bool enable);
void graph_enable_depth_test(bool enable);
void graph_enable_depth_mask(bool enable);

void graph_draw_lines(int vertex_count, int first=0);
void graph_draw_line_loop(int vertex_count, int first=0);
void graph_draw_line_strip(int vertex_count, int first=0);
void graph_draw_lines_indexed(int index_count, int *indices);
void graph_draw_triangles(int vertex_count, int first=0);
void graph_draw_triangle_fan(int vertex_count, int first=0);
void graph_draw_triangles_indexed(int index_count, int *indices);
void graph_draw_quads_indexed(int index_count, int *indices);

unsigned graph_create_texture(unsigned char *data, int texture_w, int texture_h);

void graph_print_error();

#endif
