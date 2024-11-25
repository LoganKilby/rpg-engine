#include "stdint.h"
#include "stdio.h"

typedef int b32;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;

#define KeyWasDown(lparam) (lparam & 0x40000000)
#define CountOf(arr) (sizeof(arr) / sizeof(arr[0]))
#define ArrayCount CountOf
#define Kilobytes(bytes) (bytes * 1024)
#define Megabytes(bytes) (Kilobytes(bytes) * 1024)
#define Min(a, b) a < b ? a : b
#define Max(a, b) a > b ? a : b
#define U32_MAX -(u32)1;

#define Assert(expression) if (!(expression)) { *(int *)0 = 0; }

enum InputDevice {
    Mouse,
    Keyboard,
};

enum InputEventType {
    KeyEvent,
    CursorEnterEvent,
    CursorPositionEvent,
    MouseButtonEvent,
    MouseScrollEvent,
};

struct InputEvent {
    InputDevice device;
    InputEventType type;
    int key;
    int scancode;
    int action;
    int mods;
    int button;
    double x;
    double y;
    double dx;
    double dy;

    // flags?
    int entered;
};

struct InputEventQueue {
    int count;
    InputEvent events[10];
};

struct PlatformServiceContext {
    GLFWwindow *window;
    double delta_time;
    double cursor_x;
    double cursor_y;
    InputEventQueue input_event_queue;
    int framebuffer_width;
    int framebuffer_height;

    void *memory;
    u64 memory_size;
};

void RegisterInputEvent(InputEvent *event);
bool GetNextInputEvent(InputEvent *event);
void *ReadEntireFile(const char *filename, size_t *count);
void GetWindowFramebufferSize(int *width, int *height);
bool IsButtonPressed(int button);

struct Arena {
    void *base_address;
    u64 size;
    u64 count;
};

void *ScratchAlloc(u64 count);
Arena CreateArena(u64 size);
