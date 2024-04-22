#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "cube.h"
#include "matrix.h"
#include "util.h"

#define MAX_PLAYERS 8

typedef struct {
  float x;
  float y;
  float z;
  float rx;
  float ry;
  float t;
} State;

typedef struct {
  State state;
  State state1;
  State state2;
  GLuint buffer;
} Player;

typedef struct {
  GLFWwindow *window;
  int width;
  int height;
  int scale;
  int ortho;
  float fov;
  int flying;
  int render_radius;

  bool game_running;

  Player players[MAX_PLAYERS];
  int player_count;
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
    g->game_running = false;
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

GLuint gen_player_buffer(float x, float y, float z, float rx, float ry) {
  GLfloat *data = malloc_faces(10, 6);
  make_player(data, x, y, z, rx, ry);
  return gen_faces(10, 6, data);
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

void draw_triangles_3d_ao(Attrib *attrib, GLuint buffer, int count) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
        sizeof(GLfloat) * 10, 0);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
        sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
        sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void render_item(Attrib *attrib) {
  float matrix[16];
  set_matrix_item(matrix, g->width, g->height, g->scale);
  glUseProgram(attrib->program);
  glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
  glUniform3f(attrib->camera, 0, 0, 5);
  glUniform1i(attrib->sampler, 0);
  glUniform1f(attrib->timer, 0.1); // TODO: time of day
  int w = 1; // grass block
  GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
  draw_triangles_3d_ao(attrib, buffer, 36);
  del_buffer(buffer);
}

void render_player(Attrib *attrib, Player *player) {
  State *s = &player->state;
  float matrix[16];

  set_matrix_3d(
      matrix, g->width, g->height,
      s->x, s->y, s->z, s->rx, s->ry, g->fov, g->ortho, g->render_radius);
  glUseProgram(attrib->program);
  glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
  glUniform3f(attrib->camera, s->x, s->y, s->z);
  glUniform1i(attrib->sampler, 0);
  glUniform1f(attrib->timer, 0.1); //TODO: time of day
}

void reset_model(){
  memset(g->players, 0, sizeof(Player) * MAX_PLAYERS);
  g->player_count = 0;
  g->flying = 0;
}

void get_motion_vector(int flying, int sz, int sx, float rx, float ry, float *vx, float *vy, float *vz) {
  *vx = 0; *vy = 0; *vz = 0;
  if (!sz && !sx) {
    return;
  }

  float strafe = atan2f(sz, sx);
  if (flying) {
    float m = cosf(ry);
    float y = sinf(ry);
    if (sx) {
      if (!sz) {
        y = 0;
      }
      m = 1;
    }
    if (sz > 0) {
        y = -y;
    }
    *vx = cosf(rx + strafe) * m;
    *vy = y;
    *vz = sinf(rx + strafe) * m;
  }
  else {
    *vx = cosf(rx + strafe);
    *vy = 0;
    *vz = sinf(rx + strafe);
  }
}

void handle_movement(double dt) {
  static float dy = 0;
  State *s = &g->players->state;

  int sz = 0;
  int sx = 0;

  float m = dt * 1.0;
  if (glfwGetKey(g->window, CUBE_KEY_FORWARD)) sz--;
  if (glfwGetKey(g->window, CUBE_KEY_BACKWARD)) sz++;
  if (glfwGetKey(g->window, CUBE_KEY_LEFT)) sx--;
  if (glfwGetKey(g->window, CUBE_KEY_RIGHT)) sx++;
  if (glfwGetKey(g->window, GLFW_KEY_LEFT)) s->rx -= m;
  if (glfwGetKey(g->window, GLFW_KEY_RIGHT)) s->rx += m;
  if (glfwGetKey(g->window, GLFW_KEY_UP)) s->ry += m;
  if (glfwGetKey(g->window, GLFW_KEY_DOWN)) s->ry -= m;

  float vx, vy, vz;

  get_motion_vector(g->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
  if (glfwGetKey(g->window, CUBE_KEY_JUMP)) {
    if (g->flying) {
      vy = 1;
    } else if (dy == 0) {
      dy = 8;
    }
  }

  float speed = g->flying ? 20 : 5;
  int estimate = roundf(sqrtf(
      powf(vx * speed, 2) +
      powf(vy * speed + ABS(dy) * 2, 2) +
      powf(vz * speed, 2)) * dt * 8);
  int step = MAX(8, estimate);
  float ut = dt / step;
  vx = vx * ut * speed;
  vy = vy * ut * speed;
  vz = vz * ut * speed;
  for (int i = 0; i < step; i++) {
    if (g->flying) {
      dy = 0;
    }
    else {
      dy -= ut * 25;
      dy = MAX(dy, -250);
    }
    s->x += vx;
    s->y += vy + dy * ut;
    s->z += vz;
    // if (collide(2, &s->x, &s->y, &s->z)) {
    //   dy = 0;
    // }
  }
  // if (s->y < 0) {
  //   s->y = highest_block(s->x, s->z) + 2;
  // }
}

int main(void){
  printf("Cubes game started...\n");

  if (!glfwInit()) {
    printf("Failed to initialize GLFW.\n");
    return -1;
  }

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

  // LOAD TEXTURES
  GLuint texture;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  load_png_texture("textures/texture.png");

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
  g->game_running = true;

  reset_model();

  FPS fps = {0, 0, 0};

  g->ortho = 0;
  g->fov = 65;
  g->render_radius = RENDER_CHUNK_RADIUS;

  Player *me = g->players;
  State *s = &g->players->state;

  me->buffer = 0;
  g->player_count = 1;

  double previous = glfwGetTime();
  while(1){
    g->scale = get_scale_factor();
    glfwGetFramebufferSize(g->window, &g->width, &g->height);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    update_fps(&fps);
    double now = glfwGetTime();
    double dt = now - previous;
    dt = MIN(dt, 0.2);
    dt = MAX(dt, 0.0);
    previous = now;

    handle_movement(dt);

    // RENDERING
    del_buffer(me->buffer);
    me->buffer = gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);

    glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    render_item(&block_attrib);
    render_player(&block_attrib, me);

    glfwSwapBuffers(g->window);
    glfwPollEvents();

    if (glfwWindowShouldClose(g->window)) {
      break;
    }

    if (!g->game_running){
      break;
    }
  }

  glfwTerminate();
  return 0;
}
