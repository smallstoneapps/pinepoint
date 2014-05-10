/**
 * PinePoint Pebble Watchface App
 *
 * Written by Matthew Tole <pebble@matthewtole.com>
 * Started 4th July 2013
 * Last modified 10th May 2014
 *
 * Developed for Rush Hambleton at Pine Point School
 * Original proposal document: http://dft.ba/-pinepointproposal
 *
 * GitHub Repository: https://github.com/matthewtole/pebble-pinepoint/
*/

#include <pebble.h>

#include "libs/pebble-assist/pebble-assist.h"
#include "libs/clock-layer.h"

// Commented out because inverted colours is gross.
#define INVERT_COLORS false
#if INVERT_COLORS
#define COLOR_FOREGROUND GColorWhite
#define COLOR_BACKGROUND GColorBlack
#else
#define COLOR_FOREGROUND GColorBlack
#define COLOR_BACKGROUND GColorWhite
#endif

#define WIDTH 24
#define HEIGHT 12
#define X_OFFSET 4
#define Y_OFFSET 4
#define COLUMN_COUNT 5
#define ROW_COUNT 8
#define CORNER_RADIUS 2

#define VIBE_LONG 1500
#define VIBE_SHORT 500
#define VIBE_TINY 300
#define VIBE_HUGE 3000
#define VIBE_PAUSE 500

#define CLASS_DURATION 40

static struct tm get_current_time();
static int get_minutes_left(struct tm* current_time);
static void vibrate_segments(uint32_t const segments[], int segment_count);
static void do_vibrate(int minutes);
static void grid_layer_update_callback(Layer* me, GContext* ctx);
static void handle_init(void);
static void handle_deinit(void);

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

static Window* window;
static ClockLayer* layer_time;
static Layer* layer_grid;

/**
 * Pebble app entry point
 */
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

static struct tm get_current_time() {
  time_t tmp = time(NULL);
  struct tm* now = localtime(&tmp);
  return *now;
}

/**
 * Returns minutes left in class
 * @param The current time
 * @return Minutes left before end of the class
 */
int get_minutes_left(struct tm* current_time) {

  int minutes_since_midnight = (current_time->tm_hour * 60) + current_time->tm_min;

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
void vibrate_segments(uint32_t const segments[], int segment_count) {

  VibePattern pattern = {
    .durations = segments,
    .num_segments = segment_count,
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
      vibrate_segments(segments, ARRAY_LENGTH(segments));
    }
    break;

    case 15: {
      uint32_t const segments[] = { VIBE_LONG, VIBE_PAUSE, VIBE_SHORT };
      vibrate_segments(segments, ARRAY_LENGTH(segments));
    }
    break;

    case 10: {
      uint32_t const segments[] = { VIBE_LONG };
      vibrate_segments(segments, ARRAY_LENGTH(segments));
    }
    break;

    case 5: {
      uint32_t const segments[] = { VIBE_SHORT };
      vibrate_segments(segments, ARRAY_LENGTH(segments));
    }
    break;

    case 2: {
      uint32_t const segments[] = { VIBE_TINY, VIBE_TINY, VIBE_TINY };
      vibrate_segments(segments, ARRAY_LENGTH(segments));
    }
    break;

    case 0: {
      uint32_t const segments[] = { VIBE_HUGE };
      vibrate_segments(segments, ARRAY_LENGTH(segments));
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

  struct tm the_time = get_current_time();
  int minutes_left = get_minutes_left(&the_time);

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
 * Minute change handler.
 * Called every time the minute value of the clock changes.
 * @param The app context reference.
 * @param Pointer to the Pebble event.
 */
void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {

  struct tm the_time   = get_current_time();
  int minutes_left = get_minutes_left(&the_time);

  if (the_time.tm_sec == 0) {
    do_vibrate(minutes_left);
  }

  // Update the clock layer
  clock_layer_set_time(layer_time, &the_time);
  // Mark the grid layer as dirty so it redraws.
  layer_mark_dirty(layer_grid);
}

/**
 * Init event handler
 * Called whenever the watch app is loaded.
 * @param The app context reference
 */
void handle_init() {

  // Init a Pebble window.
  window = window_create();
  window_set_background_color(window, COLOR_BACKGROUND);

  // Create the layer for the grid of blocks.
  layer_grid = layer_create_fullscreen(window);
  layer_set_update_proc(layer_grid, grid_layer_update_callback);
  layer_add_to_window(layer_grid, window);

  // Create the layer for the time text.
  layer_time = clock_layer_create(GRect(0, 128, 144, 36));
  clock_layer_add_to_window(layer_time, window);
  clock_layer_set_text_color(layer_time, COLOR_FOREGROUND);
  clock_layer_set_background_color(layer_time, GColorClear);
  clock_layer_set_font(layer_time, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  clock_layer_set_text_alignment(layer_time, GTextAlignmentCenter);
  clock_layer_set_time_format(layer_time, "%l:%M %p");
  clock_layer_update(layer_time);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  window_stack_push(window, true);

}

void handle_deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(window);
  layer_destroy(layer_grid);
  clock_layer_destroy(layer_time);
}
