#include "platform.h"
#include "vec.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <assert.h>
#include <stdio.h>

#ifdef WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

GLFWwindow *_window;

bool quit;
bool cursor_hidden;
float w, h;
Mouse mouse;
bool skip_first_mousepos = true;

KEY_CALLBACK key_callback;
CHAR_CALLBACK char_callback;
MOUSEBUTTON_CALLBACK mousebutton_callback;
MOUSEPOS_CALLBACK mousepos_callback;
SCROLL_CALLBACK scroll_callback;


int _translate_key(int glwf_key) {
    switch (glwf_key) {
    case GLFW_KEY_ESCAPE:           return PLATFORM_KEY_ESCAPE;
    case GLFW_KEY_BACKSPACE:        return PLATFORM_KEY_BACKSPACE;
    case GLFW_KEY_DELETE:           return PLATFORM_KEY_DELETE;
    case GLFW_KEY_TAB:              return PLATFORM_KEY_TAB;
    case GLFW_KEY_ENTER:            return PLATFORM_KEY_ENTER;
    case GLFW_KEY_LEFT:             return PLATFORM_KEY_LEFT;
    case GLFW_KEY_RIGHT:            return PLATFORM_KEY_RIGHT;
    case GLFW_KEY_UP:               return PLATFORM_KEY_UP;
    case GLFW_KEY_DOWN:             return PLATFORM_KEY_DOWN;
    case GLFW_KEY_HOME:             return PLATFORM_KEY_HOME;
    case GLFW_KEY_END:              return PLATFORM_KEY_END;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:      return PLATFORM_KEY_SHIFT;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:    return PLATFORM_KEY_CONTROL;
    case GLFW_KEY_A:                return PLATFORM_KEY_A;
    case GLFW_KEY_B:                return PLATFORM_KEY_B;
    case GLFW_KEY_C:                return PLATFORM_KEY_C;
    case GLFW_KEY_D:                return PLATFORM_KEY_D;
    case GLFW_KEY_E:                return PLATFORM_KEY_E;
    case GLFW_KEY_F:                return PLATFORM_KEY_F;
    case GLFW_KEY_G:                return PLATFORM_KEY_G;
    case GLFW_KEY_H:                return PLATFORM_KEY_H;
    case GLFW_KEY_I:                return PLATFORM_KEY_I;
    case GLFW_KEY_J:                return PLATFORM_KEY_J;
    case GLFW_KEY_K:                return PLATFORM_KEY_K;
    case GLFW_KEY_L:                return PLATFORM_KEY_L;
    case GLFW_KEY_M:                return PLATFORM_KEY_M;
    case GLFW_KEY_N:                return PLATFORM_KEY_N;
    case GLFW_KEY_O:                return PLATFORM_KEY_O;
    case GLFW_KEY_P:                return PLATFORM_KEY_P;
    case GLFW_KEY_Q:                return PLATFORM_KEY_Q;
    case GLFW_KEY_R:                return PLATFORM_KEY_R;
    case GLFW_KEY_S:                return PLATFORM_KEY_S;
    case GLFW_KEY_T:                return PLATFORM_KEY_T;
    case GLFW_KEY_U:                return PLATFORM_KEY_U;
    case GLFW_KEY_V:                return PLATFORM_KEY_V;
    case GLFW_KEY_W:                return PLATFORM_KEY_W;
    case GLFW_KEY_X:                return PLATFORM_KEY_X;
    case GLFW_KEY_Y:                return PLATFORM_KEY_Y;
    case GLFW_KEY_Z:                return PLATFORM_KEY_Z;
    default:                        return PLATFORM_KEY_NONE;
    }
}

int _translate_mods(int glfw_mods) {
    int mods = 0;
    if (glfw_mods & GLFW_MOD_SHIFT)
        mods |= PLATFORM_MOD_SHIFT;
    if (glfw_mods & GLFW_MOD_CONTROL)
        mods |= PLATFORM_MOD_CTRL;
    return mods;
}

int _translate_action(int glfw_action) {
    if (glfw_action == GLFW_PRESS)
        return PLATFORM_PRESS;
    else if (glfw_action == GLFW_RELEASE)
        return PLATFORM_RELEASE;
    else if (glfw_action == GLFW_REPEAT)
        return PLATFORM_REPEAT;
}

int _translate_mousebutton(int glfw_mousebutton) {
    if (glfw_mousebutton == GLFW_MOUSE_BUTTON_LEFT)
        return PLATFORM_LEFT;
    else if (glfw_mousebutton == GLFW_MOUSE_BUTTON_RIGHT)
        return PLATFORM_RIGHT;
    else if (glfw_mousebutton == GLFW_MOUSE_BUTTON_MIDDLE)
        return PLATFORM_MIDDLE;
}

void _mousebutton_callback(GLFWwindow *, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse.left = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouse.right = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            mouse.middle = true;
    }
    else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse.left = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouse.right = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            mouse.middle = false;
    }

    mouse.travel = 0;

    if (mousebutton_callback)
        mousebutton_callback(_translate_mousebutton(button), _translate_action(action), _translate_mods(mods));
}

void _mousepos_callback(GLFWwindow *, double x, double y) {
    y = h - y;
    if (skip_first_mousepos)
        skip_first_mousepos = false;
    else if (mousepos_callback)
        mousepos_callback(x, y);
    mouse.pos.x = x;
    mouse.pos.y = y;
    ++mouse.travel;
}

void _scroll_callback(GLFWwindow *_window, double x, double y) {
    if (scroll_callback)
        scroll_callback(x, y);
}

void _key_callback(GLFWwindow *, int key, int code, int action, int mods) {
    if (key == GLFW_KEY_Q && mods & GLFW_MOD_CONTROL)
        quit = true;
    if (key_callback)
        key_callback(_translate_key(key), _translate_mods(mods), _translate_action(action));
}

void _char_callback(GLFWwindow *, unsigned int code) {
    if (char_callback)
        char_callback(code);
}

void _size_callback(GLFWwindow *_window, int w, int h) {
    w = w;
    h = h;
}

void _refresh_callback(GLFWwindow *_window) {}

void _close_callback(GLFWwindow *_window) {
    quit = true;
}

/*
** :public
*/

/* TODO: assert if a window was already created */
void plat_create_window(const char *title, int _w, int _h, bool fullscreen) {
    key_callback = 0;
    char_callback = 0;
    mousebutton_callback = 0;
    mousepos_callback = 0;
    scroll_callback = 0;
    quit = false;
    cursor_hidden = false;
    mouse.left = false;
    mouse.right = false;
    mouse.middle = false;

    if (!glfwInit()) {
        fprintf(stderr, "Could not init GLFW\n");
        return;
    }

    GLFWmonitor *monitor = 0;

    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        w = mode->width;
        h = mode->height;
    }
    else {
        w = _w;
        h = _h;
    }

    mouse.travel = 0;

    _window = glfwCreateWindow(w, h, title, monitor, 0);
    if (!_window) {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(_window);
    glfwSetKeyCallback(_window, _key_callback);
    glfwSetCharCallback(_window, _char_callback);
    glfwSetWindowSizeCallback(_window, _size_callback);
    glfwSetWindowRefreshCallback(_window, _refresh_callback);
    glfwSetMouseButtonCallback(_window, _mousebutton_callback);
    glfwSetCursorPosCallback(_window, _mousepos_callback);
    glfwSetScrollCallback(_window, _scroll_callback);
    glfwSetWindowCloseCallback(_window, _close_callback);
    glfwSetTime(0.0);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1, 1);
}

/* TODO: assert if a window is already running */
void plat_run_window(MAIN_LOOP_FUNC main_loop_func) {
    while (!quit)
        main_loop_func();
    glfwDestroyWindow(_window);
    glfwTerminate();
}

vec2 plat_get_screen() {
    vec2 v;
    v.x = w;
    v.y = h;
    return v;
}

Mouse &plat_get_mouse() {
    return mouse;
}

void plat_set_cursor_hidden(bool hidden) {
    if ((cursor_hidden = hidden))
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void plat_set_key_callback(KEY_CALLBACK callback) {
    key_callback = callback;
}

void plat_set_char_callback(CHAR_CALLBACK callback) {
    char_callback = callback;
}

void plat_set_mousebutton_callback(MOUSEBUTTON_CALLBACK callback) {
    mousebutton_callback = callback;
}

void plat_set_mousepos_callback(MOUSEPOS_CALLBACK callback) {
    mousepos_callback = callback;
}

void plat_set_scroll_callback(SCROLL_CALLBACK callback) {
    scroll_callback = callback;
}

void plat_poll_events() {
    glfwPollEvents();
}

void plat_swap_buffers() {
    glfwSwapBuffers(_window);
}

void plat_set_time() {
    glfwSetTime(0.0);
}

long long int plat_get_time() {
    return (long long int)(glfwGetTime() * 1000000.0);
}

void plat_make_context_current() {
    glfwMakeContextCurrent(_window);
}

void plat_sleep(int milliseconds) {
#ifdef WIN
	Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void *plat_fopen(const char *path, const char *mode) {
    FILE *f = 0;
#ifdef WIN
    fopen_s(&f, path, mode);
#else
    f = fopen(path, mode);
#endif
    assert(f != 0);
    return f;
}
