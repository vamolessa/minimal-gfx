// this is a minimal sample for opengl 4.5 using modern direct state access (DSA) functions
// it uses winapi and wgl to create windows and the modern opengl context
// the only dependencies are `glcorearb.h`, `wglext.h` and `KHR/khrplatform.h`
// which should be put in a dependencies folder (keep khrplatform inside the "KHR" folder!)
// they are documented bellow on where to get them
//
// features used:
// - vertex array object (VAO)
// - vertex buffer object (VBO)
// - index buffer object (IBO)
// - uniform buffer object (UBO)
// - textures
//
// this was made following using this guide to modern opengl functions as a reference:
// https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions
//
// build:
// cl opengl45.c
//

#pragma comment (lib, "user32")
#pragma comment (lib, "gdi32")
#pragma comment (lib, "opengl32")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <glcorearb.h> // https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h
#include <wglext.h> // https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
// NOTE: https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h and put in "KHR" folder

#include <stdio.h>

typedef enum { false, true } bool;

#define UNREACHABLE __debugbreak()
#define ASSERT(invariant) ((invariant) ? 1 : \
    ((void)printf("%s:%d: %s (last error: %ld)\n", __FILE__, __LINE__, #invariant, GetLastError()), (void)UNREACHABLE, 0))
#define LEN(array) (sizeof(array) / sizeof((array)[0]))
#define OFFSET_OF(type, member) ((size_t)&(((type*)0)->member))

#define DEBUG_LAYER

///////////////////////////////////////////////////////////////////////////////////////////////////
// used opengl procedures table
///////////////////////////////////////////////////////////////////////////////////////////////////
#define GL_PROCS \
X(PFNGLGETSTRINGPROC, glGetString)\
X(PFNGLENABLEPROC, glEnable)\
X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)\
X(PFNGLCLIPCONTROLPROC, glClipControl)\
X(PFNGLFRONTFACEPROC, glFrontFace)\
X(PFNGLVIEWPORTPROC, glViewport)\
X(PFNGLDEPTHFUNCPROC, glDepthFunc)\
X(PFNGLCLEARNAMEDFRAMEBUFFERFVPROC, glClearNamedFramebufferfv)\
\
X(PFNGLCREATEBUFFERSPROC, glCreateBuffers)\
X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)\
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)\
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase)\
X(PFNGLDRAWELEMENTSPROC, glDrawElements)\
\
X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv)\
X(PFNGLCREATESHADERPROC, glCreateShader)\
X(PFNGLSHADERSOURCEPROC, glShaderSource)\
X(PFNGLCOMPILESHADERPROC, glCompileShader)\
X(PFNGLGETSHADERIVPROC, glGetShaderiv)\
X(PFNGLCREATEPROGRAMPROC, glCreateProgram)\
X(PFNGLATTACHSHADERPROC, glAttachShader)\
X(PFNGLLINKPROGRAMPROC, glLinkProgram)\
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)\
X(PFNGLDELETESHADERPROC, glDeleteShader)\
X(PFNGLUSEPROGRAMPROC, glUseProgram)\
\
X(PFNGLNAMEDBUFFERSTORAGEPROC, glNamedBufferStorage)\
X(PFNGLNAMEDBUFFERSUBDATAPROC, glNamedBufferSubData)\
X(PFNGLVERTEXARRAYVERTEXBUFFERPROC, glVertexArrayVertexBuffer)\
X(PFNGLVERTEXARRAYELEMENTBUFFERPROC, glVertexArrayElementBuffer)\
X(PFNGLENABLEVERTEXARRAYATTRIBPROC, glEnableVertexArrayAttrib)\
X(PFNGLVERTEXARRAYATTRIBFORMATPROC, glVertexArrayAttribFormat)\
X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding)\
X(PFNGLVERTEXARRAYBINDINGDIVISORPROC, glVertexArrayBindingDivisor)\
\
X(PFNGLCREATETEXTURESPROC, glCreateTextures)\
X(PFNGLTEXTUREPARAMETERIPROC, glTextureParameteri)\
X(PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D)\
X(PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D)\
X(PFNGLBINDTEXTUREUNITPROC, glBindTextureUnit)\
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// used wgl procedures table
///////////////////////////////////////////////////////////////////////////////////////////////////
#define WGL_PROCS \
X(PFNWGLGETEXTENSIONSSTRINGARBPROC, wglGetExtensionsStringARB)\
X(PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB)\
X(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)\
X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT)\
///////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: declare all used opengl and wgl procedures
#define X(type, name) static type name;
GL_PROCS
WGL_PROCS
#undef X

#define LOAD_PROC(type, name) do { name = (type)(void*)wglGetProcAddress(#name); ASSERT(name); } while (0)

static const wchar_t* window_class_name = L"DefaultWindowClass";
static bool should_quit = false;

static LRESULT WINAPI
process_window_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) {
        should_quit = true;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static void APIENTRY
gl_debug_message_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user
) {
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)user;

    #define PREFIX "GL DEBUG MESSAGE CALLBACK: "

    printf(PREFIX);
    puts(message);

    OutputDebugStringA(PREFIX);
    OutputDebugStringA(message);
    OutputDebugStringA("\n");

    #undef PREFIX

    if (
        type == GL_DEBUG_SOURCE_API || type == GL_DEBUG_SOURCE_WINDOW_SYSTEM || type == GL_DEBUG_SOURCE_SHADER_COMPILER ||
        type == GL_DEBUG_SOURCE_THIRD_PARTY || type == GL_DEBUG_SOURCE_APPLICATION
    ) {
        UNREACHABLE;
    }
    if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
        UNREACHABLE;
    }
}

int
main(void) {
    SetProcessDPIAware();

    HINSTANCE hinstance = GetModuleHandleW(NULL);
    ASSERT(hinstance);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // get wgl functions
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    {
        HWND window_handle = CreateWindowExW(
            0, L"STATIC", L"DummyWindow", WS_OVERLAPPED,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, NULL, NULL
        );
        ASSERT(window_handle);

        HDC dc = GetDC(window_handle);
        ASSERT(dc);

        PIXELFORMATDESCRIPTOR pixel_format_desc = {
            .nSize = sizeof(pixel_format_desc),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 24,
        };

        int pixel_format = ChoosePixelFormat(dc, &pixel_format_desc);
        ASSERT(pixel_format);
        ASSERT(DescribePixelFormat(dc, pixel_format, sizeof(pixel_format_desc), &pixel_format_desc));

        // NOTE: reason to create dummy window is that SetPixelFormat can be called only once for a window
        ASSERT(SetPixelFormat(dc, pixel_format, &pixel_format_desc));

        HGLRC gl_rc = wglCreateContext(dc);
        ASSERT(gl_rc);

        ASSERT(wglMakeCurrent(dc, gl_rc));
        ASSERT(wglGetCurrentContext());

        #define X(type, name) LOAD_PROC(type, name);
        WGL_PROCS
        #undef X

        const char* extensions = wglGetExtensionsStringARB(dc);
        printf("\n== legacy extensions ==\n%s\n", extensions);

        LOAD_PROC(PFNGLGETSTRINGPROC, glGetString);

        printf("\n== legacy context ==\n");
        printf("GL_VENDOR = %s\n", glGetString(GL_VENDOR));
        printf("GL_RENDERER = %s\n", glGetString(GL_RENDERER));
        printf("GL_VERSION = %s\n", glGetString(GL_VERSION));
        printf("GL_SHADING_LANGUAGE_VERSION = %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

        ASSERT(wglMakeCurrent(NULL, NULL));
        wglDeleteContext(gl_rc);
        ReleaseDC(window_handle, dc);
        DestroyWindow(window_handle);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // create window
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    UINT window_class_style = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    WNDCLASSEXW window_class = {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = window_class_style,
        .lpfnWndProc = &process_window_message,
        .hInstance = hinstance,
        .hCursor = LoadCursorA(NULL, IDC_ARROW),
        .lpszClassName = window_class_name,
    };
    ASSERT(RegisterClassExW(&window_class));

    HWND window_handle = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
        window_class_name,
        L"minimal opengl 4.5",
        WS_OVERLAPPEDWINDOW,
        /* X */ CW_USEDEFAULT,
        /* Y */ CW_USEDEFAULT,
        /* nWidth */ CW_USEDEFAULT,
        /* nHeight */ CW_USEDEFAULT,
        /* hWndParent */ NULL,
        /* hMenu */ NULL,
        hinstance,
        NULL
    );
    ASSERT(window_handle);
    ShowWindow(window_handle, SW_SHOW);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // create modern opengl context
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    HDC dc = GetDC(window_handle);
    ASSERT(dc);

    {
        // set pixel format for OpenGL context

        const int attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 24,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,

            // NOTE: uncomment for sRGB framebuffer, from WGL_ARB_framebuffer_sRGB extension
            // NOTE: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
            //WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,

            // NOTE: uncomment for multisampeld framebuffer, from WGL_ARB_multisample extension
            // NOTE: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
            //WGL_SAMPLE_BUFFERS_ARB, 1,
            //WGL_SAMPLES_ARB, 4, // 4x MSAA

            0,
        };

        int pixel_format = 0;
        UINT pixel_format_count = 0;
        ASSERT(wglChoosePixelFormatARB(
            dc,
            attribs,
            /* pfAttribFList */ NULL,
            /* nMaxFormats */ 1,
            &pixel_format,
            &pixel_format_count
        ));
        ASSERT(pixel_format_count);

        PIXELFORMATDESCRIPTOR pixel_format_desc = { .nSize = sizeof(pixel_format_desc) };
        ASSERT(DescribePixelFormat(dc, pixel_format, sizeof(pixel_format_desc), &pixel_format_desc));
        ASSERT(SetPixelFormat(dc, pixel_format, &pixel_format_desc));
    }

    HGLRC gl_rc = NULL;
    {
        // create modern OpenGL 4.5 context

        int context_flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#if defined(DEBUG_LAYER)
        // ask for debug context
        // this is so we can enable debug callback
        context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, context_flags,
            0,
        };

        HGLRC shared_gl_rc = NULL;
        gl_rc = wglCreateContextAttribsARB(dc, shared_gl_rc, attribs);
        ASSERT(gl_rc);
    }
    ASSERT(wglMakeCurrent(dc, gl_rc));

    const char* extensions = wglGetExtensionsStringARB(dc);
    printf("\n== modern extensions ==\n%s\n", extensions);

    printf("\n== modern context ==\n");
    printf("GL_VENDOR = %s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER = %s\n", glGetString(GL_RENDERER));
    printf("GL_VERSION = %s\n", glGetString(GL_VERSION));
    printf("GL_SHADING_LANGUAGE_VERSION = %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // enable vsync
    wglSwapIntervalEXT(1);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // get required opengl functions
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    #define X(type, name) LOAD_PROC(type, name);
    GL_PROCS
    #undef X

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // setup debug layer
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    (void)gl_debug_message_callback;
#if defined(DEBUG_LAYER)
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&gl_debug_message_callback, /* userParam */ NULL);
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // create vertex, index, vertex array and uniform buffers
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    struct VertexData {
        float pos[3];
        float col[4];
        float uv[2];
    };
    const struct VertexData vertices[] = {
        { .pos = { 0.0f, -0.5f, 0.0f}, .col = {1.0f, 0.0f, 0.0f, 1.0f}, .uv = {1.0f, 1.0f} },
        { .pos = {-0.5f,  0.5f, 0.0f}, .col = {0.0f, 0.0f, 1.0f, 1.0f}, .uv = {0.0f, 0.0f} },
        { .pos = { 0.5f,  0.5f, 0.0f}, .col = {0.0f, 1.0f, 0.0f, 1.0f}, .uv = {1.0f, 0.0f} },
    };
    ASSERT(sizeof(unsigned short) == 2);
    const unsigned short indices[] = {0, 1, 2};

    // create vertex buffer (VBO)
    GLuint vertex_buffer = 0;
    glCreateBuffers(1, &vertex_buffer);
    glNamedBufferStorage(vertex_buffer, sizeof(vertices), vertices, GL_DYNAMIC_STORAGE_BIT);

    // create index buffer (IBO/EBO (E standing for element))
    GLuint index_buffer = 0;
    glCreateBuffers(1, &index_buffer);
    glNamedBufferStorage(index_buffer, sizeof(indices), indices, GL_DYNAMIC_STORAGE_BIT);

    // create vertex array object (VAO)
    GLuint vertex_array = 0;
    glCreateVertexArrays(1, &vertex_array);

    // bind the vertex buffer to binding 0 (there can be more than one vertex buffer bound)
    glVertexArrayVertexBuffer(
        vertex_array,
        /* bindingindex */ 0,
        vertex_buffer,
        /* offset */ 0,
        /* stride */ sizeof(vertices[0])
    );
    // for when using instance drawing (since we're not using it, divisor is 0
    glVertexArrayBindingDivisor(vertex_array, /* bindingindex */ 0, /* divisor */ 0);

    // bind the index buffer (there can be only one index buffer)
    glVertexArrayElementBuffer(vertex_array, index_buffer);

    // enables and informs the format of the attribute (vertex shader input) at index 0 (pos)
    glEnableVertexArrayAttrib(vertex_array, /* attribindex */ 0);
    glVertexArrayAttribFormat(
        vertex_array,
        /* attribindex */ 0,
        /* size */ LEN(vertices[0].pos),
        GL_FLOAT,
        /* normalized */ GL_FALSE,
        /* relativeoffset */ OFFSET_OF(struct VertexData, pos)
    );
    // make the attribute at index 0 take its data from binding 0 (that is, the previously bound vertex buffer)
    glVertexArrayAttribBinding(vertex_array, /* attribindex */ 0, /* bindingindex */ 0);

    // enables and informs the format of the attribute (vertex shader input) at index 1 (col)
    glEnableVertexArrayAttrib(vertex_array, /* attribindex */ 1);
    glVertexArrayAttribFormat(
        vertex_array,
        /* attribindex */ 1,
        /* size */ LEN(vertices[0].col),
        GL_FLOAT,
        /* normalized */ GL_FALSE,
        /* relativeoffset */ OFFSET_OF(struct VertexData, col)
    );
    // make the attribute at index 0 take its data from binding 0 (that is, the previously bound vertex buffer)
    glVertexArrayAttribBinding(vertex_array, /* attribindex */ 1, /* bindingindex */ 0);

    // enables and informs the format of the attribute (vertex shader input) at index 2 (uv)
    glEnableVertexArrayAttrib(vertex_array, /* attribindex */ 2);
    glVertexArrayAttribFormat(
        vertex_array,
        /* attribindex */ 2,
        /* size */ LEN(vertices[0].uv),
        GL_FLOAT,
        /* normalized */ GL_FALSE,
        /* relativeoffset */ OFFSET_OF(struct VertexData, uv)
    );
    // make the attribute at index 0 take its data from binding 0 (that is, the previously bound vertex buffer)
    glVertexArrayAttribBinding(vertex_array, /* attribindex */ 2, /* bindingindex */ 0);

    struct UniformData {
        float transform[4][4];
    };

    // create uniform buffer (UBO)
    GLuint uniform_buffer = 0;
    glCreateBuffers(1, &uniform_buffer);
    glNamedBufferStorage(uniform_buffer, sizeof(struct UniformData), /* data */ NULL, GL_DYNAMIC_STORAGE_BIT);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // create main texture
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // NOTE: checkers pattern
    unsigned char main_texture_rgba[] = {
        255, 255, 255, 255, 127, 127, 127, 255,
        127, 127, 127, 255, 255, 255, 255, 255,
    };
    GLsizei main_texture_rgba_width = 2;
    GLsizei main_texture_rgba_height = 2;
    ASSERT(main_texture_rgba_width * main_texture_rgba_height == LEN(main_texture_rgba) / 4);

    GLuint main_texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &main_texture);
    glTextureParameteri(main_texture, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(main_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(main_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(main_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(main_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(main_texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTextureStorage2D(main_texture, /* levels */ 1, GL_RGBA8, main_texture_rgba_width, main_texture_rgba_height);
    glTextureSubImage2D(
        main_texture,
        /* level */ 0,
        /* xoffset */ 0,
        /* yoffset */ 0,
        main_texture_rgba_width,
        main_texture_rgba_height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        main_texture_rgba
    );

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // shaders
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    #define SHADER_SRC(...) #__VA_ARGS__
    const char* vertex_shader_src =
        "#version 450\n"
        SHADER_SRC(
            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec4 col;
            layout(location = 2) in vec2 texcoord;
            layout(binding = 0) uniform uniforms0 {
                mat4 transform;
            };
            out vec4 color;
            out vec2 uv;
            void main() {
                gl_Position = transform * vec4(pos, 1.0);
                color = col;
                uv = texcoord;
            }
        );

    const char* frag_shader_src =
        "#version 450\n"
        SHADER_SRC(
        in vec4 color;
        in vec2 uv;
        out vec4 frag_color;
        layout(binding = 0) uniform sampler2D main_texture;
        void main() {
            // NOTE: the `uv * 3.0` will make the checkers texture tile three times
            vec4 tex_color = texture(main_texture, uv * 3.0);
            frag_color = color * tex_color;
        }
        );
    #undef SHADER_SRC

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
    glCompileShader(vertex_shader);
    GLint vertex_shader_success = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_success);
    if (!vertex_shader_success) {
        char shader_log_buf[1024];
        glGetShaderInfoLog(vertex_shader, sizeof(shader_log_buf), /* length */ NULL, shader_log_buf);
        OutputDebugStringA("vertex shader compile error:\n");
        OutputDebugStringA(shader_log_buf);
        OutputDebugStringA("\n");
        UNREACHABLE;
    }

    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_shader_src, NULL);
    glCompileShader(frag_shader);
    GLint frag_shader_success = 0;
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &frag_shader_success);
    if (!frag_shader_success) {
        char shader_log_buf[1024];
        glGetShaderInfoLog(frag_shader, sizeof(shader_log_buf), /* length */ NULL, shader_log_buf);
        OutputDebugStringA("frag shader compile error:\n");
        OutputDebugStringA(shader_log_buf);
        OutputDebugStringA("\n");
        UNREACHABLE;
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // set opengl default state
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // NOTE: make opengl work like direct3d
    glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // draw
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    for (;;) {
        MSG message;
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (should_quit) {
            break;
        }

        RECT window_client_size = {0};
        ASSERT(GetClientRect(window_handle, &window_client_size));
        GLsizei window_width = (GLsizei)window_client_size.right;
        GLsizei window_height = (GLsizei)window_client_size.bottom;

        {
            // NOTE: update uniform buffers

            float aspect_ratio = (float)window_width / (float)window_height;
            float h = 1.7320509f;
            struct UniformData uniform_data = {
                .transform = {
                    // NOTE: a precalculated view projection matrix as an example
                    {h / aspect_ratio, 0.0f,        0.0f,  0.0f},
                    {            0.0f,    h,        0.0f,  0.0f},
                    {            0.0f, 0.0f,  -1.001001f, -1.0f},
                    {            0.0f, 0.0f, 2.99299312f,  4.0f},
                },
            };
            glNamedBufferSubData(uniform_buffer, /* offset */ 0, sizeof(uniform_data), &uniform_data);
        }

        glViewport(/* x */ 0, /* y */ 0, window_width, window_height);
        glClearNamedFramebufferfv(/* framebuffer */ 0, GL_COLOR, /* drawbuffer */ 0, (float[]){0.8f, 0.6f, 0.4f, 1.0f});
        glClearNamedFramebufferfv(/* framebuffer */ 0, GL_DEPTH, /* drawbuffer */ 0, (float[]){1.0f});

        {
            // NOTE: render loop

            glUseProgram(shader_program);
            glBindTextureUnit(0, main_texture);
            glBindVertexArray(vertex_array);
            glBindBufferBase(GL_UNIFORM_BUFFER, /* bindingindex */ 0, uniform_buffer);
            glDrawElements(GL_TRIANGLES, LEN(indices), GL_UNSIGNED_SHORT, /* offset in bytes */ 0);
        }

        // cleanup opengl state (not really required)
        glUseProgram(0);
        glBindTextureUnit(0, 0);
        glBindVertexArray(0);

        ASSERT(SwapBuffers(dc));
    }

    return 0;
}
