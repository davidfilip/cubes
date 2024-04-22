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

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

typedef struct {
  int x;
  int y;
  int z;
  int w;
} Block;

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

void on_key_press(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
    printf("ESC PRESSED...\n");
    g->game_running = false;
  }
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods) {
  int control = mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
  int exclusive = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

  if (action != GLFW_PRESS) return;

  if (button == GLFW_MOUSE_BUTTON_LEFT && exclusive) {
    printf("LEFT BUTTON CLICK\n");
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT && exclusive) {
    printf("RIGHT BUTTON CLICK\n");
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

void draw_triangles_2d(Attrib *attrib, GLuint buffer, int count) {
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableVertexAttribArray(attrib->position);
  glEnableVertexAttribArray(attrib->uv);
  glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE,
      sizeof(GLfloat) * 4, 0);
  glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
      sizeof(GLfloat) * 4, (GLvoid *)(sizeof(GLfloat) * 2));
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(attrib->position);
  glDisableVertexAttribArray(attrib->uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
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

GLuint gen_text_buffer(float x, float y, float n, char *text) {
  int length = strlen(text);
  GLfloat *data = malloc_faces(4, length);
  for (int i = 0; i < length; i++) {
    make_character(data + i * 24, x, y, n / 2, n, text[i]);
    x += n;
  }
  return gen_faces(4, length, data);
}

void draw_text(Attrib *attrib, GLuint buffer, int length) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  draw_triangles_2d(attrib, buffer, length * 6);
  glDisable(GL_BLEND);
}

void render_blocks(Attrib *attrib, Player *player) {
  State *s = &player->state;
  float matrix[16];
  set_matrix_3d(
    matrix, g->width, g->height,
    s->x, s->y, s->z, s->rx, s->ry, g->fov, g->ortho, g->render_radius);

  float planes[6][4];
  frustum_planes(planes, g->render_radius, matrix);
  glUseProgram(attrib->program);
  glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
  glUniform3f(attrib->camera, s->x, s->y, s->z);
  glUniform1i(attrib->sampler, 0);
  glUniform1i(attrib->extra1, 2);
  glUniform1f(attrib->extra2, 0.5); //light
  glUniform1f(attrib->extra3, g->render_radius * CHUNK_SIZE);
  glUniform1i(attrib->extra4, g->ortho);
  glUniform1f(attrib->timer, 1); // time of the day function

  GLuint buffer = gen_cube_buffer(0, 3, -5, 1, 1);
  draw_triangles_3d_ao(attrib, buffer, 36);
}

void render_text(Attrib *attrib, int justify, float x, float y, float n, char *text) {
  float matrix[16];
  set_matrix_2d(matrix, g->width, g->height);
  glUseProgram(attrib->program);
  glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
  glUniform1i(attrib->sampler, 1);
  glUniform1i(attrib->extra1, 0);
  int length = strlen(text);
  x -= n * justify * (length - 1) / 2;
  GLuint buffer = gen_text_buffer(x, y, n, text);
  draw_text(attrib, buffer, length);
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
  g->flying = 1;
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


void handle_mouse_input() {
  int exclusive = glfwGetInputMode(g->window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

  static double px = 0;
  static double py = 0;
  State *s = &g->players->state;

  if (exclusive && (px || py)) {
    double mx, my;
    glfwGetCursorPos(g->window, &mx, &my);
    float m = 0.0025;
    s->rx += (mx - px) * m;
    if (INVERT_MOUSE) {
      s->ry += (my - py) * m;
    }
    else {
      s->ry -= (my - py) * m;
    }
    if (s->rx < 0) {
      s->rx += RADIANS(360);
    }
    if (s->rx >= RADIANS(360)){
      s->rx -= RADIANS(360);
    }
    s->ry = MAX(s->ry, -RADIANS(90));
    s->ry = MIN(s->ry, RADIANS(90));
    px = mx;
    py = my;
  }
  else {
    glfwGetCursorPos(g->window, &px, &py);
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
  //if (s->y < 0) {
   //s->y = highest_block(s->x, s->z) + 2;
 // }
  if(s -> y < 0) {
    s->y = 0;
  }
}

int main(void){
  printf("Cubes game started...\n");

  if (!glfwInit()) {
    printf("Failed to initialize GLFW.\n");
    return -1;
  }

  g->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Cubes", NULL, NULL);
  if (!g->window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(g->window);
  glfwSwapInterval(VSYNC);
  glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetKeyCallback(g->window, on_key_press);
  glfwSetMouseButtonCallback(g->window, on_mouse_button);

  if (glewInit() != GLEW_OK) {
    printf("Failed to initialize GLEW.\n");
    return -1;
  }

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLogicOp(GL_INVERT);

  // LOAD TEXTURES
  GLuint texture;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  load_png_texture("textures/texture.png");

  GLuint font;
  glGenTextures(1, &font);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, font);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  load_png_texture("textures/font.png");

  // SHADERS
  Attrib block_attrib = {0};
  Attrib text_attrib = {0};
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

  program = load_program("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
  text_attrib.program = program;
  text_attrib.position = glGetAttribLocation(program, "position");
  text_attrib.uv = glGetAttribLocation(program, "uv");
  text_attrib.matrix = glGetUniformLocation(program, "matrix");
  text_attrib.sampler = glGetUniformLocation(program, "sampler");
  text_attrib.extra1 = glGetUniformLocation(program, "is_sign");

  FPS fps = {0, 0, 0};
  reset_model();

  g->ortho = 0;
  g->fov = 65;
  g->render_radius = RENDER_CHUNK_RADIUS;

  Player *me = g->players;
  State *s = &g->players->state;

  me->buffer = 0;
  g->player_count = 1;

  g->game_running = true;
  double previous = glfwGetTime();

  //set_block(3, 3, 1, 1);
  //set_block(1, 2, 0, 1);

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

    handle_mouse_input();
    handle_movement(dt);

    // RENDERING
    del_buffer(me->buffer);
    me->buffer = gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);

    Player *player = g->players;

    glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    //render_player(&block_attrib, me);
    render_blocks(&block_attrib, player);

    // RENDER TEXT //
    char text_buffer[1024];
    float ts = 20 * g->scale; // TODO: was 12 * g->scale
    float tx = ts / 2;
    float ty = g->height - ts;
    if (SHOW_INFO_TEXT) {
      snprintf(text_buffer, 1024,
        "Position: %.2f, %.2f, %.2f, FPS: %d",
        s->x, s->y, s->z,fps.fps);

      render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
      ty -= ts * 2;
    }

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
