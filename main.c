#include <stdio.h>

#include "game.h"

int main() {

  game_t game;
  init_game(&game);

  move_list_t move_list;
  init_move_list(&move_list);

  calculate_legal_moves(&game, &move_list);
  printf("%d\n", move_list.count);
}
