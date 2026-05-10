#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static const int WIN_CONDITIONS[8][3]{
  {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
  {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
  {0, 4, 8}, {2, 4, 6}
}; /* TOtal of 8 possible ways to win tic-tac-toe
      first set are the rows then columns and the last two are the diagonals*/

