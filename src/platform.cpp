
#define GLEW_STATIC
#include <GL/glew.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>
#include "platform.h"
#include <iostream>

static PlatformServiceContext platform;
static Arena scratch_arena;

#include "game.cpp"

void GLFWErrorCallback(int error, const char *desc);
void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void GLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void GLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void GLFWCursorEnterCallback(GLFWwindow *window, int entered);
void GLFWScrollCallback(GLFWwindow *window, double x, double y);
void GLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height);

bool InitRenderer();
void PollEvents();
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, 
                            GLsizei length, const char *message, const void *userParam);

int main() {
    MathTest();
    
    scratch_arena = CreateArena(Kilobytes(16));

    if (!glfwInit()) return -1;
    
    glfwSetErrorCallback(GLFWErrorCallback);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    //glfwWindowHint(GLFW_SAMPLES, 4); // anti-aliasing
    platform.window = glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr);
    
    if (!platform.window) {
        glfwTerminate();
        return -1;
    }
    
    glfwGetFramebufferSize(platform.window, &platform.framebuffer_width, &platform.framebuffer_height);
    
    glfwSetKeyCallback(platform.window, GLFWKeyCallback);
    glfwSetMouseButtonCallback(platform.window, GLFWMouseButtonCallback);
    glfwSetCursorPosCallback(platform.window, GLFWCursorPosCallback);
    glfwSetCursorEnterCallback(platform.window, GLFWCursorEnterCallback);
    glfwSetScrollCallback(platform.window, GLFWScrollCallback);
    glfwSetFramebufferSizeCallback(platform.window, GLFWFramebufferSizeCallback);
    
    glfwGetCursorPos(platform.window, &platform.cursor_x, &platform.cursor_y);
    glfwMakeContextCurrent(platform.window);

    if (!InitRenderer()) return -1;
    
    GameState *state = (GameState *)malloc(sizeof(GameState));
    memset(state, 0, sizeof(GameState));
    
    double last_frame_time = glfwGetTime();
    
    while (!glfwWindowShouldClose(platform.window)) {
        double current_frame_time = glfwGetTime();
        platform.delta_time = current_frame_time - last_frame_time;
        last_frame_time = current_frame_time;
        
        glClear(GL_COLOR_BUFFER_BIT);
        UpdateAndRender(state);
        glfwSwapBuffers(platform.window);
        glfwPollEvents();
    }
}

bool RegisterInputEvent(InputEvent *event) {
    bool result = false;
    
    if (platform.input_event_queue.count < CountOf(InputEventQueue::events)) {
        platform.input_event_queue.events[platform.input_event_queue.count++] = *event;
        result = true;
    } else {
        Assert(0); // storage is full
    }
    
    return result;
}

bool GetNextInputEvent(InputEvent *event) {
    bool result = false;
    
    if (platform.input_event_queue.count) {
        InputEvent *next_event = &platform.input_event_queue.events[platform.input_event_queue.count - 1];
        *event = *next_event;
        platform.input_event_queue.count--;
        
        *next_event = {};
        
        result = true;
    }
    
    return result;
}

void GLFWErrorCallback(int error, const char *desc) {
    fprintf(stderr, "Error: %s\n", desc);
}

void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    InputEvent event = {};
    event.key = key;
    event.scancode = scancode;
    event.action = action;
    event.mods = mods;
    event.device = InputDevice::Keyboard;
    event.type = InputEventType::KeyEvent;
    
    RegisterInputEvent(&event);
}

void GLFWMouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    InputEvent event = {};
    event.button = button;
    event.action = action;
    event.mods = mods;
    event.device = InputDevice::Mouse;
    event.type = InputEventType::MouseButtonEvent;
    
    RegisterInputEvent(&event);
}

void GLFWCursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    InputEvent event = {};
    event.x = xpos;
    event.y = ypos;
    
    event.dx = xpos - platform.cursor_x;
    event.dy = ypos - platform.cursor_y;
    
    event.device = InputDevice::Mouse;
    event.type = InputEventType::CursorPositionEvent;
    
    RegisterInputEvent(&event);
    
    platform.cursor_x = xpos;
    platform.cursor_y = ypos;
}

void GLFWCursorEnterCallback(GLFWwindow *window, int entered) {
    InputEvent event = {};
    event.entered = entered;
    event.device = InputDevice::Mouse;
    event.type = InputEventType::CursorEnterEvent;
    
    RegisterInputEvent(&event);
}

void GLFWScrollCallback(GLFWwindow *window, double x, double y) {
    InputEvent event = {};
    event.x = x;
    event.y = y;
    event.device = InputDevice::Mouse;
    event.type = InputEventType::MouseScrollEvent;
    
    RegisterInputEvent(&event);
}

void GLFWFramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    platform.framebuffer_width = width;
    platform.framebuffer_height = height;
    
    glViewport(0, 0, width, height);    
}

void APIENTRY glDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;
    
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

void *ReadEntireFile(const char *filename, size_t *count) {
    void *result = 0;
    
    FILE *f = fopen(filename, "r");
    if (!f) return result;
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    result = malloc(size);
    memset(result, 0, size);
    fread(result, size, 1, f);
    fclose(f);
    
    *count = size;
    
    return result;
}

bool InitRenderer() {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return false;
    }
    
    fprintf(stdout, "OpenGL %s\n", glGetString(GL_VERSION));
    
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        fprintf(stdout, "OpenGL Debug Context\n");
    }
    
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 1);

    // TODO: default font
    InitFontRenderer();
    
    InitializeUtilBuffers();
    
    return true;
}

void *ScratchAlloc(u64 count) {
    if (scratch_arena.count + count > scratch_arena.size) {
        u64 new_size = scratch_arena.size * 2;
        scratch_arena.base_address = realloc(scratch_arena.base_address, new_size);
        fprintf(stderr, "INFO: Scratch arena resized: (%lld -> %lld)\n", scratch_arena.size, new_size);
        scratch_arena.size = new_size;
    }
    
    void *result = (u8 *)scratch_arena.base_address + scratch_arena.count;
    scratch_arena.count += count;
    
    memset(result, 0, count);
    
    return result;
}

Arena CreateArena(u64 size) {
    Arena result = {};
    result.base_address = malloc(size);
    result.size = size;
    
    return result;
}

void GetWindowFramebufferSize(int *width, int *height) {
    glfwGetFramebufferSize(platform.window, width, height);
}

bool IsButtonPressed(int button) {
    return glfwGetMouseButton(platform.window, button) == GLFW_PRESS;
}
