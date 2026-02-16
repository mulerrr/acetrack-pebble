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
static TextLayer *s_server_p1_layer;
static TextLayer *s_server_p2_layer;
static TextLayer *s_status_layer;

// Buffers
static char s_p1_score_buffer[8];
static char s_p2_score_buffer[8];
static char s_p1_games_buffer[8];
static char s_p2_games_buffer[8];
static char s_p1_sets_buffer[8];
static char s_p2_sets_buffer[8];
static char s_p1_name_buffer[20];
static char s_p2_name_buffer[20];

// --- AppMessage Helpers ---

static void send_action(int action_code) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int32(iter, KEY_ACTION, action_code);
  app_message_outbox_send();
}

static void update_score_display(int key, Tuple *t) {
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
    snprintf(s_p1_name_buffer, sizeof(s_p1_name_buffer), "%s",
             t->value->cstring);
    text_layer_set_text(s_name_p1_layer, s_p1_name_buffer);
    break;
  case KEY_PLAYER2_NAME:
    snprintf(s_p2_name_buffer, sizeof(s_p2_name_buffer), "%s",
             t->value->cstring);
    text_layer_set_text(s_name_p2_layer, s_p2_name_buffer);
    break;
  case KEY_SERVER:
    // 0 = P1, 1 = P2
    if (t->value->uint32 == 0) {
      layer_set_hidden(text_layer_get_layer(s_server_p1_layer), false);
      layer_set_hidden(text_layer_get_layer(s_server_p2_layer), true);
    } else {
      layer_set_hidden(text_layer_get_layer(s_server_p1_layer), true);
      layer_set_hidden(text_layer_get_layer(s_server_p2_layer), false);
    }
    break;
  }
}

static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
  Tuple *t = dict_read_first(iterator);
  while (t != NULL) {
    update_score_display(t->key, t);
    t = dict_read_next(iterator);
  }
}

// --- Button Handlers ---

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  send_action(0); // P1 Point
  vibes_short_pulse();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  send_action(1); // P2 Point
  vibes_short_pulse();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  send_action(2); // Undo
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

  // Status/Header
  s_status_layer = text_layer_create(GRect(0, 0, bounds.size.w, 16));
  text_layer_set_text(s_status_layer, "AceTrack Remote");
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_font(s_status_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  // --- Player 1 Section ---
  // Name
  s_name_p1_layer = text_layer_create(GRect(5, 18, bounds.size.w - 10, 20));
  text_layer_set_text(s_name_p1_layer, "Player 1");
  text_layer_set_font(s_name_p1_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_name_p1_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_name_p1_layer));

  // Server Indicator (P1)
  s_server_p1_layer = text_layer_create(GRect(bounds.size.w - 20, 18, 15, 20));
  text_layer_set_text(s_server_p1_layer, "*");
  text_layer_set_font(s_server_p1_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_set_hidden(text_layer_get_layer(s_server_p1_layer), false); // Default
  layer_add_child(window_layer, text_layer_get_layer(s_server_p1_layer));

  // Score
  s_score_p1_layer = text_layer_create(GRect(0, 35, bounds.size.w, 38));
  text_layer_set_font(s_score_p1_layer,
                      fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_score_p1_layer, GTextAlignmentCenter);
  text_layer_set_text(s_score_p1_layer, "0");
  layer_add_child(window_layer, text_layer_get_layer(s_score_p1_layer));

  // Games/Sets
  s_games_p1_layer = text_layer_create(GRect(5, 70, 60, 20));
  text_layer_set_text(s_games_p1_layer, "G:0");
  layer_add_child(window_layer, text_layer_get_layer(s_games_p1_layer));

  s_sets_p1_layer = text_layer_create(GRect(70, 70, 60, 20));
  text_layer_set_text(s_sets_p1_layer, "S:0");
  layer_add_child(window_layer, text_layer_get_layer(s_sets_p1_layer));

  // Divider
  Layer *divider = layer_create(GRect(10, 88, bounds.size.w - 20, 2));
  layer_add_child(window_layer, divider);

  // --- Player 2 Section ---
  // Name
  s_name_p2_layer = text_layer_create(GRect(5, 92, bounds.size.w - 10, 20));
  text_layer_set_text(s_name_p2_layer, "Player 2");
  text_layer_set_font(s_name_p2_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_name_p2_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_name_p2_layer));

  // Server Indicator (P2)
  s_server_p2_layer = text_layer_create(GRect(bounds.size.w - 20, 92, 15, 20));
  text_layer_set_text(s_server_p2_layer, "*");
  text_layer_set_font(s_server_p2_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_set_hidden(text_layer_get_layer(s_server_p2_layer),
                   true); // Default hidden
  layer_add_child(window_layer, text_layer_get_layer(s_server_p2_layer));

  // Score
  s_score_p2_layer = text_layer_create(GRect(0, 110, bounds.size.w, 38));
  text_layer_set_font(s_score_p2_layer,
                      fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_score_p2_layer, GTextAlignmentCenter);
  text_layer_set_text(s_score_p2_layer, "0");
  layer_add_child(window_layer, text_layer_get_layer(s_score_p2_layer));

  s_games_p2_layer = text_layer_create(GRect(5, 145, 60, 20));
  text_layer_set_text(s_games_p2_layer, "G:0");
  layer_add_child(window_layer, text_layer_get_layer(s_games_p2_layer));

  s_sets_p2_layer = text_layer_create(GRect(70, 145, 60, 20));
  text_layer_set_text(s_sets_p2_layer, "S:0");
  layer_add_child(window_layer, text_layer_get_layer(s_sets_p2_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_score_p1_layer);
  text_layer_destroy(s_score_p2_layer);
  text_layer_destroy(s_games_p1_layer);
  text_layer_destroy(s_games_p2_layer);
  text_layer_destroy(s_sets_p1_layer);
  text_layer_destroy(s_sets_p2_layer);
  text_layer_destroy(s_name_p1_layer);
  text_layer_destroy(s_name_p2_layer);
  text_layer_destroy(s_server_p1_layer);
  text_layer_destroy(s_server_p2_layer);
  text_layer_destroy(s_status_layer);
}

// --- Initialization ---

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(app_message_inbox_size_maximum(),
                   app_message_outbox_size_maximum());
}

static void deinit() { window_destroy(s_main_window); }

int main(void) {
  init();
  app_event_loop();
  deinit();
}
