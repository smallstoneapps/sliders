/*
 * Sliders
 * Copyright (C) 2013 Matthew Tole
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "config.h"

#if ROCKSHOT
#include "http.h"
#include "httpcapture.h"
#endif

#define MY_UUID { 0x64, 0x6A, 0xA5, 0xC6, 0x74, 0xA3, 0x46, 0xC8, 0xBA, 0x44, 0xD9, 0x18, 0xA3, 0x02, 0xAC, 0x77 }

PBL_APP_INFO(MY_UUID, "Sliders", "Matthew Tole", 1, 0, RESOURCE_ID_MENU_ICON, APP_INFO_WATCH_FACE);

#if INVERT_COLORS
#define COLOR_FOREGROUND GColorWhite
#define COLOR_BACKGROUND GColorBlack
#else
#define COLOR_FOREGROUND GColorBlack
#define COLOR_BACKGROUND GColorWhite
#endif

#define PADDING 12
#define HEIGHT 40
#define FONT_SIZE 34
#define FONT_SIZE_SMALL 22

Window window;
Layer layer_hours;
Layer layer_minutes;
Layer layer_seconds;
GFont font_normal;
GFont font_bold;

void handle_init(AppContextRef ctx);
void hours_layer_update_callback(Layer* me, GContext* ctx);
void minutes_layer_update_callback(Layer* me, GContext* ctx);
void seconds_layer_update_callback(Layer* me, GContext* ctx);
void handle_tick(AppContextRef ctx, PebbleTickEvent *t);
PblTm get_now();

#if ROCKSHOT
void  http_success(int32_t cookie, int http_status, DictionaryIterator *dict, void *ctx);
#endif

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_tick,
      .tick_units = SECOND_UNIT
    }
  };

  #if ROCKSHOT
  handlers.messaging_info = (PebbleAppMessagingInfo) {
    .buffer_sizes = {
      .inbound = 124,
      .outbound = 124,
    },
  };
  http_capture_main(&handlers);
  #endif

  app_event_loop(params, &handlers);
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Sliders Window");
  window_stack_push(&window, true);
  window_set_background_color(&window, COLOR_BACKGROUND);

  resource_init_current_app(&APP_RESOURCES);

  #if ROCKSHOT
  http_set_app_id(15);
  http_register_callbacks((HTTPCallbacks) {
    .success = http_success
  }, NULL);
  http_capture_init(ctx);
  #endif

  font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONO_22));
  font_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONO_BOLD_34));

  layer_init(&layer_hours, GRect(0, PADDING, 144, HEIGHT));
  layer_hours.update_proc = &hours_layer_update_callback;
  layer_add_child(&window.layer, &layer_hours);

  layer_init(&layer_minutes, GRect(0, PADDING * 2 + HEIGHT, 144, HEIGHT));
  layer_minutes.update_proc = &minutes_layer_update_callback;
  layer_add_child(&window.layer, &layer_minutes);

  layer_init(&layer_seconds, GRect(0, PADDING * 3 + HEIGHT * 2, 144, HEIGHT));
  layer_seconds.update_proc = &seconds_layer_update_callback;
  layer_add_child(&window.layer, &layer_seconds);
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  PblTm now = get_now();
  layer_mark_dirty(&layer_seconds);
  if (now.tm_sec == 0) {
    if (now.tm_min == 0) {
      layer_mark_dirty(&layer_hours);
    }
    layer_mark_dirty(&layer_minutes);
  }
}

void draw_number(GContext* ctx, char* str, int num, int pos) {
  bool is_now = pos == 2;

  graphics_context_set_text_color(ctx, COLOR_BACKGROUND);

  GFont font = (is_now ? font_bold : font_normal);
  str = "XX";
  snprintf(str, sizeof(str), "%02d", num);
  GRect text_box = GRect(0, 0, 100, (is_now ? FONT_SIZE : FONT_SIZE_SMALL));
  GSize text_size = graphics_text_layout_get_max_used_size(ctx, str, font, text_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  text_box.origin = GPoint((36 * pos) - (text_size.w / 2), 14 - (text_size.h / 2) + (is_now ? -1 : 3));
  text_box.size = text_size;
  graphics_text_draw(ctx, str, font, text_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

void hours_layer_update_callback(Layer* me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, COLOR_FOREGROUND);
  graphics_fill_rect(ctx, me->bounds, 0, GCornerNone);

  static char* hour_str[5];
  PblTm now = get_now();
  int hour_now = now.tm_hour;

  for (int h = 0; h < 5; h += 1) {
    int hour = hour_now - 2 + h;
    if (hour < 0) {
      hour += 24;
    }
    if (hour >= 24) {
      hour -= 24;
    }
    if (! clock_is_24h_style() && hour > 12) {
      hour -= 12;
    }
    draw_number(ctx, hour_str[h], hour, h);
  }
}

void minutes_layer_update_callback(Layer* me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, COLOR_FOREGROUND);
  graphics_fill_rect(ctx, me->bounds, 0, GCornerNone);

  static char* minute_str[5];
  PblTm now = get_now();
  int minute_now = now.tm_min;

  for (int m = 0; m < 5; m += 1) {
    int minute = minute_now - 2 + m;
    if (minute < 0) { minute += 60; }
    if (minute >= 60) { minute -= 60; }
    draw_number(ctx, minute_str[m], minute, m);
  }
}

void seconds_layer_update_callback(Layer* me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, COLOR_FOREGROUND);
  graphics_fill_rect(ctx, me->bounds, 0, GCornerNone);

  static char* second_str[5];
  PblTm now = get_now();
  int second_now = now.tm_sec;

  for (int s = 0; s < 5; s += 1) {
    int second = second_now - 2 + s;
    if (second < 0) { second += 60; }
    if (second >= 60) { second -= 60; }
    draw_number(ctx, second_str[s], second, s);
  }
}

PblTm get_now() {
  PblTm now;
  get_time(&now);
  return now;
}

#if ROCKSHOT
void http_success(int32_t cookie, int http_status, DictionaryIterator *dict, void *ctx) {}
#endif