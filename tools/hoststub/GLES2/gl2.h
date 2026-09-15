#ifndef STUB_GLES2_H
#define STUB_GLES2_H
typedef unsigned int GLenum; typedef unsigned int GLuint; typedef int GLint;
typedef float GLfloat; typedef float GLclampf; typedef long GLsizeiptr; typedef char GLchar;
typedef unsigned int GLboolean; typedef int GLsizei;
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_STREAM_DRAW 0x88E0
#define GL_TRIANGLES 0x0004
#define GL_FLOAT 0x1406
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_COLOR_BUFFER_BIT 0x4000
GLuint glCreateShader(GLenum t);
void glShaderSource(GLuint s, GLsizei n, const GLchar *const *src, const GLint *len);
void glCompileShader(GLuint s);
void glGetShaderiv(GLuint s, GLenum p, GLint *v);
void glGetShaderInfoLog(GLuint s, GLsizei cap, GLsizei *len, GLchar *log);
GLuint glCreateProgram(void);
void glAttachShader(GLuint p, GLuint s);
void glBindAttribLocation(GLuint p, GLuint i, const GLchar *n);
void glLinkProgram(GLuint p);
void glGetProgramiv(GLuint p, GLenum pn, GLint *v);
void glDeleteShader(GLuint s);
void glDeleteProgram(GLuint p);
void glGenBuffers(GLsizei n, GLuint *b);
void glDeleteBuffers(GLsizei n, const GLuint *b);
void glBindBuffer(GLenum t, GLuint b);
void glEnableVertexAttribArray(GLuint i);
void glVertexAttribPointer(GLuint i, GLint sz, GLenum ty, GLboolean n, GLsizei st, const void *p);
void glDisable(GLenum c);
void glEnable(GLenum c);
void glBlendFunc(GLenum s, GLenum d);
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glClear(GLuint m);
void glBufferData(GLenum t, GLsizeiptr s, const void *d, GLenum u);
void glDrawArrays(GLenum m, GLint f, GLsizei c);
void glUseProgram(GLuint p);
#endif
