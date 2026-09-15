#ifndef STUB_EGL_H
#define STUB_EGL_H
typedef void *EGLDisplay; typedef void *EGLSurface; typedef void *EGLContext; typedef void *EGLConfig;
typedef int EGLint; typedef unsigned int EGLBoolean;
#define EGL_DEFAULT_DISPLAY ((EGLDisplay)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_SURFACE ((EGLSurface)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NONE 0x3038
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_SURFACE_TYPE 0x3033
#define EGL_WINDOW_BIT 0x0004
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_ALPHA_SIZE 0x3021
#define EGL_DEPTH_SIZE 0x3025
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3058
EGLDisplay eglGetDisplay(void *native);
EGLBoolean eglInitialize(EGLDisplay d, EGLint *maj, EGLint *min);
EGLBoolean eglChooseConfig(EGLDisplay d, const EGLint *attr, EGLConfig *cfg, EGLint n, EGLint *out);
EGLSurface eglCreateWindowSurface(EGLDisplay d, EGLConfig c, void *win, const EGLint *attr);
EGLContext eglCreateContext(EGLDisplay d, EGLConfig c, EGLContext share, const EGLint *attr);
EGLBoolean eglMakeCurrent(EGLDisplay d, EGLSurface dr, EGLSurface rd, EGLContext c);
EGLBoolean eglQuerySurface(EGLDisplay d, EGLSurface s, EGLint a, EGLint *v);
EGLBoolean eglDestroySurface(EGLDisplay d, EGLSurface s);
EGLBoolean eglDestroyContext(EGLDisplay d, EGLContext c);
EGLBoolean eglTerminate(EGLDisplay d);
EGLBoolean eglSwapBuffers(EGLDisplay d, EGLSurface s);
EGLint eglGetError(void);
#endif
