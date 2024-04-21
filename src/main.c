#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "config.h"
#include "cube.h"
#include "util.h"

typedef struct {
  GLFWwindow *window;
  int width;
  int height;
  int scale;
} Model;

static Model model;
static Model *g = &model;

typedef struct {
    GLuint program;
    GLuint position;
    GLuint normal;
    GLuint uv;
    GLuint matrix;
    GLuint sampler;
    GLuint camera;
    GLuint timer;
    GLuint extra1;
    GLuint extra2;
    GLuint extra3;
    GLuint extra4;
} Attrib;

void onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
    printf("ESC PRESSED...\n");
  }
}

int get_scale_factor() {
  int window_width, window_height;
  int buffer_width, buffer_height;
  glfwGetWindowSize(g->window, &window_width, &window_height);
  glfwGetFramebufferSize(g->window, &buffer_width, &buffer_height);
  int result = buffer_width / window_width;
  result = MAX(1, result);
  result = MIN(2, result);
  return result;
}

GLuint gen_cube_buffer(float x, float y, float z, float n, int w) {
  GLfloat *data = malloc_faces(10, 6);
  float ao[6][4] = {0};
  float light[6][4] = {
      {0.5, 0.5, 0.5, 0.5},
      {0.5, 0.5, 0.5, 0.5},
      {0.5, 0.5, 0.5, 0.5},
      {0.5, 0.5, 0.5, 0.5},
      {0.5, 0.5, 0.5, 0.5},
      {0.5, 0.5, 0.5, 0.5}
  };
  make_cube(data, ao, light, 1, 1, 1, 1, 1, 1, x, y, z, n, w);
  return gen_faces(10, 6, data);
}

void render_item(Attrib *attrib) {
  float matrix[16];
  set_matrix_item(matrix, g->width, g->height, g->scale);
  glUseProgram(attrib->program);
  glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
  glUniform3f(attrib->camera, 0, 0, 5);
  glUniform1i(attrib->sampler, 0);
  glUniform1f(attrib->timer, 0.1);
  int w = 1; // TODO: item index
  GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
  //draw_cube(attrib, buffer);
  del_buffer(buffer);
}

int main(void){
  printf("Cubes game started...\n");

  if (!glfwInit()) {
    printf("Failed to initialize GLFW.\n");
    return -1;
  }

// #ifdef __APPLE__
//   glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
//   glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
//   glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//   glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
// #endif

  g->window = glfwCreateWindow(1280, 720, "Cubes", NULL, NULL);
  if (!g->window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(g->window);

  if (glewInit() != GLEW_OK) {
    printf("Failed to initialize GLEW.\n");
    return -1;
  }

  // SHADERS
  Attrib block_attrib = {0};
  GLuint program;

  program = load_program("shaders/block_vertex.glsl", "shaders/block_fragment.glsl");
  block_attrib.program = program;
  block_attrib.position = glGetAttribLocation(program, "position");
  block_attrib.normal = glGetAttribLocation(program, "normal");
  block_attrib.uv = glGetAttribLocation(program, "uv");
  block_attrib.matrix = glGetUniformLocation(program, "matrix");
  block_attrib.sampler = glGetUniformLocation(program, "sampler");
  block_attrib.extra1 = glGetUniformLocation(program, "sky_sampler");
  block_attrib.extra2 = glGetUniformLocation(program, "daylight");
  block_attrib.extra3 = glGetUniformLocation(program, "fog_distance");
  block_attrib.extra4 = glGetUniformLocation(program, "ortho");
  block_attrib.camera = glGetUniformLocation(program, "camera");
  block_attrib.timer = glGetUniformLocation(program, "timer");

  glfwSetKeyCallback(g->window, onKey);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  while (!glfwWindowShouldClose(g->window)){
    g->scale = get_scale_factor();
    glfwGetFramebufferSize(g->window, &g->width, &g->height);

    glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    //render_chunks(&block_attrib);

    glfwSwapBuffers(g->window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
