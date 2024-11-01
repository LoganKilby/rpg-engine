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

struct v4 {
    f32 x, y, z, w;
};

struct v3 {
    f32 x, y, z;
};

struct v2 {
    f32 x, y;
};

struct Texture {
    GLuint id;
    int width;
    int height;
};

#define Squared(a) (a*a)

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

struct FontRenderer {
    GLuint vbo;
    GLuint vao;
    GLuint program;
    int max_vertices; // max num vertices
} static font_renderer;

struct Font {
    stbtt_bakedchar char_data[96]; // ascii 32..126
    int bitmap_width;
    int bitmap_height;
    u8 *bitmap;
    GLuint texture;
};

struct FontVertex {
    f32 x;
    f32 y;
    f32 u;
    f32 v;
};

void InitFontRenderer() {
    font_renderer.max_vertices = 512;
    glGenBuffers(1, &font_renderer.vbo);
    glGenVertexArrays(1, &font_renderer.vao);
    glBindVertexArray(font_renderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, font_renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(FontVertex) * font_renderer.max_vertices, 0, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    const char *font_vs_source = R"(
        #version 460
        layout (location = 0) in vec2 p;
        layout (location = 1) in vec2 uv;

        out vec2 v_uv;

        uniform float screen_w;
        uniform float screen_h;

        void main() {
            float y = -2 * (p.y / screen_h) - 0.5;
            float x = 2 * (p.x / screen_w) - 0.5;
            gl_Position = vec4(x, y, 0.0, 1.0);
        }
    )";

    const char *font_fs_source = R"(
        #version 460

        in vec2 v_uv;
        out vec4 frag_color;

        uniform sampler2D font_atlas;

        void main() {
            frag_color = texture(font_atlas, v_uv);
        }
    )";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &font_vs_source, 0);
    CompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &font_fs_source, 0);
    CompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    LinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    font_renderer.program = program;
}

void LoadFont(const char *filename, int bitmap_width, int bitmap_height, Font *font) {
    size_t ttf_buffer_size;
    u8 *ttf_buffer = (u8 *)ReadEntireFile(filename, &ttf_buffer_size);

    int bitmap_size = bitmap_width * bitmap_height;
    u8 *bitmap = (u8 *)malloc(bitmap_size);
    memset(bitmap, 0, bitmap_size);

    int offset = 0;
    f32 pixel_height = 32.0;
    int first_char = 32;
    int num_chars = 96;
    stbtt_BakeFontBitmap(ttf_buffer, offset, pixel_height, bitmap, bitmap_width, bitmap_height, first_char, num_chars, font->char_data);

    glGenTextures(1, &font->texture);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    font->bitmap_width = bitmap_width;
    font->bitmap_height = bitmap_height;
    font->bitmap = bitmap;
}

void DrawText(Font *font, float x, float y, char *text, int text_length) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->texture);

    // TODO: Use temp allocator
    int vertex_count = 4 * text_length;
    int vert_buffer_size = sizeof(f32) * 8 * text_length;
    f32 *verts = (f32 *)malloc(vert_buffer_size);
    memset(verts, 0, vert_buffer_size);


    stbtt_aligned_quad q;
    for (int text_index = 0; text_index < text_length; ++text_index) {
        char c = text[text_index];
        if (c >= 32 && c <= 128) {
            stbtt_GetBakedQuad(font->char_data, font->bitmap_width, font->bitmap_height, c - 32, &x, &y, &q, 1);

            FontVertex *fv = (FontVertex *)verts + text_index * 4;

            fv[0] = { q.x0, q.y0, q.s0, q.t0 };
            fv[1] = { q.x1, q.y0, q.s1, q.t0 };
            fv[2] = { q.x1, q.y1, q.s1, q.t1 };
            fv[3] = { q.x0, q.y1, q.s0, q.t1 };
        }
    }

    if (text_length > font_renderer.max_vertices) {
        int new_vertex_max = font_renderer.max_vertices * 2;
        glNamedBufferData(font_renderer.vbo, sizeof(FontVertex) * new_vertex_max, 0, GL_DYNAMIC_DRAW);
        fprintf(stdout, "Info: Font renderer vertex buffer resized (%d -> %d)\n", font_renderer.max_vertices, new_vertex_max);
        font_renderer.max_vertices = new_vertex_max;
    }

    glNamedBufferSubData(font_renderer.vbo, 0, vert_buffer_size, verts);
}

void FreeFont(Font *font) {
    if (font->bitmap) free(font->bitmap);
    if (font->texture) glDeleteTextures(1, &font->texture);
    *font = {};
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

Texture LoadTexture(const char *filename) {
    stbi_set_flip_vertically_on_load(true);

    Texture result = {};

    int width, height, channels;
    u8 *data = stbi_load(filename, &width, &height, &channels, 0);

    if (data) {
        Assert(channels == 4);

        glGenTextures(1, &result.id);
        glBindTexture(GL_TEXTURE_2D, result.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);

        result.width = width;
        result.height = height;
    } else {
        fprintf(stderr, "Failed to load texture: %s\n", stbi_failure_reason());
    }

    return result;
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

void MathTest() {
    v2 a;

    a = { 1, 0 };
    a = Normalize(a);
    Assert(a.x == 1);
    Assert(a.y == 0);

    a = { 1.0, 1.0 };
    a = VMul(a, 2.0f);
    Assert(a.x == 2);
    Assert(a.y == 2);

    a = { 3.0, 0 };
    f32 al = Length(a);
    Assert(al == 3.0);

    a = VAdd({ 0, 0 }, 3);
    Assert(a.x == 3);
    Assert(a.y == 3);

    a = VAdd({0, 0}, {3, -3});
    Assert(a.x == 3);
    Assert(a.y == -3);

    a = { 0, 0 };
    v2 b = { 1, 1 };
    v2 c = a + b;
    Assert(c.x == 1);
    Assert(c.y == 1);

    a = {};
    a = a + 1;
    Assert(a.x == 1);
    Assert(a.y == 1);

    a = {};
    b = {1,1};
    c = a - b;
    Assert(c.x == -1);
    Assert(c.y == -1);

    a = {};
    c = a - 1;
    Assert(c.x == -1);
    Assert(c.y == -1);

    a = {1,1};
    a = a * 0.5f;
    Assert(a.x == 0.5f);
    Assert(a.y == 0.5f);
}


