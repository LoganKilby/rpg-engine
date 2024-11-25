// This is to get the debugger to stop at the line where CompileShader failed instead of inside the function
#ifdef DEBUG
#define CompileShader(shader) Assert(_CompileShader(shader))
#define LinkProgram(program) Assert(_LinkProgram(program))
#else
#define CompileShader(shader) _CompileShader(shader)
#define LinkProgram(program) _LinkProgram(program);
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <glm-1.0.1/glm/glm.hpp>
#include <glm-1.0.1/glm/gtc/matrix_transform.hpp>
using namespace glm;
typedef vec2 v2;
typedef vec3 v3;
typedef vec4 v4;

struct Rect {
    f32 x;
    f32 y;
    f32 width;
    f32 height;
};

struct Texture {
    GLuint id;
    int width;
    int height;
};

#define Squared(a) (a*a)

#if 0
struct v4 {
    f32 x, y, z, w;
};

struct v3 {
    f32 x, y, z;
};

struct v2 {
    f32 x, y;
};

struct mat2 {
    v2 c0;
    v2 c1;
};

inline v2 operator+(v2 &a, v2 &b) {
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline v2 operator+(v2 &a, f32 b) {
    v2 result;
    result.x = a.x + b;
    result.y = a.y + b;
    return result;
}

inline v2 operator-(v2 &a, v2 &b) {
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline v2 operator-(v2 &a, f32 b) {
    v2 result;
    result.x = a.x - b;
    result.y = a.y - b;
    return result;
}

inline v2 operator*(v2 &a, f32 b) {
    v2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

inline v2 operator+=(v2 &a, v2 &b) {
    a.x = a.x+b.x;
    a.y = a.y+b.y;
    return a;
}

inline v2 operator+=(v2 &a, f32 b) {
    a = a + b;
    return a;
}

inline v2 operator-=(v2 &a, v2 &b) {
    a.x = a.x-b.x;
    a.y = a.y-b.y;
    return a;
}

inline v2 operator-=(v2 &a, f32 b) {
    a = a - b;
    return a;
}

inline v2 operator*=(v2 &a, f32 b) {
    a.x *= b;
    a.y *= b;
    return a;
}

f32 Length(v2 a) {
    f32 result = sqrt(a.x*a.x + a.y*a.y);
    return result;
}

v2 Normalize(v2 a) {
    f32 l = Length(a);

    v2 result;
    result.x = a.x / l;
    result.y = a.y / l;

    return result;
}

v2 VMul(v2 a, f32 b) {
    v2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

v2 VAdd(v2 a, v2 b) {
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

v2 VAdd(v2 a, f32 b) {
    v2 result;
    result.x = a.x + b;
    result.y = a.y + b;
    return result;
}

v2 MMul(mat2 m, v2 v) {
    v2 result;
    result.x = m.c0.x * v.x + m.c1.x * v.y;
    result.y = m.c0.y * v.x + m.c1.y * v.y;

    return result;
}

mat2 MMul(mat2 m, f32 n) {
    mat2 result;
    result.c0 = m.c0 * n;
    result.c1 = m.c1 * n;
    return result;
}

f32 Determinant(mat2 m) {
    f32 result = m.c0.x * m.c1.y - m.c1.x * m.c0.y;
    return result;
}

mat2 Inverse(mat2 m) {
    f32 d = Determinant(m);
    mat2 m1 = { {m.c1.y, -m.c0.y}, {-m.c1.x, m.c0.x} };
    mat2 result = MMul(m1, 1 / d);
    return result;
}
#endif

void ScreenToNDC(f32 *x, f32 *y, int screen_w, int screen_h) {
    f32 tx = *x;
    f32 ty = *y;

    ty = -(2 * (ty / screen_h) - 1.0);
    tx = 2 * (tx / screen_w) - 1.0;

    *x = tx;
    *y = ty;
}

v2 ScreenToNDC(v2 p, int screen_w, int screen_h) {
    v2 result = p;
    ScreenToNDC(&result.x, &result.y, screen_w, screen_h);

    return result;
}

v3 ScreenToNDC(v3 p, int screen_w, int screen_h) {
    v3 result = p;
    ScreenToNDC(&result.x, &result.y, screen_w, screen_h);

    return result;
}


bool _CompileShader(GLuint shader) {
    glCompileShader(shader);

    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (!status) {
        char log[512] = {};
        glGetShaderInfoLog(shader, 512, 0, log);
        fprintf(stderr, "Error: Shader compilation failed\n%s", log);
        fflush(stderr);
    }

    return status;
}

bool _LinkProgram(GLuint program) {
    glLinkProgram(program);

    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if(!status) {
        char log[512] = {};
        glGetProgramInfoLog(program, 512, 0, log);
        fprintf(stderr, "Error: Program link failed\n%s", log);
        fflush(stderr);
    }

    return status;
}

static b32 glutil_initialized = false;
static GLuint glutil_basic_vbo;
static GLuint glutil_basic_vao;
static GLuint glutil_basic_program;
static GLuint glutil_sampler_2d;

struct GLUtilVertex {
    v3 p;
    v2 uv;
};

void InitializeUtilBuffers() {
    glGenBuffers(1, &glutil_basic_vbo);
    glGenVertexArrays(1, &glutil_basic_vao);
    glBindVertexArray(glutil_basic_vao);
    glBindBuffer(GL_ARRAY_BUFFER, glutil_basic_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLUtilVertex) * 6, 0, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLUtilVertex), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLUtilVertex), (void *)offsetof(GLUtilVertex, uv));
    glEnableVertexAttribArray(1);

    u32 texture_data = U32_MAX;
    glGenTextures(1, &glutil_sampler_2d);
    glBindTexture(GL_TEXTURE_2D, glutil_sampler_2d);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data);

    const char *vsource = R"(
        #version 460

        in vec3 vp;
        in vec2 uv;

        out vec2 vuv;

        void main() {
            gl_Position = vec4(vp, 1.0);
            vuv = uv;
        }
    )";
    const char *fsource = R"(
        #version 460

        in vec2 vuv;

        out vec4 fs_color;

        uniform vec4 color;
        uniform sampler2D diffuse;

        void main() {
            fs_color = texture(diffuse, vuv) * color;
        }
    )";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &vsource, 0);
    CompileShader(vs);
    glShaderSource(fs, 1, &fsource, 0);
    CompileShader(fs);

    glutil_basic_program = glCreateProgram();
    glAttachShader(glutil_basic_program, vs);
    glAttachShader(glutil_basic_program, fs);
    LinkProgram(glutil_basic_program);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void CreateRectangleVertices(v3 *vertices, int left, int right, int top, int bottom) {
    f32 l = (f32)left;
    f32 r = (f32)right;
    f32 b = (f32)bottom;
    f32 t = (f32)top;

    vertices[0] = { l, b, 0 };
    vertices[1] = { r, t, 0 };
    vertices[2] = { l, t, 0 };
    vertices[3] = { l, b, 0 };
    vertices[4] = { r, b, 0 };
    vertices[5] = { r, t, 0 };
}

void DrawLine(int x0, int y0, int x1, int y1, v4 color) {
    if (!glutil_initialized) InitializeUtilBuffers();

    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    v3 l0 = ScreenToNDC({(f32)x0, (f32)y0, 0}, fb_width, fb_height);
    v3 l1 = ScreenToNDC({(f32)x1, (f32)y1, 0}, fb_width, fb_height);

    GLUtilVertex verts[] = {
        { l0, {} },
        { l1, {} },
    };

    glNamedBufferSubData(glutil_basic_vbo, 0, sizeof(verts), (f32*)verts);
    glUseProgram(glutil_basic_program);

    int color_location = glGetUniformLocation(glutil_basic_program, "color");
    glUniform4fv(color_location, 1, (f32*)&color);
    glBindVertexArray(glutil_basic_vao);
    glBindTexture(GL_TEXTURE_2D, glutil_sampler_2d);
    glDrawArrays(GL_LINES, 0, 2);
}

void DrawRectangleLines(int left, int right, int top, int bottom, v4 color) {
    DrawLine(left, top, right, top, color);
    DrawLine(left, top, left, bottom, color);
    DrawLine(right, top, right, bottom, color);
    DrawLine(left, bottom, right, bottom, color);
}

void DrawRectangle(int left, int right, int top, int bottom, v4 color) {
    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    f32 l = (f32)left;
    f32 r = (f32)right;
    f32 b = (f32)bottom;
    f32 t = (f32)top;

    v3 bl = ScreenToNDC({l, b, 0}, fb_width, fb_height);
    v3 tr = ScreenToNDC({r, t, 0}, fb_width, fb_height);
    v3 tl = ScreenToNDC({l, t, 0}, fb_width, fb_height);
    v3 br = ScreenToNDC({r, b, 0}, fb_width, fb_height);

    v2 bl_uv = { 0, 0 };
    v2 tr_uv = { 1, 1 };
    v2 tl_uv = { 0, 1 };
    v2 br_uv = { 1, 0 };

    GLUtilVertex verts[] = {
        { bl, bl_uv },
        { tr, tr_uv },
        { tl, tl_uv },

        { bl, bl_uv },
        { br, br_uv },
        { tr, tr_uv },
    };

    glNamedBufferSubData(glutil_basic_vbo, 0, sizeof(verts), verts);

    glUseProgram(glutil_basic_program);
    int color_location = glGetUniformLocation(glutil_basic_program, "color");
    glUniform4fv(color_location, 1, (f32*)&color);
    glBindVertexArray(glutil_basic_vao);
    glBindTexture(GL_TEXTURE_2D, glutil_sampler_2d);
    glDrawArrays(GL_TRIANGLES, 0, CountOf(verts));
}

void LoadTexture(const char *filename, Texture *texture) {
    stbi_set_flip_vertically_on_load(true);

    Texture result = {};

    // TODO(lmk): arena alloc
    int width, height, channels;
    u8 *data = stbi_load(filename, &width, &height, &channels, 0);

    if (data) {
        glGenTextures(1, &result.id);
        glBindTexture(GL_TEXTURE_2D, result.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, channels == 4 ? GL_RGBA : GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        result.width = width;
        result.height = height;
    } else {
        fprintf(stderr, "Failed to load texture: %s\n", stbi_failure_reason());
    }

    *texture = result;
}

void DrawTexture(Texture texture, int x, int y) {
    int left = x;
    int right = x + texture.width;
    int top = y;
    int bottom = y + texture.height;

    Assert(left < right);
    Assert(top < bottom);

    GLuint temp = glutil_sampler_2d;
    glutil_sampler_2d = texture.id;
    DrawRectangle(left, right, top, bottom, {1,1,1,1});
    glutil_sampler_2d = temp;
}

void DrawPixel(int x, int y, v4 color, f32 size) {
    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    v3 pixel = ScreenToNDC({(f32)x, (f32)y, 0}, fb_width, fb_height);

    GLUtilVertex vert = {
        pixel,
        {0, 0},
    };

    glNamedBufferSubData(glutil_basic_vbo, 0, sizeof(vert), &vert);

    glPointSize(size);

    glUseProgram(glutil_basic_program);
    int color_location = glGetUniformLocation(glutil_basic_program, "color");
    glUniform4fv(color_location, 1, (f32*)&color);
    glBindVertexArray(glutil_basic_vao);
    glBindTexture(GL_TEXTURE_2D, glutil_sampler_2d);
    glDrawArrays(GL_POINTS, 0, 1);
}

void DrawTextureRect(Texture texture, Rect dest, Rect texture_rect) {
    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    f32 l = (f32)dest.x;
    f32 r = (f32)dest.x + dest.width;
    f32 b = (f32)dest.y + dest.height;
    f32 t = (f32)dest.y;

    v3 bl = ScreenToNDC({l, b, 0}, fb_width, fb_height);
    v3 tr = ScreenToNDC({r, t, 0}, fb_width, fb_height);
    v3 tl = ScreenToNDC({l, t, 0}, fb_width, fb_height);
    v3 br = ScreenToNDC({r, b, 0}, fb_width, fb_height);

    f32 uvl = (f32)texture_rect.x;
    f32 uvr = (f32)texture_rect.x + texture_rect.width;
    f32 uvb = (f32)texture_rect.y + texture_rect.height;
    f32 uvt = (f32)texture_rect.y;

    v2 bl_uv = { uvl / texture.width, 1 - (uvb / texture.height) };
    v2 tr_uv = { uvr / texture.width, 1 - (uvt / texture.height) };
    v2 tl_uv = { uvl / texture.width, 1 - (uvt / texture.height) };
    v2 br_uv = { uvr / texture.width, 1 - (uvb / texture.height) };

    GLUtilVertex verts[] = {
        { bl, bl_uv },
        { tr, tr_uv },
        { tl, tl_uv },

        { bl, bl_uv },
        { br, br_uv },
        { tr, tr_uv },
    };

    glNamedBufferSubData(glutil_basic_vbo, 0, sizeof(verts), verts);

    v4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    glUseProgram(glutil_basic_program);
    int color_location = glGetUniformLocation(glutil_basic_program, "color");
    glUniform4fv(color_location, 1, (f32*)&color);
    glBindVertexArray(glutil_basic_vao);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glDrawArrays(GL_TRIANGLES, 0, CountOf(verts));
}

void DrawTextureRect(Texture texture, int px, int py, int tx, int ty, int width, int height) {
    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    f32 l = (f32)px;
    f32 r = (f32)px + width;
    f32 b = (f32)py + height;
    f32 t = (f32)py;

    v3 bl = ScreenToNDC({l, b, 0}, fb_width, fb_height);
    v3 tr = ScreenToNDC({r, t, 0}, fb_width, fb_height);
    v3 tl = ScreenToNDC({l, t, 0}, fb_width, fb_height);
    v3 br = ScreenToNDC({r, b, 0}, fb_width, fb_height);

    f32 uvl = (f32)tx;
    f32 uvr = (f32)tx + width;
    f32 uvb = (f32)ty + height;
    f32 uvt = (f32)ty;

    v2 bl_uv = { uvl / texture.width, 1 - (uvb / texture.height) };
    v2 tr_uv = { uvr / texture.width, 1 - (uvt / texture.height) };
    v2 tl_uv = { uvl / texture.width, 1 - (uvt / texture.height) };
    v2 br_uv = { uvr / texture.width, 1 - (uvb / texture.height) };

    GLUtilVertex verts[] = {
        { bl, bl_uv },
        { tr, tr_uv },
        { tl, tl_uv },

        { bl, bl_uv },
        { br, br_uv },
        { tr, tr_uv },
    };

    glNamedBufferSubData(glutil_basic_vbo, 0, sizeof(verts), verts);

    v4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    glUseProgram(glutil_basic_program);
    int color_location = glGetUniformLocation(glutil_basic_program, "color");
    glUniform4fv(color_location, 1, (f32*)&color);
    glBindVertexArray(glutil_basic_vao);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glDrawArrays(GL_TRIANGLES, 0, CountOf(verts));
}

static float cube_vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

struct Mesh {
    u32 vao;
    u32 vbo;
    int vertex_count;
};


