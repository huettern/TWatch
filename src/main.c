//Thanks to http://ninedof.wordpress.com/
#include <pebble.h>
 
Window* window;

TextLayer *bkgnd_layer, *watch_h_layer10, *watch_h_layer1,  *watch_min_layer10, *watch_min_layer1;

TextLayer *title_layer, *weather_layer, *text_layer, *time_layer;

static GBitmap *bat_base_img;
TextLayer *bat_cov_layer;
Layer* bat_stat_layer;

static BitmapLayer *bat_layer;
  int bat_level = 1; //%

char timeBuf[] = "00:00";

char seconds_str[] = "00";

char hrs_10[] = "0";
char hrs_1[] = "0";
char min_10[] = "0";
char min_1[] = "0";

bool clock_only = false;

//#define DEBUG_TIME true
int debugctr = 5;

AppTimer *anim_timer;

enum {
  KEY_LOCATION = 0,
  KEY_TEMPERATURE = 1,
  KEY_DESCRIPTION = 2,
};

  
#define HRS_10_TOP_POS GRect(0, -31, 36, 50)
#define HRS_10_MID_POS GRect(0, 21, 36, 50)
#define HRS_10_BOT_POS GRect(0, 71, 36, 50)
  
#define HRS_1_TOP_POS GRect(36, -31, 36, 50)
#define HRS_1_MID_POS GRect(36, 21, 36, 50)
#define HRS_1_BOT_POS GRect(36, 71, 36, 50)
  
#define SEC_10_TOP_POS GRect(72, -31, 36, 50)
#define SEC_10_MID_POS GRect(72, 21, 36, 50)
#define SEC_10_BOT_POS GRect(72, 71, 36, 50)
  
#define SEC_1_TOP_POS GRect(108, -31, 36, 50)
#define SEC_1_MID_POS GRect(108, 21, 36, 50)
#define SEC_1_BOT_POS GRect(108, 71, 36, 50)
  
#define ACC_X_POS_MARGIN 200
#define ACC_X_NEG_MARGIN -200
#define ACC_Y_POS_MARGIN 0
#define ACC_Y_NEG_MARGIN -450
  

char location_buffer[64], weather_buffer[64], description_buffer[64], temperature_buffer[32], time_buffer[32];

/*****************************************************************************************
Helper Funkctions
*****************************************************************************************/ 
static TextLayer* init_text_layer(GRect location, 
                                  GColor colour, GColor background, 
                                  const char *res_id, GTextAlignment alignment)
{
  TextLayer *layer = text_layer_create(location);
  text_layer_set_text_color(layer, colour);
  text_layer_set_background_color(layer, background);
  text_layer_set_font(layer, fonts_get_system_font(res_id));
  text_layer_set_text_alignment(layer, alignment);
 
  return layer;
}

void process_tuple(Tuple *t)
{
  //Get key
  int key = t->key;
 
  //Get string value, if present
  char string_value[32];
  strcpy(string_value, t->value->cstring);
 
  //Decide what to do
  switch(key) {
    case KEY_LOCATION:
      //Location received
      snprintf(location_buffer, 32, "%s", string_value);
      snprintf(weather_buffer, 64, "%s %s °C", location_buffer, temperature_buffer);
      text_layer_set_text(weather_layer, (char*) &weather_buffer);
      break;
    case KEY_TEMPERATURE:
      //Temperature received
      snprintf(temperature_buffer, 32, "%s", string_value);
      snprintf(weather_buffer, 64, "%s %s °C", location_buffer, temperature_buffer);
      //printf("weather_buffer=%s", weather_buffer);
      text_layer_set_text(weather_layer, (char*) &weather_buffer);
      //text_layer_set_text(temperature_layer, (char*) &temperature_buffer);
      break;
    case KEY_DESCRIPTION:
      snprintf(description_buffer, 64, "%s", string_value);
      text_layer_set_text(title_layer, (char*) &description_buffer);
      break;
  }
}

void app_msg_send_int(uint8_t key, uint8_t cmd)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
 
    Tuplet value = TupletInteger(key, cmd);
    dict_write_tuplet(iter, &value);
 
    app_message_outbox_send();
}

void on_animation_stopped(Animation *anim, bool finished, void *context)
{
    //Free the memoery used by the Animation
    property_animation_destroy((PropertyAnimation*) anim);
}

void determine_bat_lvl (Layer *layer, GContext *ctx)
{
  BatteryChargeState bat_state = battery_state_service_peek();
  bat_level = bat_state.charge_percent / 10;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(2, 18-bat_level, 6, bat_level), 0, GCornerNone); //GRect(137, 12, 6, 1)
}

void set_info_overlay_visible (bool on)
{
  //BatteryChargeState battery;
  if(on == true)
  {
    clock_only = false;
    //battery = battery_state_service_peek();
    text_layer_set_text_color(title_layer, GColorWhite);
    text_layer_set_text_color(weather_layer, GColorWhite);
    text_layer_set_text_color(text_layer, GColorWhite);
    text_layer_set_background_color(bat_cov_layer, GColorClear);
  }
  else
  {
    clock_only = true;
    text_layer_set_text_color(title_layer, GColorBlack);
    text_layer_set_text_color(weather_layer, GColorBlack);
    text_layer_set_text_color(text_layer, GColorBlack);
    text_layer_set_background_color(bat_cov_layer, GColorBlack);
  }
}

/*****************************************************************************************
Animation Layer
*****************************************************************************************/ 
void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay)
{
    //Declare animation
    PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
 
    //Set characteristics
    animation_set_duration((Animation*) anim, duration);
    animation_set_delay((Animation*) anim, delay);
 
    //Set stopped handler to free memory
    AnimationHandlers handlers = {
        //The reference to the stopped handler is the only one in the array
        .stopped = (AnimationStoppedHandler) on_animation_stopped
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
 
    //Start animation!
    animation_schedule((Animation*) anim);
}

/*****************************************************************************************
Human input routines
*****************************************************************************************/ 
void accel_sample_cb (AccelData *data, uint32_t num_samples)
{
  
  int acc_x = data->x;
  int acc_y = data->y;
  int acc_z = data->z;
  int samples = num_samples;
  //APP_LOG(APP_LOG_LEVEL_WARNING, "Samples: %d X=%d Y=%d Z=%d", samples, acc_x, acc_y, acc_z );
  
  if( ( (data->x < ACC_X_POS_MARGIN) && (data->x > ACC_X_NEG_MARGIN) ) && ( (data->y < ACC_Y_POS_MARGIN) && (data->y > ACC_Y_NEG_MARGIN) ) )
  {
    set_info_overlay_visible(true);
  }
  else
  {
    set_info_overlay_visible(false);
  }
}

/*****************************************************************************************
Communication to Phone
*****************************************************************************************/ 
static void in_received_handler(DictionaryIterator *iter, void *context)
{
 //Get data
  Tuple *t = dict_read_first(iter);
  if(t)
  {
    process_tuple(t);
  }
 
  //Get next
  while(t != NULL)
  {
    t = dict_read_next(iter);
    if(t)
    {
      process_tuple(t);
    }
  }
}

/*****************************************************************************************
Handle Time Layer
*****************************************************************************************/ 
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  //Here we will update the watchface display
  //Get Time
  strftime(timeBuf, sizeof("0000"), "%H%M", tick_time);
  
  int seconds = tick_time->tm_sec;
  int minutes = tick_time->tm_min;
  int hours = tick_time->tm_hour;
  
  if(bat_level < 10) bat_level++;
  else bat_level = 0;
  
  #ifdef DEBUG_TIME
  /* OVERRIDE TIME FOR DEBUG */
  if(debugctr > 1)
    {
  hours = 9;
  minutes = 59;
  seconds = 58;
  snprintf(timeBuf, sizeof("0000"), "0959");
  }
  else if(debugctr == 1)
    {
  hours = 9;
  minutes = 59;
  seconds = 59;
  snprintf(timeBuf, sizeof("0000"), "0959");
  }
  else if (debugctr == 0)
    {
  hours = 10;
  minutes = 0;
  seconds = 0;
  snprintf(timeBuf, sizeof("0000"), "1000");
  debugctr = 5;
    
  }
  debugctr--;
  #endif
  
  strncpy(hrs_10, timeBuf, 1);
  strncpy(hrs_1, timeBuf+1, 1);
  strncpy(min_10, timeBuf+2, 1);
  strncpy(min_1, timeBuf+3, 1);
  
  if(seconds == 59)
    {
    //ANIMATE MIN_1 OUT
    animate_layer(text_layer_get_layer(watch_min_layer1), &SEC_1_MID_POS, &SEC_1_BOT_POS, 300, 400);
    }
  if(seconds == 0)
    {
    //ANIMATE MIN_1 IN
    text_layer_set_text(watch_min_layer1, min_1);
    animate_layer(text_layer_get_layer(watch_min_layer1), &SEC_1_TOP_POS, &SEC_1_MID_POS, 300, 0);
    }
  
  if( (seconds == 59) && (((minutes-9) % 10) == 0) )
    {
    //ANIMATE MIN_10 OUT
    animate_layer(text_layer_get_layer(watch_min_layer10), &SEC_10_MID_POS, &SEC_10_BOT_POS, 300, 500);    
    }
  if( (seconds == 0) && ((minutes % 10) == 0) )
    {
    //ANIMATE MIN_10 IN
    text_layer_set_text(watch_min_layer10, min_10);
    animate_layer(text_layer_get_layer(watch_min_layer10), &SEC_10_TOP_POS, &SEC_10_MID_POS, 300, 100);
    }
  
  if( (seconds == 59) && ( minutes == 59)  )
    {
    //ANIMATE HRS_1 OUT
    animate_layer(text_layer_get_layer(watch_h_layer1), &HRS_1_MID_POS, &HRS_1_BOT_POS, 300, 600);  
    }
  if( (seconds == 0) && (minutes == 0)  )
    {
    //ANIMATE HRS_1 IN
    text_layer_set_text(watch_h_layer1, hrs_1);
    animate_layer(text_layer_get_layer(watch_h_layer1), &HRS_1_TOP_POS, &HRS_1_MID_POS, 300, 200); 
    }
  
  if( (seconds == 59) && ( minutes == 59) && (((hours-9) % 10) == 0) )
    {
    //ANIMATE SEC_10 OUT
    animate_layer(text_layer_get_layer(watch_h_layer10), &HRS_10_MID_POS, &HRS_10_BOT_POS, 300, 700);     
    }
  if( (seconds == 0) && (minutes == 0) && ( (hours%10) == 0) )
    {
    //ANIMATE SEC_10 IN
    text_layer_set_text(watch_h_layer10, hrs_10);
    animate_layer(text_layer_get_layer(watch_h_layer10), &HRS_10_TOP_POS, &HRS_10_MID_POS, 300, 300); 
    }
  
  //Change the TextLayer text to show the new time!
  text_layer_set_text(watch_min_layer1, min_1);
  text_layer_set_text(watch_h_layer10, hrs_10);
  text_layer_set_text(watch_h_layer1, hrs_1);
  text_layer_set_text(watch_min_layer10, min_10);
  
  snprintf(seconds_str, sizeof("00"), "%d", seconds);
  text_layer_set_text(text_layer, seconds_str);
  
  //Request new Data - Key and Data doesn't matter! Every five minutes
  if(tick_time->tm_sec % 20 == 0)
  {
    //Send an arbitrary message, the response will be handled by in_received_handler()
    app_msg_send_int(5, 5);
  }
} 

/*****************************************************************************************
window load and unload functions
*****************************************************************************************/ 
void window_unload(Window *window)
{
  //app_timer_cancel(anim_timer);
  //We will safely destroy the Window's elements here!
  text_layer_destroy(weather_layer);
  text_layer_destroy(watch_h_layer10);
  text_layer_destroy(watch_h_layer1);
  text_layer_destroy(watch_min_layer10);
  text_layer_destroy(watch_min_layer1);
  text_layer_destroy(title_layer);
  text_layer_destroy(text_layer);
  layer_destroy(bat_stat_layer);
}

void window_load(Window *window)
{
  //Load font
  ResHandle font_handle = resource_get_handle(RESOURCE_ID_FONT_IMAGINE_42);
  //load bat img
  bat_base_img = gbitmap_create_with_resource(RESOURCE_ID_IMG_BAT_BASE);
  
  //create image layer
  bat_layer = bitmap_layer_create(GRect(134, 0, 10, 21));
  bitmap_layer_set_bitmap(bat_layer, bat_base_img);
  
  bat_cov_layer = init_text_layer(GRect(134, 0, 10, 21), GColorClear, GColorClear, "RESOURCE_ID_GOTHIC_18", GTextAlignmentLeft);
 
  bat_stat_layer = layer_create(GRect(134, 0, 10, 21));
  layer_set_update_proc(bat_stat_layer, determine_bat_lvl);
  
  
  //create background layer
  bkgnd_layer = init_text_layer(GRect(0, 0, 144, 168), GColorBlack, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentLeft);
  //create weather layer
  weather_layer = init_text_layer(GRect(0, 0, 134, 23), GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentLeft);
  text_layer_set_text(weather_layer, "N/A");
    
  //create watch layer
  watch_h_layer10 = init_text_layer(GRect(0, 21, 36, 50),GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentCenter);
  text_layer_set_font(watch_h_layer10, fonts_load_custom_font(font_handle));
  text_layer_set_text(watch_h_layer10, "");
  //create watch layer
  watch_h_layer1 = init_text_layer(GRect(36, 21, 36, 50),GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentCenter);
  text_layer_set_font(watch_h_layer1, fonts_load_custom_font(font_handle));
  text_layer_set_text(watch_h_layer1, "");
  
  //create watch layer
  watch_min_layer10 = init_text_layer(GRect(72, 21, 36, 50),GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentCenter);
  text_layer_set_font(watch_min_layer10, fonts_load_custom_font(font_handle));
  text_layer_set_text(watch_min_layer10, "");
  //create watch layer
  watch_min_layer1 = init_text_layer(GRect(108, 21, 36, 50),GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentCenter);
  text_layer_set_font(watch_min_layer1, fonts_load_custom_font(font_handle));
  text_layer_set_text(watch_min_layer1, "");
  
  //create title layer
  title_layer = init_text_layer(GRect(0, 71, 144, 23), GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18_BOLD", GTextAlignmentCenter);
  text_layer_set_text(title_layer, "");
  //create text layer
  text_layer = init_text_layer(GRect(0, 94, 144, 74), GColorWhite, GColorBlack, "RESOURCE_ID_GOTHIC_18", GTextAlignmentCenter);
  text_layer_set_text(text_layer, "");
  
  set_info_overlay_visible(false);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(bkgnd_layer));
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(watch_h_layer10));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(watch_h_layer1));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(watch_min_layer10));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(watch_min_layer1));
  
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(bat_layer));
  layer_add_child(window_get_root_layer(window), bat_stat_layer);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(bat_cov_layer));
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(title_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
}


/*****************************************************************************************
Init, DeInit
*****************************************************************************************/
void init()
{
  //Initialize the app elements here!
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) tick_handler); //MINUTE_UNIT SECOND_UNIT
  accel_data_service_subscribe(5, accel_sample_cb); //add accelerometer measurement. CB called after 2 samples
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  
  //anim_timer = app_timer_register(1000, (AppTimerCallback) anim_timer_cb, NULL);
  
  //Register AppMessage events
  app_message_register_inbox_received(in_received_handler);
  //Largest possible input and output buffer sizes
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());    
  
  
  window_stack_push(window, true);
}
 
void deinit()
{
  //De-initialize elements here to save memory!
  tick_timer_service_unsubscribe();
  accel_data_service_unsubscribe();
  gbitmap_destroy(bat_base_img);
  window_destroy(window);
}
 
/*****************************************************************************************
Main
*****************************************************************************************/
int main(void)
{
  init();
  app_event_loop();
  deinit();
}