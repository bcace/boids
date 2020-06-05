#include "graphics.h"
#include "platform.h"
#include "vec.h"
#include "mat.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


char *_read_file(const char *path) {
    FILE *f = (FILE *)platform_fopen(path, "r");
    fseek(f, 0, SEEK_END); // TODO: fseek and ftell don't always return the number of characters because of the goddamn carriage return crap...
    size_t size = ftell(f);
    char *text = (char *)malloc(size + 1);
    rewind(f);
    size_t ignore = fread(text, 1, size, f);
    text[size] = '\0';
    fclose(f);
    return text;
}

void _create_shader(GLuint program, const char *path, GLenum type) {
    char *text = _read_file(path);

    GLuint shader = glCreateShader(type);
    const GLchar *sources[1] = { text };
    glShaderSource(shader, 1, sources, 0);
    glCompileShader(shader);

    free(text);

    GLint status;
    GLchar error_message[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderInfoLog(shader, 1024, 0, error_message);
        fprintf(stderr, "Error in %s: %s\n", path, error_message);
    }

    glAttachShader(program, shader);
}

void ShaderProgram::init(const char *vert_src, const char *frag_src) {
    vbo_count = 0;
    prog = glCreateProgram();
    _create_shader(prog, vert_src, GL_VERTEX_SHADER);
    _create_shader(prog, frag_src, GL_FRAGMENT_SHADER);
    glLinkProgram(prog);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
}

void ShaderProgram::use() {
    glUseProgram(prog);
    glBindVertexArray(vao);
}

void ShaderProgram::_define_in(int components, int type) {
    glGenBuffers(1, &vbos[vbo_count]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[vbo_count]);
    glVertexAttribPointer(vbo_count, components, type, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vbo_count);
    ++vbo_count;
}

void ShaderProgram::define_in_float(int components) {
    _define_in(components, GL_FLOAT);
}

void ShaderProgram::define_uniform(const char *name) {
    uniforms[uniforms_count] = glGetUniformLocation(prog, name);
    ++uniforms_count;
}

void ShaderProgram::set_uniform_int(int uniform_index, int v) {
    assert(uniform_index < uniforms_count);
    glUniform1i(uniforms[uniform_index], v);
}

void ShaderProgram::set_uniform_float(int uniform_index, float v) {
    assert(uniform_index < uniforms_count);
    glUniform1f(uniforms[uniform_index], v);
}

void ShaderProgram::set_uniform_vec3(int uniform_index, const vec3 &vec) {
    assert(uniform_index < uniforms_count);
    glUniform3f(uniforms[uniform_index], vec.x, vec.y, vec.z);
}

void ShaderProgram::set_uniform_vec4(int uniform_index, const vec4 &vec) {
    assert(uniform_index < uniforms_count);
    glUniform4f(uniforms[uniform_index], vec.x, vec.y, vec.z, vec.w);
}

void ShaderProgram::set_uniform_mat4(int uniform_index, const mat4 &mat) {
    assert(uniform_index < uniforms_count);
    glUniformMatrix4fv(uniforms[uniform_index], 1, GL_FALSE, (float *)&mat);
}

void ShaderProgram::_set_data(int vbo_index, int count, void *data, int size) {
    assert(vbo_index < vbo_count);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[vbo_index]);
    glBufferData(GL_ARRAY_BUFFER, count * size, data, GL_STATIC_DRAW);
}

/* :projection */

void graph_ortho(mat4 &m, float left, float right, float bottom, float top, float _near, float _far) {
    m.set_identity();
    m.v[0][0] = 2 / (right - left);
    m.v[1][1] = 2 / (top - bottom);
    m.v[3][0] = -(right + left) / (right - left);
    m.v[3][1] = -(top + bottom) / (top - bottom);
    m.v[2][2] = -1 / (_far - _near);
    m.v[3][2] = -_near / (_far - _near);
}

void graph_perspective(mat4 &m, float fov, float aspect, float _near, float _far) {
    fov = tan(fov * 0.5);
    m.set_identity();
    m.v[0][0] = 1.0 / (aspect * fov);
    m.v[1][1] = 1.0 / fov;
    m.v[2][3] = -1;
    m.v[2][2] = _far / (_near - _far);
    m.v[3][2] = -(_far * _near) / (_far - _near);
}

void graph_lookat(mat4 &m, vec3 pos, vec3 fwd, vec3 up) {
    fwd.normalize();
    vec3 side = fwd.cross(up);
    side.normalize();
    up = side.cross(fwd);
    m.v[0][3] = m.v[1][3] = m.v[2][3] = 0;
    m.v[3][3] = 1;
    m.v[0][0] = side.x;
    m.v[1][0] = side.y;
    m.v[2][0] = side.z;
    m.v[0][1] = up.x;
    m.v[1][1] = up.y;
    m.v[2][1] = up.z;
    m.v[0][2] = -fwd.x;
    m.v[1][2] = -fwd.y;
    m.v[2][2] = -fwd.z;
    m.v[3][0] = -side.dot(pos);
    m.v[3][1] = -up.dot(pos);
    m.v[3][2] = fwd.dot(pos);
}

/* :opengl api */

void graph_viewport(int x, int y, int w, int h) {
    glViewport(x, y, w, h);
}

void graph_clear(vec3 color) {
    glClearColor(color.r, color.g, color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graph_clear_depth() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void graph_draw_lines(int vertex_count, int first) {
    glDrawArrays(GL_LINES, first, vertex_count);
}

void graph_draw_line_loop(int vertex_count, int first) {
    glDrawArrays(GL_LINE_LOOP, first, vertex_count);
}

void graph_draw_line_strip(int vertex_count, int first) {
    glDrawArrays(GL_LINE_STRIP, first, vertex_count);
}

void graph_draw_lines_indexed(int index_count, int *indices) {
    glDrawElements(GL_LINES, index_count, GL_UNSIGNED_INT, indices);
}

void graph_draw_triangles(int vertex_count, int first) {
    glDrawArrays(GL_TRIANGLES, first, vertex_count);
}

void graph_draw_triangle_fan(int vertex_count, int first) {
    glDrawArrays(GL_TRIANGLE_FAN, first, vertex_count);
}

void graph_draw_triangles_indexed(int index_count, int *indices) {
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, indices);
}

void graph_draw_quads_indexed(int index_count, int *indices) {
    glDrawElements(GL_QUADS, index_count, GL_UNSIGNED_INT, indices);
}

void graph_print_error() {
    GLenum error = glGetError();
    if (error == GL_INVALID_ENUM)
        fprintf(stderr, "%s\n", "GL_INVALID_ENUM");
    else if (error == GL_INVALID_VALUE)
        fprintf(stderr, "%s\n", "GL_INVALID_VALUE");
    else if (error == GL_INVALID_OPERATION)
        fprintf(stderr, "%s\n", "GL_INVALID_OPERATION");
    else if (error == GL_STACK_OVERFLOW)
        fprintf(stderr, "%s\n", "GL_STACK_OVERFLOW");
    else if (error == GL_STACK_UNDERFLOW)
        fprintf(stderr, "%s\n", "GL_STACK_UNDERFLOW");
    else if (error == GL_OUT_OF_MEMORY)
        fprintf(stderr, "%s\n", "GL_OUT_OF_MEMORY");
    else if (error)
        fprintf(stderr, "%s: %d\n", "unhandled error", (int)error);
}

unsigned graph_create_texture(unsigned char *data, int source_w, int source_h) {
    // texture dimensions must be 2^n
    int texture_w = 1024, texture_h = 16; // TODO: calculate these from source_w and source_h

    unsigned char *texture = (unsigned char *)malloc((texture_w * texture_h) * 4);
    int t = 0, d = 0;
    for (int i = 0; i < source_h; ++i) {
        for (int j = 0; j < source_w; ++j) {
            texture[t++] = data[d++];
            texture[t++] = data[d++];
            texture[t++] = data[d++];
            texture[t++] = data[d++];
        }
        t += (texture_w - source_w) * 4;
    }

    unsigned texture_id;

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_w, texture_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);

    free(texture);

    return texture_id;
}

void graph_line_width(float width) {
    glLineWidth(width);
}

void graph_read_pixels(int x, int y, int w, int h, unsigned char *rgba) {
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

void graph_read_depth(int x, int y, int w, int h, float *depth) {
    glReadPixels(x, y, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
}

void graph_set_polygon_line_mode(bool line_mode) {
    if (line_mode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void graph_enable_blend(bool enable) {
    if (enable)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void graph_enable_smooth_line(bool enable) {
    if (enable)
        glEnable(GL_LINE_SMOOTH);
    else
        glDisable(GL_LINE_SMOOTH);
}

void graph_enable_smooth_polygon(bool enable) {
    if (enable)
        glEnable(GL_POLYGON_SMOOTH);
    else
        glDisable(GL_POLYGON_SMOOTH);
}

void graph_enable_depth_test(bool enable) {
    if (enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void graph_enable_depth_mask(bool enable) {
    glDepthMask(enable);
}
