#ifndef platform_h
#define platform_h

#include "vec.h"

#define WIN


typedef void (*MAIN_LOOP_FUNC)();
typedef void (*KEY_CALLBACK)(int key, int mods, int action);
typedef void (*CHAR_CALLBACK)(unsigned int code);
typedef void (*MOUSEBUTTON_CALLBACK)(int button, int action, int mods);
typedef void (*MOUSEPOS_CALLBACK)(float x, float y);
typedef void (*SCROLL_CALLBACK)(float x, float y);


enum PlatformAction {
    PLATFORM_PRESS,
    PLATFORM_RELEASE,
    PLATFORM_REPEAT,
};


enum PlatformMouseButton {
    PLATFORM_LEFT,
    PLATFORM_RIGHT,
    PLATFORM_MIDDLE,
};


enum PlatformKey {
    PLATFORM_KEY_NONE,
    PLATFORM_KEY_ESCAPE,
    PLATFORM_KEY_BACKSPACE,
    PLATFORM_KEY_DELETE,
    PLATFORM_KEY_TAB,
    PLATFORM_KEY_ENTER,
    PLATFORM_KEY_LEFT,
    PLATFORM_KEY_RIGHT,
    PLATFORM_KEY_UP,
    PLATFORM_KEY_DOWN,
    PLATFORM_KEY_HOME,
    PLATFORM_KEY_END,
    PLATFORM_KEY_SHIFT,
    PLATFORM_KEY_CONTROL,
    PLATFORM_KEY_A,
    PLATFORM_KEY_B,
    PLATFORM_KEY_C,
    PLATFORM_KEY_D,
    PLATFORM_KEY_E,
    PLATFORM_KEY_F,
    PLATFORM_KEY_G,
    PLATFORM_KEY_H,
    PLATFORM_KEY_I,
    PLATFORM_KEY_J,
    PLATFORM_KEY_K,
    PLATFORM_KEY_L,
    PLATFORM_KEY_M,
    PLATFORM_KEY_N,
    PLATFORM_KEY_O,
    PLATFORM_KEY_P,
    PLATFORM_KEY_Q,
    PLATFORM_KEY_R,
    PLATFORM_KEY_S,
    PLATFORM_KEY_T,
    PLATFORM_KEY_U,
    PLATFORM_KEY_V,
    PLATFORM_KEY_W,
    PLATFORM_KEY_X,
    PLATFORM_KEY_Y,
    PLATFORM_KEY_Z,
};


enum PlatformMod {
    PLATFORM_MOD_SHIFT = 0x1,
    PLATFORM_MOD_CTRL = 0x2,
};


struct Mouse {
    vec2 pos;
    bool left, right, middle; // buttons pressed
    int travel;
};


void plat_create_window(const char *title, int _w, int _h, bool fullscreen=false);
void plat_run_window(MAIN_LOOP_FUNC main_loop_func);
vec2 plat_get_screen();
Mouse &plat_get_mouse();

void plat_set_key_callback(KEY_CALLBACK callback);
void plat_set_char_callback(CHAR_CALLBACK callback);
void plat_set_mousebutton_callback(MOUSEBUTTON_CALLBACK callback);
void plat_set_mousepos_callback(MOUSEPOS_CALLBACK callback);
void plat_set_scroll_callback(SCROLL_CALLBACK callback);

void plat_poll_events();
void plat_swap_buffers();
void plat_make_context_current();
void plat_set_cursor_hidden(bool hidden);

void plat_set_time();
long long int plat_get_time();
void plat_sleep(int milliseconds);

void *plat_fopen(const char *path, const char *mode);

#endif
