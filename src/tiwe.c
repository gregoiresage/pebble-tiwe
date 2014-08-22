#include "pebble.h"

static Layer *simple_bg_layer;
static Window *window;
static GPoint center;

typedef struct {
  GPoint hour_display;
  GPoint random_display;
  GPoint tmp;
} Dot;

static Dot background[12];
static Dot hours[4];
static Dot minutes[6];

static bool animToHour = true;
static bool hourDisplayed = false;

static Animation* animation;
static AnimationImplementation animImpl;

static void generateRandomDisplay(){
  srand(time(NULL));

  for (int i = 0; i < 4; ++i) {
    hours[i].random_display.y = rand()%168;
    hours[i].random_display.x = rand()%144;
  }
  for (int i = 0; i < 6; ++i) {
    minutes[i].random_display.y = rand()%168;
    minutes[i].random_display.x = rand()%144;
  }
  for (int i = 0; i < 12; ++i) {
    background[i].random_display.y = rand()%168;
    background[i].random_display.x = rand()%144;
  }
}

static void animationUpdate(struct Animation *animation, const uint32_t time_normalized){
  int percent = time_normalized * 100 / ANIMATION_NORMALIZED_MAX;

  if(!animToHour){
    percent = 100 - percent;
  }
  
  for (int i = 0; i < 12; ++i) {
    background[i].tmp.y = background[i].random_display.y + (background[i].hour_display.y - background[i].random_display.y) * percent / 100;
    background[i].tmp.x = background[i].random_display.x + (background[i].hour_display.x - background[i].random_display.x) * percent / 100;
  }
  for (int i = 0; i < 4; ++i) {
    hours[i].tmp.y = hours[i].random_display.y + (hours[i].hour_display.y - hours[i].random_display.y) * percent / 100;
    hours[i].tmp.x = hours[i].random_display.x + (hours[i].hour_display.x - hours[i].random_display.x) * percent / 100;
  }
  for (int i = 0; i < 6; ++i) {
    minutes[i].tmp.y = minutes[i].random_display.y + (minutes[i].hour_display.y - minutes[i].random_display.y) * percent / 100;
    minutes[i].tmp.x = minutes[i].random_display.x + (minutes[i].hour_display.x - minutes[i].random_display.x) * percent / 100;
  }

  layer_mark_dirty(window_get_root_layer(window));
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  for (int i = 0; i < 12; ++i) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, background[i].tmp, 6);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, background[i].tmp, 5);
  }
  for (int i = 0; i < 4; ++i) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, hours[i].tmp, 6);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, hours[i].tmp, 5);
  }
  for (int i = 0; i < 6; ++i) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, minutes[i].tmp, 4);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, minutes[i].tmp, 3);
  } 
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // int32_t second_angle  = TRIG_MAX_ANGLE * tick_time->tm_sec / 60;
  int32_t minutes_angle = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  int32_t hours_angle   = TRIG_MAX_ANGLE * ((tick_time->tm_hour % 12) * 60 + tick_time->tm_min) / (12 * 60);

  for (int i = 0; i < 4; ++i) {
    hours[i].hour_display.y = (int16_t)(-cos_lookup(hours_angle) * (int32_t)(i * 15) / TRIG_MAX_RATIO) + center.y;
    hours[i].hour_display.x = (int16_t)(sin_lookup(hours_angle) * (int32_t)(i * 15) / TRIG_MAX_RATIO) + center.x;
  }
  for (int i = 0; i < 6; ++i) {
    minutes[i].hour_display.y = (int16_t)(-cos_lookup(minutes_angle) * (int32_t)((i+1) * 9 + 3) / TRIG_MAX_RATIO) + center.y;
    minutes[i].hour_display.x = (int16_t)(sin_lookup(minutes_angle) * (int32_t)((i+1) * 9 + 3) / TRIG_MAX_RATIO) + center.x;
  }

  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&bounds);

  simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, simple_bg_layer);

  generateRandomDisplay();

  for (int i = 0; i < 4; ++i) {
    hours[i].tmp.y = hours[i].random_display.y;
    hours[i].tmp.x = hours[i].random_display.x;
  }
  for (int i = 0; i < 6; ++i) {
    minutes[i].tmp.y = minutes[i].random_display.y;
    minutes[i].tmp.x = minutes[i].random_display.x;
  }
  for (int i = 0; i < 12; ++i) {
    background[i].tmp.y = background[i].random_display.y;
    background[i].tmp.x = background[i].random_display.x;

    int32_t hours_angle   = TRIG_MAX_ANGLE * i / 12;
    background[i].hour_display.y = (int16_t)(-cos_lookup(hours_angle) * 70 / TRIG_MAX_RATIO) + center.y;
    background[i].hour_display.x = (int16_t)(sin_lookup(hours_angle) * 70 / TRIG_MAX_RATIO) + center.x;
  }
}

static void window_unload(Window *window) {
  layer_destroy(simple_bg_layer);
}

static void accelDataHandler(AccelData *data, uint32_t num_samples){
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "accelDataHandler: %d", data->y); 

  if(!hourDisplayed && data->y < -450){
    animToHour = true;
    hourDisplayed = true;
    if(!animation_is_scheduled(animation))
      animation_schedule(animation);
  }
  else if(hourDisplayed && data->y > -300){
    generateRandomDisplay();
    animToHour = false;
    hourDisplayed = false;
    if(!animation_is_scheduled(animation))
      animation_schedule(animation);
  }
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_set_background_color(window, GColorBlack);

  animation = animation_create();
  animImpl.update = animationUpdate;
  animation_set_duration(animation, 500);
  animation_set_curve(animation, AnimationCurveEaseOut);
  animation_set_implementation(animation, &animImpl);

  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  accel_data_service_subscribe(1, accelDataHandler);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(window);
  animation_destroy(animation);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
