#include <pebble.h>
  
//Main Window element
static Window *s_main_window;
//Elements for the time
static TextLayer *s_time_layer;
static GFont s_time_font;
//Elements for the date
static TextLayer *s_date_layer;
static GFont s_date_font;
//Elements for the Pebble battery level
static BitmapLayer *s_battery_image_layer;
static GBitmap *s_battery_image;
static TextLayer *s_battery_level_layer;
static GFont *s_battery_font;


static void update_time() {
	//Get a tm structure
	time_t temp = time(NULL); 
	struct tm *now = localtime(&temp);

	//Character array (string) to hold the time
	static char timeString[] = "00:00";

	//Save the value of the time item
	if(clock_is_24h_style()){
		//Use 2h hour format
		strftime(timeString, sizeof("00:00"), "%H:%M", now);

	} else {
		//Use 12 hour format
		strftime(timeString, sizeof("00:00"), "%I:%M", now);
	}

	// Display this time on the TextLayer
	text_layer_set_text(s_time_layer, timeString);
}

static void update_date() {
	//Get a tm structure
	time_t temp = time(NULL);
	struct tm *now = localtime(&temp);
	
	//Character array (string) to hold the date
	static char dateString[] = "25 September";
	
	//Save the value of the date item
	strftime(dateString, sizeof("25 September"), "%d %B", now);
	
	//Set the date on the date TextLayer
	text_layer_set_text(s_date_layer, dateString);
}

static void main_window_load(Window *window) {
	//Set the background of the watchface to black
	window_set_background_color(window, GColorBlack);
	
	//===============================TIME
	//Create TextLayer for the time
	s_time_layer = text_layer_create(GRect(0, 120, 144, 50));
	text_layer_set_background_color(s_time_layer, GColorBlack);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_text(s_time_layer, "00:00");
	
	//Specify font for the time TextLayer
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LANE_36));
	text_layer_set_font(s_time_layer, s_time_font);
	
	//Align the time to the center (horizontally) of the watch
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	//===============================DATE
	//Create TextLayer for the date
	s_date_layer = text_layer_create(GRect(0, 5, 144, 50));
	text_layer_set_background_color(s_date_layer, GColorBlack);
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_text(s_date_layer, "01 January");
	
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LANE_20));
	text_layer_set_font(s_date_layer, s_date_font);
	
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	//===============================PEBBLE BATTERY
	s_battery_image_layer = bitmap_layer_create(GRect(0, 50, 52, 52));
	s_battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ICON);
	bitmap_layer_set_bitmap(s_battery_image_layer, s_battery_image);
	
	s_battery_level_layer = text_layer_create(GRect(20, 50, 94, 20));
	text_layer_set_background_color(s_battery_level_layer, GColorBlack);
	text_layer_set_text_color(s_battery_level_layer, GColorWhite);
	text_layer_set_text(s_battery_level_layer, "100%");
	
	s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LANE_10));
	text_layer_set_font(s_battery_level_layer, s_battery_font);
	
	text_layer_set_text_alignment(s_battery_level_layer, GTextAlignmentRight);
	
	
	
	
	//Add the Layers as children of the Window layer
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_image_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_level_layer));
  
	//Update the time when the window is loaded
	update_time();
}

static void main_window_unload(Window *window) {
	
	//Unload the custom fonts
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	fonts_unload_custom_font(s_battery_font);
	
	//Destroy the images
	gbitmap_destroy(s_battery_image);
	
	//Destroy the Layers
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	bitmap_layer_destroy(s_battery_image_layer);
	text_layer_destroy(s_battery_level_layer);
	
}

//Handler for every time a time unit changes (every minute)
static void minute_update(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
}

static void date_update(struct tm *tick_time, TimeUnits units_changed) {
	update_date();
}
  
static void init() {
	//Create the main Window layer
	s_main_window = window_create();

	//Setup handlers for the window when it is loaded or unloaded
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	//Show the Window on the watch (animated=true)
	window_stack_push(s_main_window, true);
  
	//Set a TickTimerService so it updates every minute
	tick_timer_service_subscribe(MINUTE_UNIT, minute_update);
	
	//Set another TickTimerService so it updates every day
	tick_timer_service_subscribe(DAY_UNIT, date_update);
}

static void deinit() {
	// Destroy Window
	window_destroy(s_main_window);

}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
