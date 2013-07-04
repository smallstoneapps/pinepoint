/**
 * PinePoint Pebble Watchface App
 *
 * Written by Matthew Tole <pebble@matthewtole.com>
 * Started 4th July 2013
 *
 * Developed for Rush Hamilton at Pine Point School
 * Original proposal document: http://dft.ba/-pinepointproposal
 *
 * GitHub Repository: https://github.com/matthewtole/pebble-pinepoint/
*/

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0x66, 0x78, 0xBA, 0xA6, 0x46, 0x8D, 0x45, 0x23, 0x98, 0x58, 0x29, 0x2F, 0xAE, 0x52, 0x72, 0x40 }

PBL_APP_INFO(MY_UUID, "Pine Point", "Matthew Tole", 1, 0,  DEFAULT_MENU_ICON, APP_INFO_WATCH_FACE);

// Commented out because inverted colours is gross.
// #define INVERT_COLORS
#ifndef INVERT_COLORS
#define COLOR_FOREGROUND GColorBlack
#define COLOR_BACKGROUND GColorWhite
#else
#define COLOR_FOREGROUND GColorWhite
#define COLOR_BACKGROUND GColorBlack
#endif

#define WIDTH 24
#define HEIGHT 12
#define X_OFFSET 4
#define Y_OFFSET 4
#define COLUMN_COUNT 5
#define ROW_COUNT 8
#define CORNER_RADIUS 2

#define VIBE_LONG 3000
#define VIBE_SHORT 1000
#define VIBE_TINY 300
#define VIBE_HUGE 5000
#define VIBE_PAUSE 500
#define VIBE_PAUSE_TINY 300

#define CLASS_DURATION 40

const int CLASS_ENDINGS[] = {
  572, // 09:32
  623, // 10:23
  678, // 11:18
  725, // 12:05
  812, // 13:32
  859, // 14:19
  905  // 15:05
};

const int CLASS_COUNT = ARRAY_LENGTH(CLASS_ENDINGS);

Window    g_window;
TextLayer g_layer_time;
Layer     g_layer_grid;

/**
 * Get the "current" time.
 * Returns the current time, although the time can be manipulated for testing.
 * @return The current time, or a fake time.
 */
PblTm get_current_time() {

  PblTm current_time;
  get_time(&current_time);

  // TESTING CODE
  current_time.tm_hour += 13;
  // END TESTING CODE

  return current_time;

}

/**
 * Returns minutes left in class
 * @param The current time
 * @return Minutes left before end of the class
 */
int get_minutes_left(PblTm current_time) {

  int minutes_since_midnight = (current_time.tm_hour * 60) + current_time.tm_min;

  // Go through the class times and find the first one that hasn't finished
  // already, and return the number of minutes until the end of that class.
  for (int c = 0; c < CLASS_COUNT; c += 1) {
    if (minutes_since_midnight <= CLASS_ENDINGS[c]) {
      return CLASS_ENDINGS[c] - minutes_since_midnight;
    }
  }

  // Minutes until the first class of the day.
  return (1440 + CLASS_ENDINGS[0]) - minutes_since_midnight;

}

/**
 * Perform a given vibration sequence.
 * @param Array of segments lengths.
 */
void vibrate_segments(uint32_t const segments[]) {

  VibePattern pattern = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pattern);

}

/**
 * Vibrate the watch based on the time left.
 * @param Minutes left before end of the class
 */
void do_vibrate(int minutes) {

  switch (minutes) {
    case 20: {
      uint32_t const segments[] = { VIBE_LONG, VIBE_PAUSE, VIBE_LONG };
      vibrate_segments(segments);
    }
    break;

    case 15: {
      uint32_t const segments[] = { VIBE_LONG, VIBE_PAUSE, VIBE_SHORT };
      vibrate_segments(segments);
    }
    break;

    case 10: {
      uint32_t const segments[] = { VIBE_LONG };
      vibrate_segments(segments);
    }
    break;

    case 5: {
      uint32_t const segments[] = { VIBE_SHORT };
      vibrate_segments(segments);
    }
    break;

    case 2: {
      uint32_t const segments[] = { VIBE_TINY, VIBE_PAUSE, VIBE_TINY };
      vibrate_segments(segments);
    }
    break;

    case 0: {
      uint32_t const segments[] = { VIBE_HUGE };
      vibrate_segments(segments);
    }
    break;

    default:
      return;
  }

}

/**
 * Update callback function for the grid layer.
 * @param Pointer to the layer to be upddated
 * @param Pointer to the current context.
 */
void grid_layer_update_callback(Layer* me, GContext* ctx) {

   graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);

   PblTm the_time = get_current_time();
   int minutes_left = get_minutes_left(the_time);

   for (int x = 0; x < COLUMN_COUNT; x += 1) {

    for (int y = 0; y < ROW_COUNT; y += 1) {

      // The cells are number from 40 to 1, top left to bottom right.
      int cell_number = 40 - ((y * COLUMN_COUNT) + x);
      // If the number is greater than the minutes left, don't draw it.
      if (minutes_left < cell_number) {
        continue;
      }

      // Construct and draw the block.
      GRect cell;
      cell.origin = GPoint(
        X_OFFSET + (x * (WIDTH + X_OFFSET)),
        Y_OFFSET + (y * (HEIGHT + Y_OFFSET))
      );
      cell.size = GSize(WIDTH, HEIGHT);
      graphics_fill_rect(ctx, cell, CORNER_RADIUS, GCornersAll);
    }

   }

}

/**
 * Main drawing function
 * Called every minute and whenever the watch app loads.
 * @param The "current" time.
 */
void draw(PblTm current_time) {

  // Format the current time into a nice string and display.
  static char time_text[] = "00:00 AM";
  string_format_time(time_text, sizeof(time_text), "%H:%M %p", &current_time);
  text_layer_set_text(&g_layer_time, time_text);

  // Mark the grid layer as dirty so it redraws.
  layer_mark_dirty(&g_layer_grid);

}

/**
 * Minute change handler.
 * Called every time the minute value of the clock changes.
 * @param The app context reference.
 * @param Pointer to the Pebble event.
 */
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

  PblTm the_time   = get_current_time();
  int minutes_left = get_minutes_left(the_time);

  do_vibrate(minutes_left);
  draw(the_time);

}

/**
 * Init event handler
 * Called whenever the watch app is loaded.
 * @param The app context reference
 */
void handle_init(AppContextRef ctx) {

  // Init a Pebble window.
  window_init(&g_window, "Pine Point");
  window_stack_push(&g_window, true);
  window_set_background_color(&g_window, COLOR_BACKGROUND);

  // Create the layer for the grid of blocks.
  layer_init(&g_layer_grid, g_window.layer.frame);
  g_layer_grid.update_proc = &grid_layer_update_callback;
  layer_add_child(&g_window.layer, &g_layer_grid);

  // Create the later for the time text.
  text_layer_init(&g_layer_time, GRect(0, 128, 144, 36));
  text_layer_set_text_color(&g_layer_time, COLOR_FOREGROUND);
  text_layer_set_background_color(&g_layer_time, GColorClear);
  text_layer_set_font(&g_layer_time, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(&g_layer_time, GTextAlignmentCenter);
  layer_add_child(&g_window.layer, &g_layer_time.layer);

  // Draw the screen.
  draw(get_current_time());
}

/**
 * Pebble app entry point
 */
void pbl_main(void *params) {

  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };

  app_event_loop(params, &handlers);

}
