#include <pebble.h>

#define KEY_DURATION            0
#define KEY_DURATION_IN_TRAFFIC 1
#define KEY_DISTANCE            2
#define KEY_TRAFFIC_CONDITION   3
#define KEY_ERROR_MSG           4
#define KEY_DESTINATION         5


static Window *s_main_window;
static Layer *s_background_layer;
static BitmapLayer *s_car_layer;
static GBitmap *s_car_bitmap;
static TextLayer *s_day_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static TextLayer *s_distance_layer;
static TextLayer *s_travel_layer;
static GFont s_day_font;
static GFont s_date_font;
static GFont s_time_font;
static GFont s_distance_font;
static GFont s_travel_font;


// Information about the travel time to the destination.
static int s_travel_duration;
static int s_travel_duration_in_traffic;
static int s_travel_distance;
static bool s_is_error;
static char s_duration_buffer[12];
static char s_distance_buffer[16];
static char s_conditions_buffer[32];
static char s_error_buffer[32];
static char s_destination_buffer[80];

static void update_time() {
  // Get a tm structure.
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer.
  static char buffer[] = "00:00";
  
  // Write the current hours and minutes into the buffer.
  if (clock_is_24h_style() == true) {
    // Use 24 hour format.
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format.
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  
  // Display this time on the TextLayer.
  text_layer_set_text(s_time_layer, buffer);
  
  // Set the date layer.
  static char date_buffer[32];
  strftime(date_buffer, sizeof(date_buffer), "%B %e", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);
  
  // Set the day layer.
  static char day_buffer[32];
  strftime(day_buffer, sizeof(day_buffer), "%a", tick_time);
  text_layer_set_text(s_day_layer, day_buffer);
}

static void update_travel_time() {
  // Prepare dictionary
  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);
  dict_write_uint8(iterator, 0, 0);
  // Send the message!
  app_message_outbox_send();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get travel update every 5 minutes.
  if (tick_time->tm_min % 5 == 0) {
    update_travel_time();
  }
}

static void background_layer_draw(Layer *this_layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 84, 144, 84), 0, GCornerNone);
}

static void main_window_load(Window *window) {
  // Create background layer.
  s_background_layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(s_background_layer, background_layer_draw);
  layer_add_child(window_get_root_layer(window), s_background_layer);
  
  // Create car layer.
  s_car_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CAR_BLACK);
  s_car_layer = bitmap_layer_create(GRect(4, 128, 36, 36));
  bitmap_layer_set_bitmap(s_car_layer, s_car_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_car_layer));
  
  // Create day TextLayer.
  s_day_layer = text_layer_create(GRect(12, 4, 122, 25));
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_text_color(s_day_layer, GColorDarkGray);
  text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);
  
  // Create custom font and add layer to window.
  s_day_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EXO_20));
  text_layer_set_font(s_day_layer, s_day_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_day_layer));
  
  // Create date TextLayer.
  s_date_layer = text_layer_create(GRect(12, 4, 122, 25));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorDarkGray);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  // Create custom font and add layer to window.
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EXO_20));
  text_layer_set_font(s_date_layer, s_date_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // Create time TextLayer.
  s_time_layer = text_layer_create(GRect(0, 16, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create custom font and add layer to window.
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CHANGA_ONE_48));
  text_layer_set_font(s_time_layer, s_time_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  // Create distance layer.
  s_distance_layer = text_layer_create(GRect(10, 94, 134, 32));
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_text_color(s_distance_layer, GColorWhite);
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentLeft);
  text_layer_set_text(s_distance_layer, "LOADING...");
  
  // Create custom font and add layer to window.
  s_distance_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUADA_ONE_24));
  text_layer_set_font(s_distance_layer, s_distance_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_distance_layer));
  
  // Create travel layer.
  s_travel_layer = text_layer_create(GRect(48, 120, 96, 48));
  text_layer_set_background_color(s_travel_layer, GColorClear);
  text_layer_set_text_color(s_travel_layer, GColorWhite);
  text_layer_set_text_alignment(s_travel_layer, GTextAlignmentLeft);
  
  // Create custom font and add layer to window.
  s_travel_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUADA_ONE_36));
  text_layer_set_font(s_travel_layer, s_travel_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_travel_layer));
}

static void main_window_unload(Window *window) {
  // Destroy travel elements
  text_layer_destroy(s_travel_layer);
  fonts_unload_custom_font(s_travel_font);
  // Destroy distance elements
  text_layer_destroy(s_distance_layer);
  fonts_unload_custom_font(s_distance_font);
  // Destroy time elements
  text_layer_destroy(s_time_layer);
  fonts_unload_custom_font(s_time_font);
  // Destroy date elements
  text_layer_destroy(s_date_layer);
  fonts_unload_custom_font(s_date_font);
  // Destroy day elements
  text_layer_destroy(s_day_layer);
  fonts_unload_custom_font(s_day_font);
  // Destroy car bitmap
  bitmap_layer_destroy(s_car_layer);
  gbitmap_destroy(s_car_bitmap);
  // Destroy background elements
  layer_destroy(s_background_layer);
}

static void update_ui() {
  if (s_is_error) {
    // Assemble full string and display.
    text_layer_set_text(s_distance_layer, "ERROR");
    text_layer_set_text(s_travel_layer, "");
    
  } else {
    // Assemble full string and display.
    text_layer_set_text(s_distance_layer, s_distance_buffer);
    text_layer_set_text(s_travel_layer, s_duration_buffer);
    
    // Get the color based on traffic.
    int s_traffic_duration = s_travel_duration_in_traffic - s_travel_duration;
    if (s_traffic_duration > s_travel_duration / 3) {
      text_layer_set_text_color(s_travel_layer, (GColor8){ .argb = 0b11110000 });
    } else if (s_traffic_duration > s_travel_duration / 6) {
      text_layer_set_text_color(s_travel_layer, (GColor8){ .argb = 0b11101100 });
    } else {
      text_layer_set_text_color(s_travel_layer, (GColor8){ .argb = 0b11001100 });
    }
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  // For all items
  s_is_error = false;
  while (t != NULL) {
    // Which key was received?
    switch (t->key) {
      case KEY_DURATION:
        s_travel_duration = (int)t->value->int32;
        break;
      case KEY_DURATION_IN_TRAFFIC:
        s_travel_duration_in_traffic = (int)t->value->int32;
        snprintf(s_duration_buffer, sizeof(s_duration_buffer), "%d min", (int)((s_travel_duration_in_traffic + 59) / 60.0));
        break;
      case KEY_DISTANCE:
        s_travel_distance = (int)t->value->int32;
        snprintf(s_distance_buffer, sizeof(s_distance_buffer), "%d.%d miles", (int)(s_travel_distance / 10), s_travel_distance % 10);
        break;
      case KEY_TRAFFIC_CONDITION:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "%s", t->value->cstring);
        break;
      case KEY_ERROR_MSG:
        s_is_error = true;
        snprintf(s_error_buffer, sizeof(s_error_buffer), "%s", t->value->cstring);
        break;
      case KEY_DESTINATION:
        snprintf(s_destination_buffer, sizeof(s_destination_buffer), "%s", t->value->cstring);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }
    
    // Look for next item.
    t = dict_read_next(iterator);
  }
  
  update_ui();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start.
  update_time();
  
  // Register with TickTimerService.
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window.
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
