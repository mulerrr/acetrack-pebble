#include "game_menu.h"
#include "match.h"
#include "mode_select.h"
#include <pebble.h>

// Message Keys (matching package.json)
#define KEY_ACTION 0
#define KEY_SCORE_P1 1
#define KEY_SCORE_P2 2
#define KEY_GAMES_P1 3
#define KEY_GAMES_P2 4
#define KEY_SETS_P1 5
#define KEY_SETS_P2 6
#define KEY_SERVER 7
#define KEY_PLAYER1_NAME 8
#define KEY_PLAYER2_NAME 9

// UI Elements
static Window *s_main_window;
static TextLayer *s_score_p1_layer;
static TextLayer *s_score_p2_layer;
static TextLayer *s_games_p1_layer;
static TextLayer *s_games_p2_layer;
static TextLayer *s_sets_p1_layer;
static TextLayer *s_sets_p2_layer;
static TextLayer *s_name_p1_layer;
static TextLayer *s_name_p2_layer;
// Server layers removed, merged into name
static TextLayer *s_status_layer;
static TextLayer *s_time_layer;

// Buffers
static char s_p1_score_buffer[8];
static char s_p2_score_buffer[8];
static char s_p1_games_buffer[8];
static char s_p2_games_buffer[8];
static char s_p1_sets_buffer[8];
static char s_p2_sets_buffer[8];
static char s_p1_name_buffer[40]; // Increased to hold ": Serve"
static char s_p2_name_buffer[40]; // Increased to hold ": Serve"
static char s_time_buffer[8];

// Source Names
static char s_p1_name_source[32] = "P1";
static char s_p2_name_source[32] = "P2";

// State
static bool s_is_standalone = false;
static int s_remote_server = 0; // 0=P1, 1=P2 (for Remote Mode)

// --- Time Handling ---

static void update_time() {
  if (!s_main_window)
    return; // Guard against NULL window

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// --- Display Helpers ---

static void update_name_text() {
  if (!s_main_window)
    return;

  int server = s_is_standalone ? match_get_state()->server : s_remote_server;

  snprintf(s_p1_name_buffer, sizeof(s_p1_name_buffer), "%s%s", s_p1_name_source,
           (server == 0) ? " : Serve" : "");
  text_layer_set_text(s_name_p1_layer, s_p1_name_buffer);

  snprintf(s_p2_name_buffer, sizeof(s_p2_name_buffer), "%s%s", s_p2_name_source,
           (server == 1) ? " : Serve" : "");
  text_layer_set_text(s_name_p2_layer, s_p2_name_buffer);
}

static void update_ui_from_state() {
  if (!s_main_window)
    return; // Guard

  MatchState *state = match_get_state();

  // Score
  snprintf(s_p1_score_buffer, sizeof(s_p1_score_buffer), "%d", state->p1_score);
  if (state->p1_score == 50)
    snprintf(s_p1_score_buffer, sizeof(s_p1_score_buffer), "Ad");
  text_layer_set_text(s_score_p1_layer, s_p1_score_buffer);

  snprintf(s_p2_score_buffer, sizeof(s_p2_score_buffer), "%d", state->p2_score);
  if (state->p2_score == 50)
    snprintf(s_p2_score_buffer, sizeof(s_p2_score_buffer), "Ad");
  text_layer_set_text(s_score_p2_layer, s_p2_score_buffer);

  // Games
  snprintf(s_p1_games_buffer, sizeof(s_p1_games_buffer), "G:%d",
           state->p1_games);
  text_layer_set_text(s_games_p1_layer, s_p1_games_buffer);

  snprintf(s_p2_games_buffer, sizeof(s_p2_games_buffer), "G:%d",
           state->p2_games);
  text_layer_set_text(s_games_p2_layer, s_p2_games_buffer);

  // Sets
  snprintf(s_p1_sets_buffer, sizeof(s_p1_sets_buffer), "S:%d", state->p1_sets);
  text_layer_set_text(s_sets_p1_layer, s_p1_sets_buffer);

  snprintf(s_p2_sets_buffer, sizeof(s_p2_sets_buffer), "S:%d", state->p2_sets);
  text_layer_set_text(s_sets_p2_layer, s_p2_sets_buffer);

  // Names & Server
  update_name_text();
}

void main_window_update_ui() { update_ui_from_state(); }

// --- AppMessage Helpers ---

static void send_action(int action_code) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int32(iter, KEY_ACTION, action_code);
  app_message_outbox_send();
}

static void update_score_display(int key, Tuple *t) {
  if (!s_main_window)
    return; // Guard

  switch (key) {
  case KEY_SCORE_P1:
    snprintf(s_p1_score_buffer, sizeof(s_p1_score_buffer), "%d",
             (int)t->value->uint32);
    if (t->value->uint32 == 50)
      snprintf(s_p1_score_buffer, sizeof(s_p1_score_buffer), "Ad");
    text_layer_set_text(s_score_p1_layer, s_p1_score_buffer);
    break;
  case KEY_SCORE_P2:
    snprintf(s_p2_score_buffer, sizeof(s_p2_score_buffer), "%d",
             (int)t->value->uint32);
    if (t->value->uint32 == 50)
      snprintf(s_p2_score_buffer, sizeof(s_p2_score_buffer), "Ad");
    text_layer_set_text(s_score_p2_layer, s_p2_score_buffer);
    break;
  case KEY_GAMES_P1:
    snprintf(s_p1_games_buffer, sizeof(s_p1_games_buffer), "G:%d",
             (int)t->value->uint32);
    text_layer_set_text(s_games_p1_layer, s_p1_games_buffer);
    break;
  case KEY_GAMES_P2:
    snprintf(s_p2_games_buffer, sizeof(s_p2_games_buffer), "G:%d",
             (int)t->value->uint32);
    text_layer_set_text(s_games_p2_layer, s_p2_games_buffer);
    break;
  case KEY_SETS_P1:
    snprintf(s_p1_sets_buffer, sizeof(s_p1_sets_buffer), "S:%d",
             (int)t->value->uint32);
    text_layer_set_text(s_sets_p1_layer, s_p1_sets_buffer);
    break;
  case KEY_SETS_P2:
    snprintf(s_p2_sets_buffer, sizeof(s_p2_sets_buffer), "S:%d",
             (int)t->value->uint32);
    text_layer_set_text(s_sets_p2_layer, s_p2_sets_buffer);
    break;
  case KEY_PLAYER1_NAME:
    snprintf(s_p1_name_source, sizeof(s_p1_name_source), "%s",
             t->value->cstring);
    update_name_text();
    break;
  case KEY_PLAYER2_NAME:
    snprintf(s_p2_name_source, sizeof(s_p2_name_source), "%s",
             t->value->cstring);
    update_name_text();
    break;
  case KEY_SERVER:
    // 0 = P1, 1 = P2
    s_remote_server = (int)t->value->uint32;
    update_name_text();
    break;
  }
}

static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
  // Only process if main window is loaded (game is active)
  if (!s_main_window)
    return;

  // Ignore remote updates in Standalone Mode
  if (s_is_standalone)
    return;

  Tuple *t = dict_read_first(iterator);
  while (t != NULL) {
    update_score_display(t->key, t);
    t = dict_read_next(iterator);
  }
}

// --- Button Handlers ---

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_standalone) {
    match_add_point(0); // P1
    update_ui_from_state();
  } else {
    send_action(0); // P1 Point
  }
  vibes_short_pulse();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_standalone) {
    match_add_point(1); // P2
    update_ui_from_state();
  } else {
    send_action(1); // P2 Point
  }
  vibes_short_pulse();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_standalone) {
    game_menu_show();
  } else {
    send_action(2); // Undo
  }
  vibes_short_pulse();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// --- Window Lifecycle ---

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Status/Header (Title)
  s_status_layer =
      text_layer_create(GRect(2, -2, 100, 18)); // Moved up slightly
  if (s_is_standalone) {
    text_layer_set_text(s_status_layer, "Standalone");
  } else {
    text_layer_set_text(s_status_layer, "AT Remote");
  }
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentLeft);
  text_layer_set_font(s_status_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  // Time
  s_time_layer = text_layer_create(GRect(bounds.size.w - 42, -2, 40, 18));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // --- Player 1 Section ---
  // Row 1: Games | Name | Sets (Y = 16)

  // Games P1 (Left)
  s_games_p1_layer = text_layer_create(GRect(2, 16, 30, 20));
  text_layer_set_text(s_games_p1_layer, "G:0");
  text_layer_set_font(s_games_p1_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_games_p1_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_games_p1_layer));

  // Sets P1 (Right)
  s_sets_p1_layer = text_layer_create(GRect(112, 16, 30, 20));
  text_layer_set_text(s_sets_p1_layer, "S:0");
  text_layer_set_font(s_sets_p1_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_sets_p1_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_sets_p1_layer));

  // Name P1 (Center)
  s_name_p1_layer = text_layer_create(GRect(32, 16, 80, 20));
  text_layer_set_font(s_name_p1_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_name_p1_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_name_p1_layer));

  // Score P1 (Y = 36)
  s_score_p1_layer = text_layer_create(GRect(0, 36, bounds.size.w, 50));
  text_layer_set_font(s_score_p1_layer,
                      fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_score_p1_layer, GTextAlignmentCenter);
  text_layer_set_text(s_score_p1_layer, "0");
  layer_add_child(window_layer, text_layer_get_layer(s_score_p1_layer));

  // Divider
  Layer *divider = layer_create(GRect(10, 85, bounds.size.w - 20, 2));
  layer_add_child(window_layer, divider);

  // --- Player 2 Section ---

  // Score P2 (Y = 88)
  s_score_p2_layer = text_layer_create(GRect(0, 88, bounds.size.w, 50));
  text_layer_set_font(s_score_p2_layer,
                      fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_score_p2_layer, GTextAlignmentCenter);
  text_layer_set_text(s_score_p2_layer, "0");
  layer_add_child(window_layer, text_layer_get_layer(s_score_p2_layer));

  // Row 2: Games | Name | Sets (Y = 138)

  // Games P2 (Left)
  s_games_p2_layer = text_layer_create(GRect(2, 138, 30, 20));
  text_layer_set_text(s_games_p2_layer, "G:0");
  text_layer_set_font(s_games_p2_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_games_p2_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_games_p2_layer));

  // Sets P2 (Right)
  s_sets_p2_layer = text_layer_create(GRect(112, 138, 30, 20));
  text_layer_set_text(s_sets_p2_layer, "S:0");
  text_layer_set_font(s_sets_p2_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_sets_p2_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_sets_p2_layer));

  // Name P2 (Center)
  s_name_p2_layer = text_layer_create(GRect(32, 138, 80, 20));
  text_layer_set_font(s_name_p2_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_name_p2_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_name_p2_layer));

  // Initial Time
  update_time();

  // Set initial names
  update_name_text();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Initial State if Standalone
  if (s_is_standalone) {
    update_ui_from_state();
  }
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();

  text_layer_destroy(s_score_p1_layer);
  text_layer_destroy(s_score_p2_layer);
  text_layer_destroy(s_games_p1_layer);
  text_layer_destroy(s_games_p2_layer);
  text_layer_destroy(s_sets_p1_layer);
  text_layer_destroy(s_sets_p2_layer);
  text_layer_destroy(s_name_p1_layer);
  text_layer_destroy(s_name_p2_layer);
  text_layer_destroy(s_status_layer);
  text_layer_destroy(s_time_layer);
}

// --- Initialization ---

void game_window_push(bool is_standalone) {
  s_is_standalone = is_standalone;
  if (s_is_standalone) {
    match_init();
  }

  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);
}

static void init() {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(64, 64);

  // Launch Mode Select
  mode_select_init();
}

static void deinit() { mode_select_deinit(); }

int main(void) {
  init();
  app_event_loop();
  deinit();
}
