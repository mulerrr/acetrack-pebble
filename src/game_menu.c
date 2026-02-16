#include "game_menu.h"
#include "main.h"
#include "match.h"
#include <pebble.h>

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[1];
static SimpleMenuItem s_menu_items[2];
static Window *s_menu_window;

static void menu_select_callback(int index, void *ctx) {
  if (index == 0) {
    // Undo
    if (match_can_undo()) {
      match_undo();
      main_window_update_ui();
      window_stack_pop(true); // Close menu
    }
  } else if (index == 1) {
    // End Game
    window_stack_pop(true); // Close menu
    window_stack_pop(true); // Close game window (return to mode select)
  }
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_items[0] = (SimpleMenuItem){
      .title = "Undo",
      .subtitle = "Revert last point",
      .callback = menu_select_callback,
  };

  s_menu_items[1] = (SimpleMenuItem){
      .title = "End Game",
      .callback = menu_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection){
      .num_items = 2,
      .items = s_menu_items,
  };

  s_simple_menu_layer =
      simple_menu_layer_create(bounds, window, s_menu_sections, 1, NULL);
  layer_add_child(window_layer,
                  simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void menu_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
  window_destroy(window);
  s_menu_window = NULL;
}

void game_menu_show() {
  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers){
                                                .load = menu_window_load,
                                                .unload = menu_window_unload,
                                            });
  window_stack_push(s_menu_window, true);
}
