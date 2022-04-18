#include "game.h"

static color_t opposite_color(color_t color) {
  return (color == WHITE) ? BLACK : WHITE;
}

static int pawn_direction(color_t color) {
  return (color == WHITE) ? 1 : -1;
}

void set_starting_position(board_t *board) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      board->cells[row][col].is_occupied = (row < 2 || row > 5);
    }
  }
  for (int col = 0; col < 8; col++) {
    board->cells[1][col].piece = (piece_t){ WHITE, PAWN };
    board->cells[6][col].piece = (piece_t){ BLACK, PAWN };
  }
  board->cells[0][0].piece = (piece_t){ WHITE, ROOK };
  board->cells[0][7].piece = (piece_t){ WHITE, ROOK };
  board->cells[7][0].piece = (piece_t){ BLACK, ROOK };
  board->cells[7][7].piece = (piece_t){ BLACK, ROOK };
  board->cells[0][1].piece = (piece_t){ WHITE, KNIGHT };
  board->cells[0][6].piece = (piece_t){ WHITE, KNIGHT };
  board->cells[7][1].piece = (piece_t){ BLACK, KNIGHT };
  board->cells[7][6].piece = (piece_t){ BLACK, KNIGHT };
  board->cells[0][2].piece = (piece_t){ WHITE, BISHOP };
  board->cells[0][5].piece = (piece_t){ WHITE, BISHOP };
  board->cells[7][2].piece = (piece_t){ BLACK, BISHOP };
  board->cells[7][5].piece = (piece_t){ BLACK, BISHOP };
  board->cells[0][4].piece = (piece_t){ WHITE, QUEEN };
  board->cells[0][3].piece = (piece_t){ WHITE, KING };
  board->cells[7][4].piece = (piece_t){ BLACK, QUEEN };
  board->cells[7][3].piece = (piece_t){ BLACK, KING };
}

void init_move_list(move_list_t *move_list) {
  move_list->count = 0;
  move_list->capacity = DEFAULT_MOVE_LIST_CAPACITY;
  move_list->entries = (move_t*)malloc(sizeof(move_t) * DEFAULT_MOVE_LIST_CAPACITY);
}

void init_game(game_t *game) {
  init_move_list(&game->moves_made);
  set_starting_position(&game->position);
}

void add_to_move_list(move_list_t *move_list, move_t move) {
  if (move_list->count == move_list->capacity) {
    move_list->capacity *= 2;
    move_list->entries = (move_t*)realloc(move_list->entries, sizeof(move_t) * move_list->capacity);
  }
  move_list->entries[move_list->count++] = move;
}

static void add_simple_move(
  move_list_t *move_list,
  cell_loc_t start,
  cell_loc_t end,
  piece_t piece
) {
  move_t move;
  move.end = end;
  move.start = start;
  move.piece = piece;
  move.did_promote = false;
  move.did_capture_piece = false;
  move.is_king_side_castle = false;
  move.is_queen_side_castle = false;
  add_to_move_list(move_list, move);
}

static void add_capturing_move(
  move_list_t *move_list,
  cell_loc_t start,
  cell_loc_t end,
  piece_t piece,
  cell_loc_t captured_piece_loc,
  piece_t captured_piece
) {
  move_t move;
  move.end = end;
  move.start = start;
  move.piece = piece;
  move.did_promote = false;
  move.did_capture_piece = true;
  move.is_king_side_castle = false;
  move.is_queen_side_castle = false;
  move.captured_piece = captured_piece;
  move.captured_piece_loc = captured_piece_loc;
  add_to_move_list(move_list, move);
}

static void add_promoting_move(
  move_list_t *move_list,
  cell_loc_t start,
  cell_loc_t end,
  piece_t piece,
  piece_t promoted_piece
) {
  move_t move;
  move.end = end;
  move.start = start;
  move.piece = piece;
  move.did_promote = true;
  move.did_capture_piece = false;
  move.is_king_side_castle = false;
  move.is_queen_side_castle = false;
  move.promoted_piece = promoted_piece;
  add_to_move_list(move_list, move);
}

static void add_capturing_and_promoting_move(
  move_list_t *move_list,
  cell_loc_t start,
  cell_loc_t end,
  piece_t piece,
  cell_loc_t captured_piece_loc,
  piece_t captured_piece,
  piece_t promoted_piece
) {
  move_t move;
  move.end = end;
  move.start = start;
  move.piece = piece;
  move.did_promote = true;
  move.did_capture_piece = true;
  move.is_king_side_castle = false;
  move.is_queen_side_castle = false;
  move.promoted_piece = promoted_piece;
  move.captured_piece = captured_piece;
  move.captured_piece_loc = captured_piece_loc;
  add_to_move_list(move_list, move);
}

static void add_castling_move(
  move_list_t *move_list,
  cell_loc_t start,
  cell_loc_t end,
  piece_t piece,
  bool is_king_side
) {
  move_t move;
  move.end = end;
  move.start = start;
  move.piece = piece;
  move.did_promote = true;
  move.did_capture_piece = true;
  move.is_king_side_castle = is_king_side;
  move.is_queen_side_castle = !is_king_side;
  move.promoted_piece = promoted_piece;
  move.captured_piece = captured_piece;
  move.captured_piece_loc = captured_piece_loc;
  add_to_move_list(move_list, move);
}

static bool is_valid_cell_location(cell_loc_t cell_loc) {
  return (cell_loc.row >= 0) &&
         (cell_loc.row <= 7) &&
         (cell_loc.col >= 0) &&
         (cell_loc.row <= 7);
}

static int find_knight_cells(cell_loc_t cell_loc, cell_loc_t *result_cell_locs) {
  int count = 0;
  int knight_deltas[] = { -2, -1, 1, 2 };
  for (int row_delta_idx = 0; row_delta_idx < 4; row_delta_idx++) {
    for (int col_delta_idx = 0; col_delta_idx < 4; col_delta_idx++) {
      cell_loc_t candidate = (cell_loc_t) {
        cell_loc.row + knight_deltas[row_delta_idx],
        cell_loc.col + knight_deltas[col_delta_idx]
      };
      if (abs(knight_deltas[row_delta_idx]) != abs(knight_deltas[col_delta_idx]) && is_valid_cell_location(candidate)) {
        result_cell_locs[count++] = candidate;
      }
    }
  }
  return count;
}

bool is_cell_endangered_by_color(board_t *board, cell_loc_t cell_loc, color_t color) {
  int pawn_direction = (color == WHITE) ? -1 : 1;
  if (cell_loc.row > 1 && cell_loc.row < 6) {
    if (cell_loc.col > 0) {
      cell_t candidate = board->cells[row + pawn_direction][col - 1];
      if (candidate.is_occupied && candidate.piece.color == color && candidate.piece.type == PAWN) {
        return true;
      }
    }
    if (cell_loc.col < 7) {
      cell_t candidate = board->cells[row + pawn_direction][col + 1];
      if (candidate.is_occupied && candidate.piece.color == color && candidate.piece.type == PAWN) {
        return true;
      }
    }
  }
  cell_loc_t knight_cell_locs[8];
  int knight_cell_loc_cnt = find_knight_cells(cell_loc, &knight_cell_locs[0]);
  for (int i = 0; i < knight_cell_loc_cnt; i++) {
    cell_t candidate = board->cells[knight_cell_locs[i].row][knight_cell_locs[i].col];
    if (candidate.is_occupied && candidate.piece.color == color && candidate.piece.type == KNIGHT) {
      return true;
    }
  }
  int deltas[] = { -1, 1 };
  for (int row_di = 0; row_di < 2; row_di++) {
    for (int col_di = 0; col_di < 2; col_di++) {
      for (int m = 1; m < 8; m++) {
        cell_loc_t can = (cell_loc_t) {
          cell_loc.row + m * deltas[row_di],
          cell_loc.col + m * deltas[col_di]
        };
        if (is_valid_cell_location(can)) {
          cell_t cell = board->cells[can.row][can.col];
          if (cell.is_occupied) {
            if (cell.piece.color == color && (cell.piece.type == BISHOP || cell.piece.type == QUEEN)) {
              return true;
            } else {
              break;
            }
          }
        } else {
          break;
        }
      }
    }
  }
  for (int di = 0; di < 2; di++) {
    for (int is_row_delta = 0; is_row_delta < 2; is_row_delta++) {
      for (int m = 1; m < 8; m++) {
        cell_loc_t can = (cell_loc_t) {
          cell_loc.row + (is_row_delta == 1 ? 1 : 0) * m * deltas[di],
          cell_loc.col + (is_row_delta == 0 ? 1 : 0) * m * deltas[di]
        };
        if (is_valid_cell_location(can)) {
          cell_t cell = board->cells[can.row][can.col];
          if (cell.is_occupied) {
            if (cell.piece.color == color && (cell.piece.type == ROOK || cell.piece.type == QUEEN)) {
              return true;
            } else {
              break;
            }
          }
        } else {
          break;
        }
      }
    }
  }
  int king_deltas[] = { -1, 0, 1 };
  for (int row_di = 0; row_di < 3; row_di++) {
    for (int col_di = 0; col_di < 3; col_di++) {
      cell_loc_t can = (cell_loc_t) {
        cell_loc.row + king_deltas[row_di],
        cell_loc.col + king_deltas[col_di]
      };
      if ((king_deltas[row_di] != 0 || king_deltas[col_di] != 0) && is_valid_cell_location(can)) {
        return true;
      }
    }
  }
  return false;
}

void calculate_legal_moves(game_t *game, move_list_t *move_list) {
  color_t color_to_move = (game->moves_made.count % 2 == 0) ? WHITE : BLACK;
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      cell_t cell = game->position.cells[row][col];
      if (cell.is_occupied && cell.piece.color == color_to_move) {
        int direction, king_moved, king_rook_moved, queen_rook_moved;
        int deltas[] = { -1, 1 };
        int king_deltas[] = { -1, 0, 1 };
        int knight_deltas[] = { -2, -1, 1, 2 };
        switch (cell.piece.type) {
          case PAWN:
            direction = (color_to_move == WHITE) ? 1 : -1;
            if (!game->position.cells[row + direction][col].is_occupied) {
              if ((row == 6 && color_to_move == WHITE) || (row == 1 && color_to_move == BLACK)) {
                for (int piece_type = KNIGHT; piece_type <= QUEEN; piece_type++) {
                  add_promoting_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row + direction, col },
                    cell.piece,
                    (piece_t) {
                      color_to_move,
                      (piece_type_t)piece_type
                    }
                  );
                }
              } else {
                add_simple_move(
                  move_list,
                  (cell_loc_t) { row, col },
                  (cell_loc_t) { row + direction, col },
                  cell.piece
                );
              }
              if (((row == 1 && color_to_move == WHITE) || (row == 6 && color_to_move == BLACK)) && !game->position.cells[row + 2 * direction][col].is_occupied) {
                add_simple_move(
                  move_list,
                  (cell_loc_t) { row, col },
                  (cell_loc_t) { row + 2 * direction, col },
                  cell.piece
                );
              }
            }
            if (col > 0) {
              cell_t candidate = game->position.cells[row + direction][col - 1];
              if (candidate.is_occupied && candidate.piece.color == opposite_color(color_to_move)) {
                if ((row == 6 && color_to_move == WHITE) || (row == 1 && color_to_move == BLACK)) {
                  for (int piece_type = KNIGHT; piece_type <= QUEEN; piece_type++) {
                    add_capturing_and_promoting_move(
                      move_list,
                      (cell_loc_t) { row, col },
                      (cell_loc_t) { row + direction, col - 1 },
                      cell.piece,
                      (cell_loc_t) { row + direction, col - 1 },
                      candidate.piece,
                      (piece_t) {
                        color_to_move,
                        (piece_type_t)piece_type
                      }
                    );
                  }
                } else {
                  add_simple_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row + direction, col - 1 },
                    cell.piece
                  );
                }
              }
            }
            if (col < 7) {
              cell_t candidate = game->position.cells[row + direction][col + 1];
              if (candidate.is_occupied && candidate.piece.color == opposite_color(color_to_move)) {
                if ((row == 6 && color_to_move == WHITE) || (row == 1 && color_to_move == BLACK)) {
                  for (int piece_type = KNIGHT; piece_type <= QUEEN; piece_type++) {
                    add_capturing_and_promoting_move(
                      move_list,
                      (cell_loc_t) { row, col },
                      (cell_loc_t) { row + direction, col + 1 },
                      cell.piece,
                      (cell_loc_t) { row + direction, col + 1 },
                      candidate.piece,
                      (piece_t) {
                        color_to_move,
                        (piece_type_t)piece_type
                      }
                    );
                  }
                } else {
                  add_simple_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row + direction, col + 1 },
                    cell.piece
                  );
                }
              }
            }
            if (color_to_move == WHITE && row == 4 && game->moves_made.count > 0) {
              move_t previous_move = game->moves_made.entries[game->moves_made.count - 1];
              if (previous_move.start.row == 6 &&
                  previous_move.end.row == 4 &&
                  previous_move.piece.color == BLACK &&
                  previous_move.piece.type == PAWN) {
                if (col > 0 && previous_move.start.col == (col - 1)) {
                  add_capturing_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row + 1, col - 1 },
                    cell.piece,
                    previous_move.end,
                    previous_move.piece
                  );
                } else if (col < 7 && previous_move.start.col == (col + 1)) {
                  add_capturing_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row + 1, col + 1 },
                    cell.piece,
                    previous_move.end,
                    previous_move.piece
                  );
                }
              }
            } else if (color_to_move == BLACK && col == 3 && game->moves_made.count > 0) {
              move_t previous_move = game->moves_made.entries[game->moves_made.count - 1];
              if (previous_move.start.row == 1 &&
                  previous_move.end.row == 3 &&
                  previous_move.piece.color == WHITE &&
                  previous_move.piece.type == PAWN) {
                if (col > 0 && previous_move.start.col == (col - 1)) {
                  add_capturing_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row - 1, col - 1 },
                    cell.piece,
                    previous_move.end,
                    previous_move.piece
                  );
                } else if (col < 7 && previous_move.start.col == (col + 1)) {
                  add_capturing_move(
                    move_list,
                    (cell_loc_t) { row, col },
                    (cell_loc_t) { row - 1, col + 1 },
                    cell.piece,
                    previous_move.end,
                    previous_move.piece
                  );
                }
              }
            }
            break;

          case KNIGHT:
            for (int row_delta_idx = 0; row_delta_idx < 4; row_delta_idx++) {
              for (int col_delta_idx = 0; col_delta_idx < 4; col_delta_idx++) {
                int row_delta = knight_deltas[row_delta_idx];
                int col_delta = knight_deltas[col_delta_idx];
                int new_row = row + row_delta;
                int new_col = col + col_delta;
                if (abs(row_delta) != abs(col_delta) && new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                  cell_t candidate = game->position.cells[new_row][new_col];
                  if (!candidate.is_occupied) {
                    add_simple_move(
                      move_list,
                      (cell_loc_t) { row, col },
                      (cell_loc_t) { new_row, new_col },
                      cell.piece
                    );
                  } else if (candidate.piece.color == opposite_color(color_to_move)) {
                    add_capturing_move(
                      move_list,
                      (cell_loc_t) { row, col },
                      (cell_loc_t) { new_row, new_col },
                      cell.piece,
                      (cell_loc_t) { new_row, new_col },
                      candidate.piece
                    );
                  }
                }
              }
            }
            break;

          case BISHOP:
            for (int row_delta_idx = 0; row_delta_idx < 2; row_delta_idx++) {
              for (int col_delta_idx = 0; col_delta_idx < 2; col_delta_idx++) {
                for (int multiplier = 1; multiplier < 8; multiplier++) {
                  int new_row = row + (deltas[row_delta_idx] * multiplier);
                  int new_col = col + (deltas[col_delta_idx] * multiplier);
                  if (new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                    cell_t candidate = game->position.cells[new_row][new_col];
                    if (!candidate.is_occupied) {
                      add_simple_move(
                        move_list,
                        (cell_loc_t) { row, col },
                        (cell_loc_t) { new_row, new_col },
                        cell.piece
                      );
                    } else {
                      if (candidate.piece.color == opposite_color(color_to_move)) {
                        add_capturing_move(
                          move_list,
                          (cell_loc_t) { row, col },
                          (cell_loc_t) { new_row, new_col },
                          cell.piece,
                          (cell_loc_t) { new_row, new_col },
                          candidate.piece
                        );
                      }
                      break;
                    }
                  }
                }
              }
            }
            break;

          case ROOK:
            for (int delta_idx = 0; delta_idx < 2; delta_idx++) {
              for (int is_row_delta = 0; is_row_delta < 2; is_row_delta++) {
                for (int multiplier = 1; multiplier < 8; multiplier++) {
                  int new_row = (is_row_delta == 1) ? (row + deltas[delta_idx] * multiplier) : row;
                  int new_col = (is_row_delta == 0) ? (col + deltas[delta_idx] * multiplier) : col;
                  if (new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                    cell_t candidate = game->position.cells[new_row][new_col];
                    if (!candidate.is_occupied) {
                      add_simple_move(
                        move_list,
                        (cell_loc_t) { row, col },
                        (cell_loc_t) { new_row, new_col },
                        cell.piece
                      );
                    } else {
                      if (candidate.piece.color == opposite_color(color_to_move)) {
                        add_capturing_move(
                          move_list,
                          (cell_loc_t) { row, col },
                          (cell_loc_t) { new_row, new_col },
                          cell.piece,
                          (cell_loc_t) { new_row, new_col },
                          candidate.piece
                        );
                      }
                      break;
                    }
                  }
                }
              }
            }
            break;

          case QUEEN:
            for (int delta_idx = 0; delta_idx < 2; delta_idx++) {
              for (int is_row_delta = 0; is_row_delta < 2; is_row_delta++) {
                for (int multiplier = 1; multiplier < 8; multiplier++) {
                  int new_row = (is_row_delta == 1) ? (row + deltas[delta_idx] * multiplier) : row;
                  int new_col = (is_row_delta == 0) ? (col + deltas[delta_idx] * multiplier) : col;
                  if (new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                    cell_t candidate = game->position.cells[new_row][new_col];
                    if (!candidate.is_occupied) {
                      add_simple_move(
                        move_list,
                        (cell_loc_t) { row, col },
                        (cell_loc_t) { new_row, new_col },
                        cell.piece
                      );
                    } else {
                      if (candidate.piece.color == opposite_color(color_to_move)) {
                        add_capturing_move(
                          move_list,
                          (cell_loc_t) { row, col },
                          (cell_loc_t) { new_row, new_col },
                          cell.piece,
                          (cell_loc_t) { new_row, new_col },
                          candidate.piece
                        );
                      }
                      break;
                    }
                  }
                }
              }
            }
            for (int row_delta_idx = 0; row_delta_idx < 2; row_delta_idx++) {
              for (int col_delta_idx = 0; col_delta_idx < 2; col_delta_idx++) {
                for (int multiplier = 1; multiplier < 8; multiplier++) {
                  int new_row = row + (deltas[row_delta_idx] * multiplier);
                  int new_col = col + (deltas[col_delta_idx] * multiplier);
                  if (new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                    cell_t candidate = game->position.cells[new_row][new_col];
                    if (!candidate.is_occupied) {
                      add_simple_move(
                        move_list,
                        (cell_loc_t) { row, col },
                        (cell_loc_t) { new_row, new_col },
                        cell.piece
                      );
                    } else {
                      if (candidate.piece.color == opposite_color(color_to_move)) {
                        add_capturing_move(
                          move_list,
                          (cell_loc_t) { row, col },
                          (cell_loc_t) { new_row, new_col },
                          cell.piece,
                          (cell_loc_t) { new_row, new_col },
                          candidate.piece
                        );
                      }
                      break;
                    }
                  }
                }
              }
            }
            break;

          case KING:
            for (int row_delta_idx = 0; row_delta_idx < 2; row_delta_idx++) {
              for (int col_delta_idx = 0; col_delta_idx < 2; col_delta_idx++) {
                int row_delta = king_deltas[row_delta_idx];
                int col_delta = king_deltas[col_delta_idx];
                int new_row = row + row_delta;
                int new_col = col + col_delta;
                if ((row_delta != 0 || col_delta != 0) && new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                  cell_t candidate = game->position.cells[new_row][new_col];
                  if (!candidate.is_occupied) {
                    add_simple_move(
                      move_list,
                      (cell_loc_t) { row, col },
                      (cell_loc_t) { new_row, new_col },
                      cell.piece
                    );
                  } else {
                    if (candidate.piece.color == opposite_color(color_to_move)) {
                      add_capturing_move(
                        move_list,
                        (cell_loc_t) { row, col },
                        (cell_loc_t) { new_row, new_col },
                        cell.piece,
                        (cell_loc_t) { new_row, new_col },
                        candidate.piece
                      );
                    }
                    break;
                  }
                }
              }
            }
            king_moved = 0;
            king_rook_moved = 0;
            queen_rook_moved = 0;
            for (int move_idx = 0; move_idx < game->moves_made.count; move_idx++) {
              move_t move = game->moves_made.entries[move_idx];
              if (move.is_king_side_castle || move.is_queen_side_castle || (move.piece.color == color_to_move && move.piece.type == KING)) {
                king_moved = 1;
                break;
              }
              if (move.piece.color == color_to_move && move.start.row == ((color_to_move == WHITE) ? 0 : 7)) {
                if (move.start.col == 0) {
                  queen_rook_moved = 1;
                } else if (move.start.col == 7) {
                  king_rook_moved = 1;
                }
              }
            }
            if (!king_moved && !is_cell_endangered_by_color(game->position, (cell_loc_t) { row, col }, opposite_color(color_to_move))) {
              int base_row = (color_to_move == WHITE) ? 0 : 7;
              if (!king_rook_moved &&
                  !game->position.cells[base_row][1].is_occupied &&
                  !game->position.cells[base_row][2].is_occupied &&
                  !is_cell_endangered_by_color(game->position, (cell_loc_t) { base_row, 2 }, opposite_color(color_to_move))) {
                add_castling_move(
                  move_list,
                  (cell_loc_t) { row, col },
                  (cell_loc_t) { row, 1 },
                  cell.piece,
                  true
                );
              }
              if (!queen_rook_moved &&
                  !game->position.cells[base_row][4].is_occupied &&
                  !game->position.cells[base_row][5].is_occupied &&
                  !game->position.cells[base_row][6].is_occupied &&
                  !is_cell_endangered_by_color(game->position, (cell_loc_t) { base_row, 4 }, opposite_color(color_to_move)) &&
                  !is_cell_endangered_by_color(game->position, (cell_loc_t) { base_row, 5 }, opposite_color(color_to_move))) {
                add_castling_move(
                  move_list,
                  (cell_loc_t) { row, col },
                  (cell_loc_t) { row, 6 },
                  cell.piece,
                  false
                );
              }
            }
            break;
        }
      }
    }
  }
}
