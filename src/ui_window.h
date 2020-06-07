#ifndef window_h
#define window_h

#include "math_vec.h"


typedef void (*WINDOW_MAIN_LOOP_FUNC)();
typedef void (*WINDOW_KEY_CB)(int key, int mods, int action);
typedef void (*WINDOW_CHAR_CB)(unsigned int code);
typedef void (*WINDOW_MOUSEBUTTON_CB)(int button, int action, int mods);
typedef void (*WINDOW_MOUSEPOS_CB)(float x, float y);
typedef void (*WINDOW_SCROLL_CB)(float x, float y);

enum WindowAction {
    WINDOW_PRESS,
    WINDOW_RELEASE,
    WINDOW_REPEAT,
};

enum WindowMouseButton {
    WINDOW_LEFT,
    WINDOW_RIGHT,
    WINDOW_MIDDLE,
};

enum WindowMod {
    WINDOW_MOD_SHIFT = 0x1,
    WINDOW_MOD_CTRL = 0x2,
};

enum WindowKey {
    WINDOW_KEY_NONE,
    WINDOW_KEY_ESCAPE,
    WINDOW_KEY_BACKSPACE,
    WINDOW_KEY_DELETE,
    WINDOW_KEY_TAB,
    WINDOW_KEY_ENTER,
    WINDOW_KEY_LEFT,
    WINDOW_KEY_RIGHT,
    WINDOW_KEY_UP,
    WINDOW_KEY_DOWN,
    WINDOW_KEY_HOME,
    WINDOW_KEY_END,
    WINDOW_KEY_SHIFT,
    WINDOW_KEY_CONTROL,
    WINDOW_KEY_A,
    WINDOW_KEY_B,
    WINDOW_KEY_C,
    WINDOW_KEY_D,
    WINDOW_KEY_E,
    WINDOW_KEY_F,
    WINDOW_KEY_G,
    WINDOW_KEY_H,
    WINDOW_KEY_I,
    WINDOW_KEY_J,
    WINDOW_KEY_K,
    WINDOW_KEY_L,
    WINDOW_KEY_M,
    WINDOW_KEY_N,
    WINDOW_KEY_O,
    WINDOW_KEY_P,
    WINDOW_KEY_Q,
    WINDOW_KEY_R,
    WINDOW_KEY_S,
    WINDOW_KEY_T,
    WINDOW_KEY_U,
    WINDOW_KEY_V,
    WINDOW_KEY_W,
    WINDOW_KEY_X,
    WINDOW_KEY_Y,
    WINDOW_KEY_Z,
};

struct WindowMouse {
    vec2 pos;
    bool left, right, middle; /* buttons pressed */
    int travel;
};

struct Window {
    float w, h;
    WindowMouse mouse;
};

extern Window window;

void window_create(const char *title, int w, int h, bool fullscreen=false);
void window_run(WINDOW_MAIN_LOOP_FUNC main_loop_func);

void window_set_key_callback(WINDOW_KEY_CB callback);
void window_set_char_callback(WINDOW_CHAR_CB callback);
void window_set_mousebutton_callback(WINDOW_MOUSEBUTTON_CB callback);
void window_set_mousepos_callback(WINDOW_MOUSEPOS_CB callback);
void window_set_scroll_callback(WINDOW_SCROLL_CB callback);

void window_poll_events();
void window_swap_buffers();
void window_make_context_current();
void window_set_cursor_hidden(bool hidden);

void window_set_time();
long long int window_get_time();

#endif
