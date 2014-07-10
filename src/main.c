#include <pebble.h>
 
Window* window;
TextLayer *text_layer;
TextLayer *watch_layer;
InverterLayer *inv_layer;

char timeBuf[] = "00:00";
 
 
void window_unload(Window *window)
{
  //We will safely destroy the Window's elements here!
  text_layer_destroy(watch_layer);
  
  inverter_layer_destroy(inv_layer);
}

void window_load(Window *window)
{
  //Load font
  ResHandle font_handle = resource_get_handle(FONT_MUSEO_SANS_42);
  
  //We will add the creation of the Window's elements here soon!
  watch_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_background_color(watch_layer, GColorBlack);
  text_layer_set_text_color(watch_layer, GColorWhite);
  text_layer_set_text_alignment(watch_layer, GTextAlignmentCenter);
  //text_layer_set_font(watch_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_font(watch_layer, fonts_load_custom_font(font_handle));
  

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(watch_layer));
  
  //Inverter layer
  inv_layer = inverter_layer_create(GRect(0, 0, 144, 62));
  layer_add_child(window_get_root_layer(window), (Layer*) inv_layer);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  //Here we will update the watchface display
  //Format the buffer string using tick_time as the time source
  strftime(timeBuf, sizeof("00:00"), "%H:%M", tick_time);

  //Change the TextLayer text to show the new time!
  text_layer_set_text(watch_layer, timeBuf);
} 

void init()
{
  //Initialize the app elements here!
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick_handler);
  window_stack_push(window, true);
}
 
void deinit()
{
  //De-initialize elements here to save memory!
  tick_timer_service_unsubscribe();
  window_destroy(window);
}
 
int main(void)
{
  init();
  app_event_loop();
  deinit();
}