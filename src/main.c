#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "config.h"

typedef struct {
  GLFWwindow *window;
} Model;

static Model model;
static Model *g = &model;

void onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
    printf("ESC PRESSED...\n");
  }
}

int main(void){
  printf("Cubes game started...");

  if (!glfwInit()) {
    printf("Failed to initialize GLFW.\n");
    return -1;
  }

#ifdef __APPLE__
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

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

  glfwSetKeyCallback(g->window, onKey);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  while (!glfwWindowShouldClose(g->window)){
    glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(g->window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
