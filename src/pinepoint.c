#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0x66, 0x78, 0xBA, 0xA6, 0x46, 0x8D, 0x45, 0x23, 0x98, 0x58, 0x29, 0x2F, 0xAE, 0x52, 0x72, 0x40 }

PBL_APP_INFO(MY_UUID, "Pine Point", "Matthew Tole", 1, 0,  DEFAULT_MENU_ICON, APP_INFO_WATCH_FACE);


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

#define CLASS_DURATION 40
#define CLASS_COUNT 3

const int classEndings[] = { 572, 623, 678 };

Window window;
TextLayer timeLayer;
Layer gridLayer;

int minutesToEndOfClass(PblTm currentTime) {

  int minutesSinceMidnight = (currentTime.tm_hour * 60) + currentTime.tm_min;

  for (int c = 0; c < CLASS_COUNT; c += 1) {
    if (minutesSinceMidnight <= classEndings[c]) {
      return classEndings[c] - minutesSinceMidnight;
    }
  }

  return (1440 + classEndings[0]) - minutesSinceMidnight;

}

void gridLayer_update_callback(Layer *me, GContext* ctx) {

   graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);

   PblTm currentTime;
   get_time(&currentTime);
   currentTime.tm_hour = 11;
   currentTime.tm_min = 11;
   int cellsToShow = minutesToEndOfClass(currentTime);

   for (int x = 0; x < COLUMN_COUNT; x += 1) {
    for (int y = 0; y < ROW_COUNT; y += 1) {
      int cellNumber = 40 - ((y * COLUMN_COUNT) + x);
      if (cellsToShow  < cellNumber) {
        continue;
      }
      GRect cell;
      cell.origin = GPoint(X_OFFSET + (x * (WIDTH + X_OFFSET)), Y_OFFSET + (y * (HEIGHT + Y_OFFSET)));
      cell.size = GSize(WIDTH, HEIGHT);
      graphics_fill_rect(ctx, cell, CORNER_RADIUS, GCornersAll);
    }
   }

}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

  PblTm currentTime;
  get_time(&currentTime);

  layer_mark_dirty(&gridLayer);

  vibes_double_pulse();

  static char timeText[] = "00:00 AM";
  string_format_time(timeText, sizeof(timeText), "%H:%M %p", &currentTime);
  text_layer_set_text(&timeLayer, timeText);
}

void handle_init(AppContextRef ctx) {

  window_init(&window, "Pine Point");
  window_stack_push(&window, true);
  window_set_background_color(&window, COLOR_BACKGROUND);

  layer_init(&gridLayer, window.layer.frame);
  gridLayer.update_proc = &gridLayer_update_callback;
  layer_add_child(&window.layer, &gridLayer);

  text_layer_init(&timeLayer, GRect(0, 128, 144, 36));
  text_layer_set_text_color(&timeLayer, COLOR_FOREGROUND);
  text_layer_set_background_color(&timeLayer, GColorClear);
  text_layer_set_font(&timeLayer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(&timeLayer, GTextAlignmentCenter);

  handle_minute_tick(ctx, NULL);

  layer_add_child(&window.layer, &timeLayer.layer);
}


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
