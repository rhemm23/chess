#include <stdio.h>

#include "game.h"

int main() {

  game_t game;
  init_game(&game);

  game.position.cells[0][1].is_occupied = false;
  game.position.cells[0][2].is_occupied = false;
  game.position.cells[0][4].is_occupied = false;
  game.position.cells[0][5].is_occupied = false;
  game.position.cells[0][6].is_occupied = false;

  move_list_t move_list;
  init_move_list(&move_list);

  calculate_legal_moves(&game, &move_list);

  for (int i = 0; i < move_list.count; i++) {
    if (move_list.entries[i].piece.type == KING) {
      printf("%d\n\n", i);
      printf("queen: %d\n", move_list.entries[i].is_queen_side_castle);
      printf("king: %d\n", move_list.entries[i].is_king_side_castle);
    }
  }
}
