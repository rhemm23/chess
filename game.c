#include "game.h"

static int deltas[] = { -1, 1 };
static int king_deltas[] = { -1, 0, 1 };
static int knight_deltas[] = { -2, -1, 1, 2 };

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

static bool is_valid_coord(coord_t coord) {
  return (coord.row >= 0) &&
         (coord.row <= 7) &&
         (coord.col >= 0) &&
         (coord.col <= 7);
}

bool is_cell_endangered_by_color(board_t *board, coord_t coord, color_t color) {
  for (int di = 0; di < 2; di++) {
    coord_t can_coord = (coord_t) {
      coord.row + (color == WHITE) ? -1 : 1,
      coord.col + deltas[di]
    };
    if (is_valid_coord(can_coord)) {
      cell_t candidate = board->cells[coord.row][coord.col];
      if (candidate.is_occupied && candidate.piece.color == color && candidate.piece.type == PAWN) {
        return true;
      }
    }
  }
  for (int row_di = 0; row_di < 4; row_di++) {
    for (int col_di = 0; col_di < 4; col_di++) {
      coord_t can_coord = (coord_t) {
        coord.row + knight_deltas[row_di],
        coord.col + knight_deltas[col_di]
      };
      if (abs(knight_deltas[row_di]) != abs(knight_deltas[col_di]) && is_valid_coord(can_coord)) {
        cell_t candidate = board->cells[coord.row][coord.col];
        if (candidate.is_occupied && candidate.piece.color == color && candidate.piece.type == KNIGHT) {
          return true;
        }
      }
    }
  }
  for (int row_di = 0; row_di < 2; row_di++) {
    for (int col_di = 0; col_di < 2; col_di++) {
      for (int m = 1; m < 8; m++) {
        coord_t can_coord = (coord_t) {
          coord.row + m * deltas[row_di],
          coord.col + m * deltas[col_di]
        };
        if (is_valid_coord(can_coord)) {
          cell_t cell = board->cells[can_coord.row][can_coord.col];
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
        coord_t can_coord = (coord_t) {
          coord.row + (is_row_delta == 1) * m * deltas[di],
          coord.col + (is_row_delta == 0) * m * deltas[di]
        };
        if (is_valid_coord(can_coord)) {
          cell_t cell = board->cells[can_coord.row][can_coord.col];
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
  for (int row_di = 0; row_di < 3; row_di++) {
    for (int col_di = 0; col_di < 3; col_di++) {
      if (king_deltas[row_di] != 0 || king_deltas[col_di] != 0) {
        coord_t can_coord = (coord_t) {
          coord.row + king_deltas[row_di],
          coord.col + king_deltas[col_di]
        };
        if (is_valid_coord(can_coord)) {
          cell_t cell = board->cells[can_coord.row][can_coord.col];
          if (cell.is_occupied && cell.piece.color == color && cell.piece.type == KING) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void calculate_legal_moves(game_t *game, move_list_t *move_list) {
  color_t color_to_move = (game->moves_made.count % 2 == 0) ? WHITE : BLACK;
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      bool can_king_side_castle;
      bool can_queen_side_castle;
      cell_t cell = game->position.cells[row][col];
      if (cell.is_occupied && cell.piece.color == color_to_move) {
        switch (cell.piece.type) {
          case PAWN:
            if (!game->position.cells[row + pawn_direction(color_to_move)][col].is_occupied) {
              if ((color_to_move == WHITE && row == 6) || (color_to_move == BLACK && row == 1)) {
                for (int piece_type = KNIGHT; piece_type <= QUEEN; piece_type++) {
                  add_to_move_list(
                    move_list,
                    (move_t) {
                      cell.piece,
                      (coord_t) { row, col },
                      (coord_t) { row + pawn_direction(color_to_move), col },
                      false,
                      false,
                      false,
                      (piece_type_t) piece_type
                    }
                  );
                }
              } else {
                add_to_move_list(
                  move_list,
                  (move_t) {
                    cell.piece,
                    (coord_t) { row, col },
                    (coord_t) { row + pawn_direction(color_to_move), col },
                    false,
                    false,
                    false,
                    (piece_type_t) 0
                  }
                );
              }
              if (((color_to_move == WHITE && row == 1) ||
                  (color_to_move == BLACK && row == 6)) &&
                  !game->position.cells[row + pawn_direction(color_to_move) * 2][col].is_occupied) {
                add_to_move_list(
                  move_list,
                  (move_t) {
                    cell.piece,
                    (coord_t) { row, col },
                    (coord_t) { row + pawn_direction(color_to_move) * 2, col },
                    false,
                    false,
                    false,
                    (piece_type_t) 0
                  }
                );
              }
            }
            for (int delta_idx = 0; delta_idx < 2; delta_idx++) {
              coord_t coord = (coord_t) {
                row + pawn_direction(color_to_move),
                col + deltas[delta_idx]
              };
              if (is_valid_coord(coord)) {
                cell_t can = game->position.cells[coord.row][coord.col];
                if (can.is_occupied && can.piece.color == opposite_color(color_to_move)) {
                  if ((color_to_move == WHITE && row == 6) || (color_to_move == BLACK && row == 1)) {
                    for (int piece_type = KNIGHT; piece_type <= QUEEN; piece_type++) {
                      add_to_move_list(
                        move_list,
                        (move_t) {
                          cell.piece,
                          (coord_t) { row, col },
                          coord,
                          false,
                          false,
                          false,
                          (piece_type_t) piece_type
                        }
                      );
                    }
                  } else {
                    add_to_move_list(
                      move_list,
                      (move_t) {
                        cell.piece,
                        (coord_t) { row, col },
                        coord,
                        false,
                        false,
                        false,
                        (piece_type_t) 0
                      }
                    );
                  }
                }
              }
            }
            if (((color_to_move == WHITE && row == 4) || (color_to_move == BLACK && row == 3)) && game->moves_made.count > 0) {
              move_t previous_move = game->moves_made.entries[game->moves_made.count - 1];
              if (previous_move.piece.type == PAWN &&
                  previous_move.piece.color == opposite_color(color_to_move) &&
                  previous_move.start.row == (color_to_move == WHITE) ? 6 : 1 &&
                  previous_move.end.row == (color_to_move == WHITE) ? 4 : 3 &&
                  abs(previous_move.end.col - col) == 1) {
                add_to_move_list(
                  move_list,
                  (move_t) {
                    cell.piece,
                    (coord_t) { row, col },
                    (coord_t) { row + pawn_direction(color_to_move), previous_move.end.col },
                    true,
                    false,
                    false,
                    (piece_type_t) 0
                  }
                );
              }
            }
            break;

          case KNIGHT:
            for (int row_di = 0; row_di < 4; row_di++) {
              for (int col_di = 0; col_di < 4; col_di++) {
                coord_t coord = (coord_t) {
                  row + knight_deltas[row_di],
                  col + knight_deltas[col_di]
                };
                if (abs(knight_deltas[row_di]) != abs(knight_deltas[col_di]) && is_valid_coord(coord)) {
                  cell_t can = game->position.cells[coord.row][coord.col];
                  if (!can.is_occupied || can.piece.color == opposite_color(color_to_move)) {
                    add_to_move_list(
                      move_list,
                      (move_t) {
                        cell.piece,
                        (coord_t) { row, col },
                        coord,
                        false,
                        false,
                        false,
                        (piece_type_t) 0
                      }
                    );
                  }
                }
              }
            }
            break;

          case BISHOP:
            for (int row_di = 0; row_di < 2; row_di++) {
              for (int col_di = 0; col_di < 2; col_di++) {
                for (int m = 1; m < 8; m++) {
                  coord_t coord = (coord_t) {
                    row + deltas[row_di] * m,
                    col + deltas[col_di] * m
                  };
                  if (is_valid_coord(coord)) {
                    cell_t can = game->position.cells[coord.row][coord.col];
                    if (!can.is_occupied || can.piece.color == opposite_color(color_to_move)) {
                      add_to_move_list(
                        move_list,
                        (move_t) {
                          cell.piece,
                          (coord_t) { row, col },
                          coord,
                          false,
                          false,
                          false,
                          (piece_type_t) 0
                        }
                      );
                      if (can.is_occupied) {
                        break;
                      }
                    } else {
                      break;
                    }
                  } else {
                    break;
                  }
                }
              }
            }
            break;

          case ROOK:
            for (int di = 0; di < 2; di++) {
              for (int is_row_delta = 0; is_row_delta < 2; is_row_delta++) {
                for (int m = 1; m < 8; m++) {
                  coord_t coord = (coord_t) {
                    row + deltas[di] * m * (is_row_delta == 1),
                    col + deltas[di] * m * (is_row_delta == 0)
                  };
                  if (is_valid_coord(coord)) {
                    cell_t can = game->position.cells[coord.row][coord.col];
                    if (!can.is_occupied || can.piece.color == opposite_color(color_to_move)) {
                      add_to_move_list(
                        move_list,
                        (move_t) {
                          cell.piece,
                          (coord_t) { row, col },
                          coord,
                          false,
                          false,
                          false,
                          (piece_type_t) 0
                        }
                      );
                      if (can.is_occupied) {
                        break;
                      }
                    } else {
                      break;
                    }
                  } else {
                    break;
                  }
                }
              }
            }
            break;

          case QUEEN:
            for (int row_di = 0; row_di < 2; row_di++) {
              for (int col_di = 0; col_di < 2; col_di++) {
                for (int m = 1; m < 8; m++) {
                  coord_t coord = (coord_t) {
                    row + deltas[row_di] * m,
                    col + deltas[col_di] * m
                  };
                  if (is_valid_coord(coord)) {
                    cell_t can = game->position.cells[coord.row][coord.col];
                    if (!can.is_occupied || can.piece.color == opposite_color(color_to_move)) {
                      add_to_move_list(
                        move_list,
                        (move_t) {
                          cell.piece,
                          (coord_t) { row, col },
                          coord,
                          false,
                          false,
                          false,
                          (piece_type_t) 0
                        }
                      );
                      if (can.is_occupied) {
                        break;
                      }
                    } else {
                      break;
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
                  coord_t coord = (coord_t) {
                    row + deltas[di] * m * (is_row_delta == 1),
                    col + deltas[di] * m * (is_row_delta == 0)
                  };
                  if (is_valid_coord(coord)) {
                    cell_t can = game->position.cells[coord.row][coord.col];
                    if (!can.is_occupied || can.piece.color == opposite_color(color_to_move)) {
                      add_to_move_list(
                        move_list,
                        (move_t) {
                          cell.piece,
                          (coord_t) { row, col },
                          coord,
                          false,
                          false,
                          false,
                          (piece_type_t) 0
                        }
                      );
                      if (can.is_occupied) {
                        break;
                      }
                    } else {
                      break;
                    }
                  } else {
                    break;
                  }
                }
              }
            }
            break;

          case KING:
            for (int row_di = 0; row_di < 3; row_di++) {
              for (int col_di = 0; col_di < 3; col_di++) {
                coord_t coord = (coord_t) {
                  row + king_deltas[row_di],
                  col + king_deltas[col_di]
                };
                if ((king_deltas[row_di] != 0 || king_deltas[col_di] != 0) && is_valid_coord(coord)) {
                  cell_t can = game->position.cells[coord.row][coord.col];
                  if (!can.is_occupied || can.piece.color == opposite_color(color_to_move)) {
                    add_to_move_list(
                      move_list,
                      (move_t) {
                        cell.piece,
                        (coord_t) { row, col },
                        coord,
                        false,
                        false,
                        false,
                        (piece_type_t) 0
                      }
                    );
                  }
                }
              }
            }
            can_king_side_castle = true;
            can_queen_side_castle = true;
            for (int move_idx = 0; move_idx < game->moves_made.count; move_idx++) {
              move_t move = game->moves_made.entries[move_idx];
              if (move.piece.color == color_to_move) {
                if (move.is_king_side_castle || move.is_queen_side_castle || move.piece.type == KING) {
                  can_king_side_castle = false;
                  can_queen_side_castle = false;
                } else if (move.piece.type == ROOK && ((move.start.row == 0 && color_to_move == WHITE) || (move.start.row == 7 && color_to_move == BLACK))) {
                  if (move.start.col == 0) {
                    can_king_side_castle = false;
                  } else if (move.start.col = 7) {
                    can_queen_side_castle = false;
                  }
                }
                if (!can_king_side_castle && !can_queen_side_castle) {
                  break;
                }
              }
            }
            if (can_king_side_castle &&
                !game->position.cells[(color_to_move == WHITE) ? 0 : 7][1].is_occupied &&
                !game->position.cells[(color_to_move == WHITE) ? 0 : 7][2].is_occupied &&
                !is_cell_endangered_by_color(&game->position, (coord_t) { row, 2 }, opposite_color(color_to_move)) &&
                !is_cell_endangered_by_color(&game->position, (coord_t) { row, col }, opposite_color(color_to_move))) {
              add_to_move_list(
                move_list,
                (move_t) {
                  cell.piece,
                  (coord_t) { row, col },
                  (coord_t) { row, 1 },
                  false,
                  true,
                  false,
                  (piece_type_t) 0
                }
              );
            }
            if (can_queen_side_castle &&
                !game->position.cells[(color_to_move == WHITE) ? 0 : 7][4].is_occupied &&
                !game->position.cells[(color_to_move == WHITE) ? 0 : 7][5].is_occupied &&
                !game->position.cells[(color_to_move == WHITE) ? 0 : 7][6].is_occupied &&
                !is_cell_endangered_by_color(&game->position, (coord_t) { row, 4 }, opposite_color(color_to_move)) &&
                !is_cell_endangered_by_color(&game->position, (coord_t) { row, col }, opposite_color(color_to_move))) {
              add_to_move_list(
                move_list,
                (move_t) {
                  cell.piece,
                  (coord_t) { row, col },
                  (coord_t) { row, 5 },
                  false,
                  false,
                  true,
                  (piece_type_t) 0
                }
              );
            }
            break;
        }
      }
    }
  }
}
