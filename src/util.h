#ifndef _util_h_
#define _util_h_

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

typedef struct {
  unsigned int fps;
  unsigned int frames;
  double since;
} FPS;

void update_fps(FPS *fps);

GLuint gen_buffer(GLsizei size, GLfloat *data);
void del_buffer(GLuint buffer);

GLfloat *malloc_faces(int components, int faces);
GLuint gen_faces(int components, int faces, GLfloat *data);

GLuint make_shader(GLenum type, const char *source);
GLuint load_shader(GLenum type, const char *path);
GLuint make_program(GLuint shader1, GLuint shader2);
GLuint load_program(const char *path1, const char *path2);

void load_png_texture(const char *file_name);

#endif
