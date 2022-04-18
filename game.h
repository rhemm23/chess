#ifndef GAME_H
#define GAME_H

#define DEFAULT_MOVE_LIST_CAPACITY 100

#include <stdbool.h>
#include <stdlib.h>

typedef enum color {
  WHITE,
  BLACK
} color_t;

typedef enum piece_type {
  PAWN,
  KNIGHT,
  BISHOP,
  ROOK,
  QUEEN,
  KING
} piece_type_t;

typedef struct piece {
  color_t color;
  piece_type_t type;
} piece_t;

typedef struct cell {
  bool is_occupied;
  piece_t piece;
} cell_t;

typedef struct board {
  cell_t cells[8][8];
} board_t;

typedef struct cell_loc {
  int row;
  int col;
} cell_loc_t;

typedef struct move {
  cell_loc_t start;
  cell_loc_t end;
  piece_t piece;
  bool did_promote;
  bool did_capture_piece;
  bool is_king_side_castle;
  bool is_queen_side_castle;
  cell_loc_t captured_piece_loc;
  piece_t captured_piece;
  piece_t promoted_piece;
} move_t;

typedef struct move_list {
  int count;
  int capacity;
  move_t *entries;
} move_list_t;

typedef struct game {
  move_list_t moves_made;
  board_t position;
} game_t;

void init_game(game_t *game);
void set_starting_position(board_t *board);
void init_move_list(move_list_t *move_list);
void add_to_move_list(move_list_t *move_list, move_t move);
void calculate_legal_moves(game_t *game, move_list_t *move_list);

#endif
