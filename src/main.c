#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#define CUBE_KEY_FORWARD 'W'
#define CUBE_KEY_BACKWARD 'S'
#define CUBE_KEY_LEFT 'A'
#define CUBE_KEY_RIGHT 'D'

void onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ENTER) {
    printf("ENTER PRESSED.\n");
  }
}

int main(void){
  GLFWwindow* window;

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

  window = glfwCreateWindow(1280, 720, "Cubes", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    printf("Failed to initialize GLEW.\n");
    return -1;
  }

  glfwSetKeyCallback(window, onKey);

  /* Loop until the user closes the window */
  glViewport(0, 0, 1280, 720);
  while (!glfwWindowShouldClose(window))
  {
    glClearColor(250.0f / 255.0f, 119.0f / 255.0f, 110.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
