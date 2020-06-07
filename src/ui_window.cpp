#include "ui_window.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <assert.h>


GLFWwindow *glfw_window = 0;

bool quit;
bool cursor_hidden;
bool skip_first_mousepos = true;
Window window;

WINDOW_KEY_CB key_callback;
WINDOW_CHAR_CB char_callback;
WINDOW_MOUSEBUTTON_CB mousebutton_callback;
WINDOW_MOUSEPOS_CB mousepos_callback;
WINDOW_SCROLL_CB scroll_callback;

static int _translate_key(int glwf_key) {
    switch (glwf_key) {
    case GLFW_KEY_ESCAPE:           return WINDOW_KEY_ESCAPE;
    case GLFW_KEY_BACKSPACE:        return WINDOW_KEY_BACKSPACE;
    case GLFW_KEY_DELETE:           return WINDOW_KEY_DELETE;
    case GLFW_KEY_TAB:              return WINDOW_KEY_TAB;
    case GLFW_KEY_ENTER:            return WINDOW_KEY_ENTER;
    case GLFW_KEY_LEFT:             return WINDOW_KEY_LEFT;
    case GLFW_KEY_RIGHT:            return WINDOW_KEY_RIGHT;
    case GLFW_KEY_UP:               return WINDOW_KEY_UP;
    case GLFW_KEY_DOWN:             return WINDOW_KEY_DOWN;
    case GLFW_KEY_HOME:             return WINDOW_KEY_HOME;
    case GLFW_KEY_END:              return WINDOW_KEY_END;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:      return WINDOW_KEY_SHIFT;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:    return WINDOW_KEY_CONTROL;
    case GLFW_KEY_A:                return WINDOW_KEY_A;
    case GLFW_KEY_B:                return WINDOW_KEY_B;
    case GLFW_KEY_C:                return WINDOW_KEY_C;
    case GLFW_KEY_D:                return WINDOW_KEY_D;
    case GLFW_KEY_E:                return WINDOW_KEY_E;
    case GLFW_KEY_F:                return WINDOW_KEY_F;
    case GLFW_KEY_G:                return WINDOW_KEY_G;
    case GLFW_KEY_H:                return WINDOW_KEY_H;
    case GLFW_KEY_I:                return WINDOW_KEY_I;
    case GLFW_KEY_J:                return WINDOW_KEY_J;
    case GLFW_KEY_K:                return WINDOW_KEY_K;
    case GLFW_KEY_L:                return WINDOW_KEY_L;
    case GLFW_KEY_M:                return WINDOW_KEY_M;
    case GLFW_KEY_N:                return WINDOW_KEY_N;
    case GLFW_KEY_O:                return WINDOW_KEY_O;
    case GLFW_KEY_P:                return WINDOW_KEY_P;
    case GLFW_KEY_Q:                return WINDOW_KEY_Q;
    case GLFW_KEY_R:                return WINDOW_KEY_R;
    case GLFW_KEY_S:                return WINDOW_KEY_S;
    case GLFW_KEY_T:                return WINDOW_KEY_T;
    case GLFW_KEY_U:                return WINDOW_KEY_U;
    case GLFW_KEY_V:                return WINDOW_KEY_V;
    case GLFW_KEY_W:                return WINDOW_KEY_W;
    case GLFW_KEY_X:                return WINDOW_KEY_X;
    case GLFW_KEY_Y:                return WINDOW_KEY_Y;
    case GLFW_KEY_Z:                return WINDOW_KEY_Z;
    default:                        return WINDOW_KEY_NONE;
    }
}

static int _translate_mods(int glfw_mods) {
    int mods = 0;
    if (glfw_mods & GLFW_MOD_SHIFT)
        mods |= WINDOW_MOD_SHIFT;
    if (glfw_mods & GLFW_MOD_CONTROL)
        mods |= WINDOW_MOD_CTRL;
    return mods;
}

static int _translate_action(int glfw_action) {
    switch (glfw_action) {
    case GLFW_PRESS:
        return WINDOW_PRESS;
    case GLFW_RELEASE:
        return WINDOW_RELEASE;
    case GLFW_REPEAT:
        return WINDOW_REPEAT;
    default:
        return -1;
    }
}

static int _translate_mousebutton(int glfw_mousebutton) {
    switch (glfw_mousebutton) {
    case GLFW_MOUSE_BUTTON_LEFT:
        return WINDOW_LEFT;
    case GLFW_MOUSE_BUTTON_RIGHT:
        return WINDOW_RIGHT;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        return WINDOW_MIDDLE;
    default:
        return -1;
    }
}

static void _mousebutton_callback(GLFWwindow *, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            window.mouse.left = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            window.mouse.right = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            window.mouse.middle = true;
    }
    else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            window.mouse.left = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            window.mouse.right = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            window.mouse.middle = false;
    }

    window.mouse.travel = 0;

    if (mousebutton_callback)
        mousebutton_callback(_translate_mousebutton(button), _translate_action(action), _translate_mods(mods));
}

static void _mousepos_callback(GLFWwindow *, double x, double y) {
    y = window.h - y;
    if (skip_first_mousepos)
        skip_first_mousepos = false;
    else if (mousepos_callback)
        mousepos_callback(x, y);
    window.mouse.pos.x = x;
    window.mouse.pos.y = y;
    ++window.mouse.travel;
}

static void _scroll_callback(GLFWwindow *glfw_window, double x, double y) {
    if (scroll_callback)
        scroll_callback(x, y);
}

static void _key_callback(GLFWwindow *, int key, int code, int action, int mods) {
    if (key == GLFW_KEY_Q && mods & GLFW_MOD_CONTROL)
        quit = true;
    if (key_callback)
        key_callback(_translate_key(key), _translate_mods(mods), _translate_action(action));
}

static void _char_callback(GLFWwindow *, unsigned int code) {
    if (char_callback)
        char_callback(code);
}

static void _size_callback(GLFWwindow *glfw_window, int w, int h) {
    w = w;
    h = h;
}

static void _refresh_callback(GLFWwindow *glfw_window) {}

static void _close_callback(GLFWwindow *glfw_window) {
    quit = true;
}

void window_create(const char *title, int w, int h, bool fullscreen) {
    assert(glfw_window == 0);

    key_callback = 0;
    char_callback = 0;
    mousebutton_callback = 0;
    mousepos_callback = 0;
    scroll_callback = 0;
    quit = false;
    cursor_hidden = false;
    window.mouse.left = false;
    window.mouse.right = false;
    window.mouse.middle = false;

    if (!glfwInit()) {
        printf("Could not init GLFW\n");
        return;
    }

    GLFWmonitor *monitor = 0;

    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        window.w = mode->width;
        window.h = mode->height;
    }
    else {
        window.w = w;
        window.h = h;
    }

    window.mouse.travel = 0;

    glfw_window = glfwCreateWindow(window.w, window.h, title, monitor, 0);
    if (!glfw_window) {
        printf("Could not create GLFW glfw_window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(glfw_window);
    glfwSetKeyCallback(glfw_window, _key_callback);
    glfwSetCharCallback(glfw_window, _char_callback);
    glfwSetWindowSizeCallback(glfw_window, _size_callback);
    glfwSetWindowRefreshCallback(glfw_window, _refresh_callback);
    glfwSetMouseButtonCallback(glfw_window, _mousebutton_callback);
    glfwSetCursorPosCallback(glfw_window, _mousepos_callback);
    glfwSetScrollCallback(glfw_window, _scroll_callback);
    glfwSetWindowCloseCallback(glfw_window, _close_callback);
    glfwSetTime(0.0);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1, 1);
}

void window_run(WINDOW_MAIN_LOOP_FUNC main_loop_func) {
    while (!quit)
        main_loop_func();
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}

void window_set_cursor_hidden(bool hidden) {
    if ((cursor_hidden = hidden))
        glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void window_set_key_callback(WINDOW_KEY_CB callback) {
    key_callback = callback;
}

void window_set_char_callback(WINDOW_CHAR_CB callback) {
    char_callback = callback;
}

void window_set_mousebutton_callback(WINDOW_MOUSEBUTTON_CB callback) {
    mousebutton_callback = callback;
}

void window_set_mousepos_callback(WINDOW_MOUSEPOS_CB callback) {
    mousepos_callback = callback;
}

void window_set_scroll_callback(WINDOW_SCROLL_CB callback) {
    scroll_callback = callback;
}

void window_poll_events() {
    glfwPollEvents();
}

void window_swap_buffers() {
    glfwSwapBuffers(glfw_window);
}

void window_set_time() {
    glfwSetTime(0.0);
}

long long int window_get_time() {
    return (long long int)(glfwGetTime() * 1000000.0);
}

void window_make_context_current() {
    glfwMakeContextCurrent(glfw_window);
}
