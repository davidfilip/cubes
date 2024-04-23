#include "item.h"
#include "util.h"

const int blocks[256][6] = {
  // w => (left, right, top, bottom, front, back) tiles
  {0, 0, 0, 0, 0, 0}, // 0 - empty
  {16, 16, 32, 0, 16, 16}, // 1 - grass
  {1, 1, 1, 1, 1, 1}, // 2 - sand
  {2, 2, 2, 2, 2, 2}, // 3 - stone
  {3, 3, 3, 3, 3, 3}, // 4 - brick
  {20, 20, 36, 4, 20, 20}, // 5 - wood
  {5, 5, 5, 5, 5, 5}, // 6 - cement
  {6, 6, 6, 6, 6, 6}, // 7 - dirt
  {7, 7, 7, 7, 7, 7}, // 8 - plank
};
