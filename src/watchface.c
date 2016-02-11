#include <pebble.h>
#include <gcolor_definitions.h>

static Window *s_main_window;
static Layer *s_layer;

static time_t ctime; 
static struct tm *ctick_time = NULL;

static int tz2 = +18;

static bool isBtConnected = false;

static AppSync s_sync;
static uint8_t s_sync_buffer[32];

static void bt_handler(bool connected) {
  isBtConnected = connected;
  
  if(connected)
  	vibes_short_pulse();
  else
  	vibes_double_pulse();
  layer_mark_dirty(s_layer);
}

static void update_time() {
  // Get a tm structure
  ctime = time(NULL); 
  ctick_time = localtime(&ctime);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
   update_time();
   layer_mark_dirty(s_layer);
}

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  // Update UI here
  static char s_count_buffer[32];
  snprintf(s_count_buffer, sizeof(s_count_buffer), "Key: %d Value: %d Old: %d", key, (int)new_tuple->value->int32, (int)old_tuple->value->int32);
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  // An error occured!
  APP_LOG(APP_LOG_LEVEL_ERROR, "sync error!");
}

static void draw_hour(Layer *this_layer, GContext *ctx, int offset, int tz_offset, int y, const char *font, int font_height) {
  GRect bounds = layer_get_bounds(this_layer);
  
  // min_per_pixel is scaled 10x to get more precision without having
  // to go to floating point
  int16_t min_per_pixel = (10*bounds.size.w)/100;
  
  time_t otime = ctime+3600*(offset); 
  struct tm *tick_time = localtime(&otime);

  GRect frame = GRect(0+6*offset*min_per_pixel-(min_per_pixel*tick_time->tm_min)/10, y-font_height/2, 
                      bounds.size.w, font_height + 2);
  

  static char s_buffer[3];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H" : "%I", tick_time);
  
//    APP_LOG(APP_LOG_LEVEL_INFO, "%d: m: %d frame: %d y: %d %s", offset, min_per_pixel, frame.origin.x, frame.origin.y, s_buffer);
                                            
  graphics_draw_text(ctx, 
    s_buffer,
    fonts_get_system_font(font),
    frame,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentCenter,
    NULL
  );
    
  if (offset == 0) {	  
	  GSize size = graphics_text_layout_get_content_size(s_buffer,
		fonts_get_system_font(font),
		frame,
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentCenter
	  );
  
	  frame.origin.x += size.w/2 + 10;
	  frame.origin.y += font_height - 18;
  
	  otime = ctime+3600*(tz_offset+offset); 
	  tick_time = localtime(&otime);
	  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
											  "%H" : "%I", tick_time);
  
	  graphics_draw_text(ctx, 
		s_buffer,
		fonts_get_system_font(FONT_KEY_GOTHIC_18),
		frame,
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentCenter,
		NULL
	  );
	}
}

static void draw_minutes(Layer *this_layer, GContext *ctx, int y) {
  GRect bounds = layer_get_bounds(this_layer);

  int16_t min_per_pixel = (10*bounds.size.w)/100;
  
  int16_t min;
  int16_t offset = 0-ctick_time->tm_min*min_per_pixel/10;
  
//   APP_LOG(APP_LOG_LEVEL_INFO, "t: %d m: %d o: %d from: %d to %d", tick_time->tm_min, min_per_pixel, offset, 
//   (-60)*min_per_pixel/10+offset,
//   (120)*min_per_pixel/10+offset);
  
  for(min = -60; min <= 100; min += 5) {
  	  int16_t x = min*min_per_pixel/10+offset+bounds.size.w/2;
  	  if (x > 0 && x < bounds.size.w)
  	  	graphics_draw_line(ctx, GPoint(x, y+6), GPoint(x, y+9));
  }
  
  for(min = -60; min <= 100; min += 15) {
  	  int16_t x = min*min_per_pixel/10+offset+bounds.size.w/2;
  	  if (x > 0 && x < bounds.size.w)
  	  	graphics_draw_line(ctx, GPoint(x, y), GPoint(x, y+15));
  }

  for(min = -60; min <= 120; min += 15) {
  	  int16_t x = min*min_per_pixel/10+offset;
  	  
  	  GRect frame = GRect(x, y+15, bounds.size.w, 24);
  	  
  	  static char s_buffer[3];
  	  int16_t value = min;
  	  value %= 60;
  	  if(value < 0)
  	  	value = 60+value;
  	  
  	  snprintf(s_buffer, 3, "%d", value);
  	  
  	  graphics_draw_text(ctx, 
  	  	s_buffer,
    	fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    	frame,
    	GTextOverflowModeTrailingEllipsis,
    	GTextAlignmentCenter,
    	NULL
  	  );  	  
  	    
  }

}

static void draw_day(Layer *this_layer, GContext *ctx, int day, int y) {
  GRect bounds = layer_get_bounds(this_layer);

  int16_t pixels_per_day = (100*bounds.size.w)/3;
  
  time_t otime = ctime + day*86400;
  struct tm *tick_time = localtime(&otime);
  
  int16_t percent_of_day = (tick_time->tm_hour*3600 + tick_time->tm_min*60)/864;
  int16_t offset = pixels_per_day*percent_of_day/100;

//   APP_LOG(APP_LOG_LEVEL_INFO, "p: %d o: %d px: %d", percent_of_day, offset, pixels_per_day);
  
  GRect frame = GRect(0-offset/100+day*pixels_per_day/100+pixels_per_day/200, y-20, bounds.size.w, 18);
  
//   APP_LOG(APP_LOG_LEVEL_INFO, "%d: m: %d frame: %d", offset, min_per_pixel, frame.origin.x);
  
  static char s_buffer[4];
  strftime(s_buffer, sizeof(s_buffer), "%a", tick_time);
                                          
  graphics_draw_text(ctx, 
    s_buffer,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    frame,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentCenter,
    NULL
  );
  
  GRect frame_day = GRect(0-offset/100+day*pixels_per_day/100+pixels_per_day/200, y+2, bounds.size.w, 18);
  strftime(s_buffer, sizeof(s_buffer), "%d", tick_time);
  
  graphics_draw_text(ctx, 
    s_buffer,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    frame_day,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentCenter,
    NULL
  );

}

static void draw_day_ticks(Layer *this_layer, GContext *ctx, int y) {
  GRect bounds = layer_get_bounds(this_layer);

  int16_t pixels_per_day = (100*bounds.size.w)/3;
  
  int16_t percent_of_day = (ctick_time->tm_hour*3600 + ctick_time->tm_min*60)/864;
  int16_t offset = pixels_per_day*percent_of_day/100;

//   APP_LOG(APP_LOG_LEVEL_INFO, "p: %d o: %d px: %d", percent_of_day, offset, pixels_per_day);
  
  //GRect frame = GRect(0-offset/100+day*pixels_per_day/100+pixels_per_day/200, y-18, bounds.size.w, 18);


  int16_t day;
  for(day = -1; day <= 4; day += 1) {
  	  int16_t x = (day*pixels_per_day-offset)/100+bounds.size.w/2;
//   	  APP_LOG(APP_LOG_LEVEL_INFO, "d: %d x: %d", day, x);
	  if (x > 0 && x < bounds.size.w)
  	  	graphics_draw_line(ctx, GPoint(x, y+10), GPoint(x, y-10));
  }

  int16_t qday; // quarter day
  for(qday = -4; qday <= 16; qday += 1) {
  	  int16_t x = (qday*pixels_per_day/4-offset)/100+bounds.size.w/2;
//   	  APP_LOG(APP_LOG_LEVEL_INFO, "d: %d x: %d", day, x);
	  if (x > 0 && x < bounds.size.w)
  	  	graphics_draw_line(ctx, GPoint(x, y+1), GPoint(x, y+3));
  }

}

static void draw_bat(Layer *this_layer, GContext *ctx, int y) {

  GRect bounds = layer_get_bounds(this_layer);

  BatteryChargeState bat = battery_state_service_peek();
  
  int16_t width = bounds.size.w;
  
  int16_t offset = bounds.size.w/2 - (100 - bat.charge_percent)*width/100;
  
//   APP_LOG(APP_LOG_LEVEL_INFO, "o: %d y: %d w: %d", offset, y, width);
  
  int g = 50; // 100-50
  int ye = 25; // 50-25 
  int r = 25; // 25-0
  
  GRect green  = GRect(offset, y, width*g/100, 6);
  GRect yellow = GRect(green.origin.x+green.size.w,   y, width*ye/100, 6);
  GRect red =    GRect(yellow.origin.x+yellow.size.w, y, width*r/100, 6);
  
  graphics_context_set_fill_color(ctx, GColorGreen);
  graphics_fill_rect(ctx, green, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_rect(ctx, yellow, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, red, 0, GCornerNone);
  

}


static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  
  uint16_t ms_start = time_ms(NULL, NULL);
  
  graphics_context_set_antialiased(ctx, true);
  
  if (!isBtConnected) {
	graphics_context_set_fill_color(ctx, GColorDukeBlue);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  } else {
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }
    
  graphics_context_set_fill_color(ctx, GColorBlack);    
    
  uint16_t ms_fill = time_ms(NULL, NULL);

  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, GPoint(bounds.size.w/2, 0), GPoint(bounds.size.w/2, bounds.size.h));
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  int hour_loc = bounds.size.h/2;
  
  if(ctick_time->tm_min < 20)
  	draw_hour(this_layer, ctx, -1, tz2, hour_loc, FONT_KEY_ROBOTO_BOLD_SUBSET_49, 49);
  draw_hour(this_layer, ctx,    0, tz2, hour_loc, FONT_KEY_ROBOTO_BOLD_SUBSET_49, 49);
  draw_hour(this_layer, ctx,    1, tz2, hour_loc, FONT_KEY_ROBOTO_BOLD_SUBSET_49, 49);
  if(ctick_time->tm_min > 50)
  	draw_hour(this_layer, ctx,  2, tz2, hour_loc, FONT_KEY_ROBOTO_BOLD_SUBSET_49, 49);
  
  graphics_context_set_stroke_width(ctx, 1);
  uint16_t ms_hour = time_ms(NULL, NULL);
  
  draw_minutes(this_layer, ctx, bounds.size.h/2+49/2+5);
  
  uint16_t ms_day = time_ms(NULL, NULL);
  
  int day_loc = bounds.size.h/2-49/2-12;

  draw_day(this_layer, ctx, -1, day_loc);
  draw_day(this_layer, ctx, 0, day_loc);
  draw_day(this_layer, ctx, 1, day_loc);
  draw_day(this_layer, ctx, 2, day_loc);

  draw_day_ticks(this_layer, ctx, bounds.size.h/2-49/2-12);

  uint16_t ms_end = time_ms(NULL, NULL);
  
  draw_bat(this_layer, ctx, 2);
  
  static int repaints = 0; 
  ++repaints;
  
  ms_fill = ms_fill < ms_start ? ms_fill + 1000 : ms_fill;
  ms_hour = ms_hour < ms_start ? ms_hour + 1000 : ms_hour;
  ms_day = ms_day < ms_start ? ms_day + 1000 : ms_day;
  ms_end = ms_end < ms_start ? ms_end + 1000 : ms_end;
  
  static uint16_t tt_max = 0;
  if(ms_end-ms_start > tt_max)
  	tt_max = ms_end-ms_start;
  
//   char buf[20];
//   snprintf(buf, 19, "%d %d %d %d %d %d", repaints, tt_max, 
//   	ms_fill-ms_start,
//   	ms_hour-ms_start,
//   	ms_day-ms_start,
//   	ms_end-ms_start);
  
  //APP_LOG(APP_LOG_LEVEL_INFO, buf);
  
//   GRect repaint_box = GRect(5, bounds.size.h-18, bounds.size.w, 18);
//   graphics_draw_text(ctx, 
//     buf,
//     fonts_get_system_font(FONT_KEY_GOTHIC_18),
//     repaint_box,
//     GTextOverflowModeTrailingEllipsis,
//     GTextAlignmentLeft,
//     NULL
//   );
  
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_layer = layer_create(
      GRect(0, 0, bounds.size.w, bounds.size.h));

  // Improve the layout to be more like a watchface
//   layer_set_background_color(s_layer, GColorClear);
//   text_layer_set_text_color(s_time_layer, GColorBlack);
//   text_layer_set_text(s_time_layer, "00:00");
//   text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
//   text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Set the update_proc
  layer_set_update_proc(s_layer, canvas_update_proc);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, s_layer);
  

}

static void main_window_unload(Window *window) {
	// Destroy TextLayer
	layer_destroy(s_layer);
}

static void init() {

  update_time();

  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Setup initial values
  Tuplet initial_values[] = {
    TupletInteger(AppKeyReady, 0),
    TupletInteger(REFRESH, 0),
    TupletInteger(TEMP, 0),
    TupletInteger(TEMP_HIGH, 0),
    TupletInteger(TEMP_LOW, 0),
  };

  // Begin using AppSync
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL); 
}

static void deinit() {
  window_destroy(s_main_window);
  app_sync_deinit(&s_sync);
}


int main(void) {
  init();
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  connection_service_subscribe((ConnectionHandlers) { 
  	.pebble_app_connection_handler = bt_handler });
  isBtConnected = connection_service_peek_pebble_app_connection();
  app_event_loop();
  deinit();
}
