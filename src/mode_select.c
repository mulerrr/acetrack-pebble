#include "mode_select.h"
#include "main.h"
#include <pebble.h>

static Window *s_mode_window;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[1];
static SimpleMenuItem s_menu_items[2];

static void menu_select_callback(int index, void *ctx) {
  // Index 0: Remote, Index 1: Standalone
  bool is_standalone = (index == 1);
  game_window_push(is_standalone);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_items[0] = (SimpleMenuItem){
      .title = "Remote Mode",
      .callback = menu_select_callback,
  };
  s_menu_items[1] = (SimpleMenuItem){
      .title = "Standalone Mode",
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

static void main_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
}

void mode_select_init() {
  s_mode_window = window_create();
  window_set_window_handlers(s_mode_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_mode_window, true);
}

void mode_select_deinit() { window_destroy(s_mode_window); }
