#include "match.h"

static MatchState s_match_state;
static MatchState s_prev_state;
static bool s_has_history = false;

void match_init() { match_reset(); }

void match_reset() {
  s_match_state.p1_score = 0;
  s_match_state.p2_score = 0;
  s_match_state.p1_games = 0;
  s_match_state.p2_games = 0;
  s_match_state.p1_sets = 0;
  s_match_state.p2_sets = 0;
  s_match_state.server = 0;
  s_match_state.is_tiebreak = false;

  s_has_history = false;
}

MatchState *match_get_state() { return &s_match_state; }

void match_undo() {
  if (s_has_history) {
    s_match_state = s_prev_state;
    s_has_history = false; // Single level undo for now
  }
}

bool match_can_undo() { return s_has_history; }

static void handle_game_win(int player) {
  s_match_state.p1_score = 0;
  s_match_state.p2_score = 0;

  // Toggle server after every game (simplified)
  s_match_state.server = !s_match_state.server;

  if (player == 0) {
    s_match_state.p1_games++;
  } else {
    s_match_state.p2_games++;
  }

  // Set Logic (Standard 6 games)
  bool p1_wins_set =
      (s_match_state.p1_games >= 6 &&
       s_match_state.p1_games >= s_match_state.p2_games + 2) ||
      (s_match_state.p1_games == 7 &&
       s_match_state.p2_games == 6); // Tiebreak win (simplified for now)
  bool p2_wins_set =
      (s_match_state.p2_games >= 6 &&
       s_match_state.p2_games >= s_match_state.p1_games + 2) ||
      (s_match_state.p2_games == 7 && s_match_state.p1_games == 6);

  if (p1_wins_set) {
    s_match_state.p1_sets++;
    s_match_state.p1_games = 0;
    s_match_state.p2_games = 0;
  } else if (p2_wins_set) {
    s_match_state.p2_sets++;
    s_match_state.p1_games = 0;
    s_match_state.p2_games = 0;
  }
}

void match_add_point(int player) {
  // Save state for undo
  s_prev_state = s_match_state;
  s_has_history = true;

  int *scorer_score =
      (player == 0) ? &s_match_state.p1_score : &s_match_state.p2_score;
  int *opponent_score =
      (player == 0) ? &s_match_state.p2_score : &s_match_state.p1_score;

  // Standard Game Logic
  if (*scorer_score == 0) {
    *scorer_score = 15;
  } else if (*scorer_score == 15) {
    *scorer_score = 30;
  } else if (*scorer_score == 30) {
    *scorer_score = 40;
  } else if (*scorer_score == 40) {
    if (*opponent_score < 40) {
      // Game Win
      handle_game_win(player);
    } else if (*opponent_score == 40) {
      // Deuce -> Ad
      *scorer_score = 50; // Use 50 for Ad
    } else if (*opponent_score == 50) {
      // Opponent had Ad -> Deuce
      *opponent_score = 40;
    }
  } else if (*scorer_score == 50) {
    // Ad -> Game Win
    handle_game_win(player);
  }
}
