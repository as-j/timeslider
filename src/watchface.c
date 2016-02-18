#include <pebble.h>

static Window *s_main_window;
static Layer *s_layer;

static time_t ctime; 
static struct tm *ctick_time = NULL;

#define INVALID_TEMP 250  
static int temp = INVALID_TEMP;
static time_t temp_age = 0;

static int tz2 = +18;
static int metric = 1;
static int buzz = 1;
static int buzz_mute = 1;

static bool isBtConnected = false;

#define AppKeyReady 1
#define REFRESH 2
#define TEMP 10

#define CONFIG_TZ_OFFSET 20
#define CONFIG_METRIC    21
#define CONFIG_BUZZ      22
#define CONFIG_BUZZ_MUTE 23

static void bt_handler(bool connected) {
  isBtConnected = connected;
  
  if (buzz) {
    int ok = 1;
    if (buzz_mute == 1) {
      int sec = time_start_of_today();
      if (sec < (3600*6))
        ok = 0;
      if (sec > (3600*22))
        ok = 0;
    } 
    if (ok) {
      if(connected) {
    	  vibes_short_pulse();
      } else {
  	    vibes_double_pulse();
      }
    }
  }
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

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    // Get the first pair
  Tuple *data = dict_read_first(iterator);
  while (data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Key received: %d", (int)data->key);
    switch(data->key) {
      case TEMP:
        temp = data->value->int32;
        temp_age = ctime;
        APP_LOG(APP_LOG_LEVEL_INFO, "Temp received: %d", temp);
        break;
      case CONFIG_TZ_OFFSET:
        switch (data->type) {
          case TUPLE_BYTE_ARRAY:
            APP_LOG(APP_LOG_LEVEL_INFO, "tz2 can't handle a byte array");
            break;
          case TUPLE_CSTRING:
          {
            char *str = data->value->cstring;
            bool neg = false;
            if (str[0] == '-') {
              neg = true;
              str += 1;
            } else if(str[0] == '+') {
              str += 1;
            }
            tz2 = atoi(str);
            if (neg) {
              tz2 *= -1;
            }
            break;
          }
          case TUPLE_INT:
          case TUPLE_UINT:
            tz2 = data->value->int32;
            break;
        }
        persist_write_int(CONFIG_TZ_OFFSET, tz2);
        APP_LOG(APP_LOG_LEVEL_INFO, "Tz2: %d type: %d", tz2, data->type);
        break;
      case CONFIG_METRIC:
        metric = data->value->int8;
        persist_write_int(CONFIG_METRIC, metric);
        APP_LOG(APP_LOG_LEVEL_INFO, "Metric: %d %d %d %d", metric, data->value->int32, data->value->int16, data->type);
        break;
      case CONFIG_BUZZ:
        buzz = data->value->int8;
        persist_write_int(CONFIG_BUZZ, buzz);
        APP_LOG(APP_LOG_LEVEL_INFO, "Buzz: %d", buzz);
        break;
      case CONFIG_BUZZ_MUTE:
        buzz_mute = data->value->int8;
        persist_write_int(CONFIG_BUZZ_MUTE, buzz_mute);
        APP_LOG(APP_LOG_LEVEL_INFO, "Buzz Mute: %d", buzz_mute);
        break;
      
    }
    data = dict_read_next(iterator);
  } 
  layer_mark_dirty(s_layer);
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
    
  if (offset == 0 && tz_offset != 0) {	  
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

static void draw_temp(Layer *this_layer, GContext *ctx, int y) {
  GRect bounds = layer_get_bounds(this_layer);
  
  int16_t deg_per_pixel = 0;
  if (metric)
    deg_per_pixel = (45*bounds.size.w)/100;
  else
    deg_per_pixel = (35*bounds.size.w)/100;
  
  int16_t temp_conv = temp;
  if (!metric)
    temp_conv = 90*temp/50 + 32;
  
  int16_t deg;
  int16_t offset = 0-temp_conv*deg_per_pixel/10;
  
  int16_t tbase = temp_conv - temp_conv%10;
  
  for(deg = tbase-20; deg <= tbase+20; deg += 2) {
      int th = abs(deg%10);
      if (th > 1 && th < 9) {
  	    int16_t x = deg*deg_per_pixel/10+offset+bounds.size.w/2;
  	    if (x > 0 && x < bounds.size.w)
    	  	graphics_draw_line(ctx, GPoint(x, y), GPoint(x, y+3));
      }
  }

  for(deg = tbase-20; deg <= tbase+20; deg += 10) {
  	  int16_t x = deg*deg_per_pixel/10+offset;
  	  
  	  GRect frame = GRect(x, y-12, bounds.size.w, 18);
  	  
  	  static char s_buffer[4];
  	  snprintf(s_buffer, 4, "%d", deg);
  	  
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

#define GREATER(x,y) ((x>y) ? (x) : (y))

static void draw_step(Layer *this_layer, GContext *ctx, int y, int total_steps) {
  GRect bounds = layer_get_bounds(this_layer);

  int16_t step_per_pixel = (bounds.size.w)/100;
  
  int16_t offset = 0-total_steps*step_per_pixel/10;
  
  int step = total_steps-(total_steps % 100);
  for(step = GREATER(step-1000, 0); step <= total_steps+1000; step += 100) {
      if (step%500) {
  	    int16_t x = step*step_per_pixel/10+offset+bounds.size.w/2;
  	    if (x > 0 && x < bounds.size.w)
    	  	graphics_draw_line(ctx, GPoint(x, y), GPoint(x, y+3));
      }
  }

  step = total_steps-(total_steps % 500);
  for(step = GREATER(step-1000, 0); step <= total_steps+1000; step += 500) {
  	  int16_t x = step*step_per_pixel/10+offset;
  	  
  	  GRect frame = GRect(x, y-8, bounds.size.w, 14);
  	  
  	  static char s_buffer[8];
  	  snprintf(s_buffer, 8, "%d", step);
  	  
  	  graphics_draw_text(ctx, 
  	  	s_buffer,
    	fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
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
  
#ifdef PBL_COLOR
  int height = 8;
#else
  int height = 4;
#endif
  
  GRect green  = GRect(offset, y, width*g/100, height);
  GRect yellow = GRect(green.origin.x+green.size.w,   y, width*ye/100, height);
#ifndef PBL_COLOR
  height /= 2;
#endif
  GRect red =    GRect(yellow.origin.x+yellow.size.w, y, width*r/100, height);
  
  graphics_context_set_fill_color(ctx,  COLOR_FALLBACK(GColorGreen, GColorWhite));
  graphics_fill_rect(ctx, green, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorYellow, GColorLightGray));
  graphics_fill_rect(ctx, yellow, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorRed, GColorDarkGray));
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
  
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorRed, GColorWhite));
#ifdef PBL_PLATFORM_APLITE
  graphics_context_set_stroke_width(ctx, 1);
#else
    graphics_context_set_stroke_width(ctx, 3);
#endif 
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
  
  if (temp != INVALID_TEMP) {
    draw_temp(this_layer, ctx, bounds.size.h/2+75);
  } 
  
  if ((ctime-temp_age) > 5400) {
    // Prepare dictionary
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    // Write data
    int key = REFRESH;
    int value = 1;
    dict_write_int(iterator, key, &value, sizeof(int), true /* signed */);
    // Send the data!
    app_message_outbox_send();
    temp_age = ctime; // only ask every 90 minutes....
  }

  uint16_t ms_end = time_ms(NULL, NULL);
  
  draw_bat(this_layer, ctx, 0);
  
#if defined(PBL_HEALTH)
  // Check step data is available
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, 
                                                                    time_start_of_today(), ctime);
  
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    // Data is available!
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    draw_step(this_layer, ctx, 25, total_steps);
  }
#endif
  
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
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  app_message_open(64, 64);
  
  if (persist_exists(CONFIG_TZ_OFFSET))
    tz2 = persist_read_int(CONFIG_TZ_OFFSET);
  if (persist_exists(CONFIG_METRIC))
    metric = persist_read_int(CONFIG_METRIC);
  if (persist_exists(CONFIG_BUZZ))
    buzz = persist_read_int(CONFIG_BUZZ);
  if (persist_exists(CONFIG_BUZZ_MUTE))
    buzz_mute = persist_read_int(CONFIG_BUZZ_MUTE);
  

}

static void deinit() {
  window_destroy(s_main_window);
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
