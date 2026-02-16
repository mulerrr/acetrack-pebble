#pragma once

#include <pebble.h>

typedef struct {
  int p1_score; // 0, 15, 30, 40, 50 (Ad)
  int p2_score;
  int p1_games;
  int p2_games;
  int p1_sets;
  int p2_sets;
  int server; // 0 for Player 1, 1 for Player 2
  bool is_tiebreak;
} MatchState;

void match_init();
void match_add_point(int player); // 0 for P1, 1 for P2
MatchState *match_get_state();
void match_reset();
void match_undo();
bool match_can_undo();
