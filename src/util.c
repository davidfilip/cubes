#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "util.h"
#include "lodepng.h"

char *load_file(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
      fprintf(stderr, "fopen %s failed: %d %s\n", path, errno, strerror(errno));
      exit(1);
  }
  fseek(file, 0, SEEK_END);
  int length = ftell(file);
  rewind(file);
  char *data = calloc(length + 1, sizeof(char));
  fread(data, 1, length, file);
  fclose(file);
  return data;
}

GLuint gen_buffer(GLsizei size, GLfloat *data) {
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return buffer;
}

void del_buffer(GLuint buffer) {
  glDeleteBuffers(1, &buffer);
}

GLfloat *malloc_faces(int components, int faces) {
  return malloc(sizeof(GLfloat) * 6 * components * faces);
}

GLuint gen_faces(int components, int faces, GLfloat *data) {
  GLuint buffer = gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
  free(data);
  return buffer;
}

GLuint make_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    GLchar *info = calloc(length, sizeof(GLchar));
    glGetShaderInfoLog(shader, length, NULL, info);
    fprintf(stderr, "glCompileShader failed:\n%s\n", info);
    free(info);
  }
  return shader;
}

GLuint load_shader(GLenum type, const char *path) {
  char *data = load_file(path);
  GLuint result = make_shader(type, data);
  free(data);
  return result;
}

GLuint make_program(GLuint shader1, GLuint shader2) {
  GLuint program = glCreateProgram();
  glAttachShader(program, shader1);
  glAttachShader(program, shader2);
  glLinkProgram(program);
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    GLchar *info = calloc(length, sizeof(GLchar));
    glGetProgramInfoLog(program, length, NULL, info);
    fprintf(stderr, "glLinkProgram failed: %s\n", info);
    free(info);
  }
  glDetachShader(program, shader1);
  glDetachShader(program, shader2);
  glDeleteShader(shader1);
  glDeleteShader(shader2);
  return program;
}

GLuint load_program(const char *path1, const char *path2) {
  GLuint shader1 = load_shader(GL_VERTEX_SHADER, path1);
  GLuint shader2 = load_shader(GL_FRAGMENT_SHADER, path2);
  GLuint program = make_program(shader1, shader2);
  return program;
}

void flip_image_vertical(unsigned char *data, unsigned int width, unsigned int height) {
  unsigned int size = width * height * 4;
  unsigned int stride = sizeof(char) * width * 4;
  unsigned char *new_data = malloc(sizeof(unsigned char) * size);

  for (unsigned int i = 0; i < height; i++) {
    unsigned int j = height - i - 1;
    memcpy(new_data + j * stride, data + i * stride, stride);
  }

  memcpy(data, new_data, size);
  free(new_data);
}

void load_png_texture(const char *file_name) {
  unsigned int error;
  unsigned char *data;
  unsigned int width, height;

  error = lodepng_decode32_file(&data, &width, &height, file_name);
  if (error) {
    fprintf(stderr, "load_png_texture %s failed, error %u: %s\n", file_name, error, lodepng_error_text(error));
    exit(1);
  }

  flip_image_vertical(data, width, height);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  free(data);
}
