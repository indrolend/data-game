#pragma once

// Minimal fixed-function OpenGL 1.x immediate-mode recorder used by the iOS
// port. iOS has no desktop OpenGL, so DesktopRenderer.cpp (the real game
// renderer) is compiled against these entry points instead. Calls are
// transformed/lit on the CPU and collected into a triangle stream that a thin
// Metal layer draws. This header is platform neutral and has no Apple deps.

#include <cstdint>
#include <vector>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef float GLfloat;
typedef double GLdouble;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_POINTS 0x0000
#define GL_LINE_LOOP 0x0002
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_CULL_FACE 0x0B44
#define GL_LIGHTING 0x0B50
#define GL_COLOR_MATERIAL 0x0B57
#define GL_FOG 0x0B60
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_NORMALIZE 0x0BA1
#define GL_BLEND 0x0BE2
#define GL_TEXTURE_2D 0x0DE1
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_COMPILE 0x1300
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_LIGHT2 0x4002
#define GL_LIGHT3 0x4003
#define GL_LIGHT4 0x4004
#define GL_LIGHT5 0x4005
#define GL_LIGHT6 0x4006
#define GL_LIGHT7 0x4007
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_POSITION 0x1203
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_MODE 0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_EXP2 0x0801
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ALWAYS 0x0207
#define GL_EQUAL 0x0202
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_CLAMP 0x2900
#define GL_REPEAT 0x2901
#define GL_UNSIGNED_BYTE 0x1401
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_UNPACK_ALIGNMENT 0x0CF5

void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClear(GLbitfield mask);
void glClearStencil(GLint s);
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glFlush();
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glDepthMask(GLboolean flag);
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glCullFace(GLenum mode);
void glPolygonMode(GLenum face, GLenum mode);
void glLineWidth(GLfloat width);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void glStencilMask(GLuint mask);

void glMatrixMode(GLenum mode);
void glLoadIdentity();
void glPushMatrix();
void glPopMatrix();
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glMultMatrixf(const GLfloat* m);
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);

void glBegin(GLenum mode);
void glEnd();
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glNormal3f(GLfloat x, GLfloat y, GLfloat z);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glTexCoord2f(GLfloat s, GLfloat t);

void glLightfv(GLenum light, GLenum pname, const GLfloat* params);
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightModelfv(GLenum pname, const GLfloat* params);
void glFogfv(GLenum pname, const GLfloat* params);
void glFogf(GLenum pname, GLfloat param);
void glFogi(GLenum pname, GLint param);

GLuint glGenLists(GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList();
void glCallList(GLuint list);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glPixelStorei(GLenum pname, GLint param);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const void* pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                     GLsizei height, GLenum format, GLenum type, const void* pixels);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y,
                      GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x,
                         GLint y, GLsizei width, GLsizei height);

namespace glshim {

struct Vertex {
    float x, y, z, w;   // clip space (GL convention, z in [-w, w])
    float r, g, b, a;   // lit + fogged colour
    float u, v;
};

struct Batch {
    std::uint32_t first = 0;
    std::uint32_t count = 0;
    std::uint32_t texture = 0;   // 0 = untextured
    bool blend = false;
    bool depthTest = false;
    bool depthWrite = true;
    int cull = 0;                // 0 none, 1 back faces culled
};

struct Texture {
    int width = 0, height = 0;
    bool clamp = false;
    bool linear = true;
    bool valid = false;          // has pixel data
    std::uint32_t version = 0;   // bumped whenever pixels change
    std::vector<std::uint8_t> rgba;
};

struct Frame {
    float clear[4] = {0, 0, 0, 1};
    int viewportWidth = 0, viewportHeight = 0;
    std::vector<Vertex> vertices;
    std::vector<Batch> batches;
};

// Resets per-frame recorded geometry and GL state to defaults. Textures
// (and compiled display lists) persist across frames like real GL objects.
void beginFrame(int width, int height);
const Frame& frame();
const Texture* texture(std::uint32_t id);

} // namespace glshim
