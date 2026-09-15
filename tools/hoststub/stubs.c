/* заглушки Android/EGL/GLES для прогона оболочки на хосте */
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
int __android_log_print(int p, const char *t, const char *f, ...) { (void)p;(void)t;(void)f; return 0; }
typedef void *EGLDisplay; typedef void *EGLSurface; typedef void *EGLContext; typedef void *EGLConfig;
typedef int EGLint; typedef unsigned int EGLBoolean;
static int dummy;
EGLDisplay eglGetDisplay(void *n){(void)n;return &dummy;}
EGLBoolean eglInitialize(EGLDisplay d,EGLint*a,EGLint*b){(void)d;if(a)*a=1;if(b)*b=4;return 1;}
EGLBoolean eglChooseConfig(EGLDisplay d,const EGLint*a,EGLConfig*c,EGLint n,EGLint*o){(void)d;(void)a;if(c&&n>0)*c=&dummy;if(o)*o=1;return 1;}
EGLSurface eglCreateWindowSurface(EGLDisplay d,EGLConfig c,void*w,const EGLint*a){(void)d;(void)c;(void)w;(void)a;return &dummy;}
EGLContext eglCreateContext(EGLDisplay d,EGLConfig c,EGLContext s,const EGLint*a){(void)d;(void)c;(void)s;(void)a;return &dummy;}
EGLBoolean eglMakeCurrent(EGLDisplay d,EGLSurface a,EGLSurface b,EGLContext c){(void)d;(void)a;(void)b;(void)c;return 1;}
EGLBoolean eglQuerySurface(EGLDisplay d,EGLSurface s,EGLint a,EGLint*v){(void)d;(void)s;(void)a;if(v)*v=1280;return 1;}
EGLBoolean eglDestroySurface(EGLDisplay d,EGLSurface s){(void)d;(void)s;return 1;}
EGLBoolean eglDestroyContext(EGLDisplay d,EGLContext c){(void)d;(void)c;return 1;}
EGLBoolean eglTerminate(EGLDisplay d){(void)d;return 1;}
EGLBoolean eglSwapBuffers(EGLDisplay d,EGLSurface s){(void)d;(void)s;return 1;}
EGLint eglGetError(void){return 0;}
typedef unsigned int GLenum; typedef unsigned int GLuint; typedef int GLint; typedef char GLchar;
typedef long GLsizeiptr; typedef unsigned int GLboolean; typedef int GLsizei;
static GLuint ids = 1;
GLuint glCreateShader(GLenum t){(void)t;return ids++;}
void glShaderSource(GLuint s,GLsizei n,const GLchar*const*c,const GLint*l){(void)s;(void)n;(void)c;(void)l;}
void glCompileShader(GLuint s){(void)s;}
void glGetShaderiv(GLuint s,GLenum p,GLint*v){(void)s;(void)p;if(v)*v=1;}
void glGetShaderInfoLog(GLuint s,GLsizei c,GLsizei*l,GLchar*o){(void)s;(void)c;if(l)*l=0;if(o)o[0]=0;}
GLuint glCreateProgram(void){return ids++;}
void glAttachShader(GLuint p,GLuint s){(void)p;(void)s;}
void glBindAttribLocation(GLuint p,GLuint i,const GLchar*n){(void)p;(void)i;(void)n;}
void glLinkProgram(GLuint p){(void)p;}
void glGetProgramiv(GLuint p,GLenum n,GLint*v){(void)p;(void)n;if(v)*v=1;}
void glDeleteShader(GLuint s){(void)s;}
void glDeleteProgram(GLuint p){(void)p;}
void glGenBuffers(GLsizei n,GLuint*b){int i;for(i=0;i<n;i++)b[i]=ids++;}
void glDeleteBuffers(GLsizei n,const GLuint*b){(void)n;(void)b;}
void glBindBuffer(GLenum t,GLuint b){(void)t;(void)b;}
void glEnableVertexAttribArray(GLuint i){(void)i;}
void glVertexAttribPointer(GLuint i,GLint s,GLenum t,GLboolean n,GLsizei st,const void*p){(void)i;(void)s;(void)t;(void)n;(void)st;(void)p;}
void glDisable(GLenum c){(void)c;}
void glEnable(GLenum c){(void)c;}
void glBlendFunc(GLenum s,GLenum d){(void)s;(void)d;}
void glViewport(GLint x,GLint y,GLsizei w,GLsizei h){(void)x;(void)y;(void)w;(void)h;}
void glClearColor(float r,float g,float b,float a){(void)r;(void)g;(void)b;(void)a;}
void glClear(GLuint m){(void)m;}
int g_last_verts = 0;
void glBufferData(GLenum t,GLsizeiptr s,const void*d,GLenum u){(void)t;(void)d;(void)u;g_last_verts=(int)(s/24);}
void glDrawArrays(GLenum m,GLint f,GLsizei c){(void)m;(void)f;(void)c;}
void glUseProgram(GLuint p){(void)p;}
typedef struct AInputEvent AInputEvent;
struct android_poll_source;
int32_t AInputEvent_getType(const AInputEvent*e){(void)e;return 0;}
int32_t AMotionEvent_getAction(const AInputEvent*e){(void)e;return 0;}
float AMotionEvent_getX(const AInputEvent*e,size_t i){(void)e;(void)i;return 0;}
float AMotionEvent_getY(const AInputEvent*e,size_t i){(void)e;(void)i;return 0;}
int32_t AKeyEvent_getKeyCode(const AInputEvent*e){(void)e;return 0;}
int32_t AKeyEvent_getAction(const AInputEvent*e){(void)e;return 0;}
int ALooper_pollAll(int t,int*f,int*e,void**d){(void)t;(void)f;(void)e;(void)d;return -1;}
