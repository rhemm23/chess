#include <stdio.h>

#include "game.h"

int main() {

  game_t game;
  init_game(&game);

  game.position.cells[1][3].is_occupied = false;

  move_list_t move_list;
  init_move_list(&move_list);

  calculate_legal_moves(&game, &move_list);

  for (int i = 0; i < move_list.count; i++) {
    printf("%d: (%d, %d) -> (%d, %d)\n", move_list.entries[i].piece.type, move_list.entries[i].start.row, move_list.entries[i].start.col, move_list.entries[i].end.row, move_list.entries[i].end.col);
  }
  printf("%d\n", move_list.count);
}
