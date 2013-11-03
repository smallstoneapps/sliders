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

#include <pebble_os.h>
#include <pebble_app.h>
#include <pebble_fonts.h>

#include "pebble-assist.h"

#include "settings.h"

#define PADDING 12
#define HEIGHT 40
#define FONT_SIZE 34
#define FONT_SIZE_SMALL 22
#define FONT_SIZE_DATE 24

static Window* window;
static Layer* layer_hours;
static Layer* layer_minutes;
static Layer* layer_seconds;
static Layer* layer_date;
static GFont* font_normal;
static GFont* font_bold;
static GFont* font_date;
static struct tm* current_time = NULL;

static void do_init();
static void do_deinit();
static void hours_layer_update_callback(Layer* me, GContext* ctx);
static void minutes_layer_update_callback(Layer* me, GContext* ctx);
static void seconds_layer_update_callback(Layer* me, GContext* ctx);
static void date_layer_update_callback(Layer* me, GContext* ctx);
static void handle_tick(struct tm* tick_time, TimeUnits units_changed);

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}

void do_init() {

  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorWhite);

  font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONO_22));
  font_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONO_BOLD_34));
  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONO_BOLD_24));

  layer_hours = layer_create(GRect(0, PADDING, 144, HEIGHT));
  layer_set_update_proc(layer_hours, &hours_layer_update_callback);
  layer_add_child(window_get_root_layer(window), layer_hours);
  layer_add_to_window(layer_hours, window);

  layer_minutes = layer_create(GRect(0, PADDING * 2 + HEIGHT, 144, HEIGHT));
  layer_set_update_proc(layer_minutes, &minutes_layer_update_callback);
  layer_add_to_window(layer_minutes, window);

  #if DATE
  layer_date = layer_create(GRect(0, PADDING * 3 + HEIGHT * 2, 144, HEIGHT));
  layer_set_update_proc(layer_date, &date_layer_update_callback);
  layer_add_to_window(layer_date, window);
  #else
  layer_seconds = layer_create(GRect(0, PADDING * 3 + HEIGHT * 2, 144, HEIGHT));
  layer_set_update_proc(layer_seconds, &seconds_layer_update_callback);
  layer_add_to_window(layer_seconds, window);
  #endif

  #if DATE
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
  #else
  tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);
  #endif
  time_t now = time(NULL);
  struct tm *tmp_time = localtime(&now);
  handle_tick(tmp_time, SECOND_UNIT);
}

void do_deinit() {
  layer_destroy(layer_hours);
  layer_destroy(layer_minutes);
  layer_destroy(layer_seconds);
  window_destroy(window);
  tick_timer_service_unsubscribe();
}

static void handle_tick(struct tm* tick_time, TimeUnits units_changed) {
  current_time = tick_time;
  #if DATE
  if (tick_time->tm_hour == 0 && tick_time->tm_min == 0 && tick_time->tm_hour == 0) {
    layer_mark_dirty(layer_date);
  }
  #else
  layer_mark_dirty(layer_seconds);
  #endif
  if (tick_time->tm_sec == 0) {
    if (tick_time->tm_min == 0) {
      layer_mark_dirty(layer_hours);
    }
    layer_mark_dirty(layer_minutes);
  }
}

void draw_number(GContext* ctx, char* str, int num, int pos) {
  bool is_now = pos == 2;

  graphics_context_set_text_color(ctx, GColorWhite);

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

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(me), 0, GCornerNone);

  if (current_time == NULL) {
    return;
  }

  char* hour_str[5];
  int hour_now = current_time->tm_hour;

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
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(me), 0, GCornerNone);

  if (current_time == NULL) {
    return;
  }

  char* minute_str[5];
  int minute_now = current_time->tm_min;

  for (int m = 0; m < 5; m += 1) {
    int minute = minute_now - 2 + m;
    if (minute < 0) { minute += 60; }
    if (minute >= 60) { minute -= 60; }
    draw_number(ctx, minute_str[m], minute, m);
  }
}

void seconds_layer_update_callback(Layer* me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(me), 0, GCornerNone);

  if (current_time == NULL) {
    return;
  }

  char* second_str[5];
  int second_now = current_time->tm_sec;

  for (int s = 0; s < 5; s += 1) {
    int second = second_now - 2 + s;
    if (second < 0) { second += 60; }
    if (second >= 60) { second -= 60; }
    draw_number(ctx, second_str[s], second, s);
  }
}

void date_layer_update_callback(Layer* me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(me), 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);

  if (current_time == NULL) {
    return;
  }

  char date_str[64];
  strftime(date_str, sizeof(date_str), DATE_FORMAT, current_time);
  graphics_text_draw(ctx, date_str, font_date, GRect(0, 3, 144, FONT_SIZE_DATE), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}