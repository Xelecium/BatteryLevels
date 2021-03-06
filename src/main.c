#include <pebble.h>
	
//Key for getting data from the phone 
//about the battery & its charge status
#define PHONE_BATTERY_DATA_KEY 3
#define PHONE_CHARGE_STATE_KEY 7
  
//Key for pinging the phone to update information
#define SIGNAL_TO_PHONE_KEY 11
	
//Main Window element
static Window *s_main_window;

//Elements for the time
static TextLayer *s_time_layer;
static GFont s_time_font;

//Elements for the date
static TextLayer *s_date_layer;
static GFont s_date_font;

//Elements for the Pebble battery level
static BitmapLayer *s_pebble_image_layer;
static GBitmap *s_pebble_image;
static TextLayer *s_pebble_battery_layer;
static GFont *s_pebble_font;

static BitmapLayer *s_pebble_plug_layer;
static GBitmap *s_pebble_plug;
static BitmapLayer *s_pebble_charge_layer;
static GBitmap *s_pebble_charge;

//Elements for the Phone battery level
static BitmapLayer *s_phone_image_layer;
static GBitmap *s_phone_image;
static TextLayer *s_phone_battery_layer;
static GFont *s_phone_font;
static BitmapLayer *s_phone_plug_layer;
static GBitmap *s_phone_plug_image;
static BitmapLayer *s_phone_disconnected_layer;
static GBitmap *s_phone_disconnected_image;


//===================PING PHONE
static void ping_phone(int key, int value) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_int(iter, key, &value, sizeof(int), true);
	app_message_outbox_send();
}

//===================UPDATE TIME
static void update_time() {
	//Get a tm structure
	time_t temp = time(NULL); 
	struct tm *now = localtime(&temp);

	//Character array (string) to hold the time, with placeholder text
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
	APP_LOG(APP_LOG_LEVEL_INFO, "Updated time to %s", timeString);
	ping_phone(SIGNAL_TO_PHONE_KEY, 1);	
}

//====================UPDATE DATE
static void update_date() {
	//Get a tm structure
	time_t temp = time(NULL);
	struct tm *now = localtime(&temp);
	
	//Character array (string) to hold the date, with a placeholder
	static char dateString[] = "25 September";
	
	//Save the value of the date item
	strftime(dateString, sizeof("25 September"), "%d %B", now);
	
	//Set the date on the date TextLayer
	text_layer_set_text(s_date_layer, dateString);
	APP_LOG(APP_LOG_LEVEL_INFO, "Updated date to %s", dateString);
}

//======================PHONE STATUS
static void phone_battery(int value) {
	static char phoneValue[] = "100";
	if (value >= 100) {
		snprintf(phoneValue, sizeof("100"), "100");
	}
	else {
		snprintf(phoneValue, sizeof("100%"), "%i%%", value);
	}
	text_layer_set_text(s_phone_battery_layer, phoneValue);
	APP_LOG(APP_LOG_LEVEL_INFO, "Phone Battery Level Update: %i%%", value);
}

static void phone_plugged(int value) {
	if (value == 0) {
		//Phone is running on battery
		bitmap_layer_set_bitmap(s_phone_plug_layer, NULL);
	}
	else {
		//Some other state (charging via USB or AC)
		bitmap_layer_set_bitmap(s_phone_plug_layer, s_phone_plug_image);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Phone Charge State: %i", value);
}

//=====================PEBBLE STATUS
static void battery_handler(BatteryChargeState charge_state) {
	//Temporary buffer value
	static char batteryValue[] = "100";
	 
	//Display plug if the pebble is plugged in
	if (charge_state.is_plugged) {
		bitmap_layer_set_bitmap(s_pebble_plug_layer, s_pebble_plug);
	}
	else {
		bitmap_layer_set_bitmap(s_pebble_plug_layer, NULL);
	}
	
	//Behavior depends on if the Pebble is currently charging or not
	if (charge_state.is_charging) {
		//Empty the TextLayer, set the BitmapLayer
		text_layer_set_text(s_pebble_battery_layer, "");
		bitmap_layer_set_bitmap(s_pebble_charge_layer, s_pebble_charge);
	} 
	else {
		//Empty the BitmapLayer, set the TextLayer
		bitmap_layer_set_bitmap(s_pebble_charge_layer, NULL);
		if (charge_state.charge_percent >= 100) {
			snprintf(batteryValue, sizeof(batteryValue), "100");
		}
		else {
			snprintf(batteryValue, sizeof(batteryValue), "%d%%", charge_state.charge_percent);
		}
		text_layer_set_text(s_pebble_battery_layer, batteryValue);
	}
	APP_LOG(APP_LOG_LEVEL_INFO, "Pebble Battery Status: %i%%", charge_state.charge_percent);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Pebble Charge State: %i", charge_state.is_charging);
}

//===================BLUETOOTH HANDLER
static void bluetooth_handler(bool connected) {
	if (connected) {
		//Send a signal to the phone to send battery info
		bitmap_layer_set_bitmap(s_phone_disconnected_layer, NULL);
		ping_phone(SIGNAL_TO_PHONE_KEY, 1);	
	} else {
		//We can't get info about the phone, so adjust related images
		text_layer_set_text(s_phone_battery_layer, "");
		bitmap_layer_set_bitmap(s_phone_disconnected_layer, s_phone_disconnected_image);
		bitmap_layer_set_bitmap(s_phone_plug_layer, NULL);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "BlueTooth connection status: %i", connected);
}

//===================DATA RECEIVER HANDLER
static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *tuple = dict_read_first(iter);
	while (tuple) {
		switch (tuple->key) {
			case PHONE_BATTERY_DATA_KEY:
				phone_battery((int)tuple->value->int32);
				break;
			case PHONE_CHARGE_STATE_KEY:
				phone_plugged((int)tuple->value->int32);
				break;
			default: break;
		}
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Data: Value %i", (int)tuple->value->int32);
		tuple = dict_read_next(iter);
	}
}
//====================COMMUNICATION CALLBACKS
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
	text_layer_set_text(s_phone_battery_layer, "");
	bitmap_layer_set_bitmap(s_phone_plug_layer, NULL);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Sent ping to phone");
}

//======================WINDOW LOAD
static void main_window_load(Window *window) {
	//Set the background of the watchface to black
	window_set_background_color(window, GColorBlack);
	
	//===============================TIME
	s_time_layer = text_layer_create(GRect(0, 117, 144, 40));
	text_layer_set_background_color(s_time_layer, GColorBlack);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_text(s_time_layer, "");
	
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROUND_36));
	text_layer_set_font(s_time_layer, s_time_font);
	
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	//===============================DATE
	s_date_layer = text_layer_create(GRect(0, 10, 144, 30));
	text_layer_set_background_color(s_date_layer, GColorBlack);
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_text(s_date_layer, "");
	
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROUND_20));
	text_layer_set_font(s_date_layer, s_date_font);
	
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	//===============================PEBBLE BATTERY
	s_pebble_image_layer = bitmap_layer_create(GRect(6, 45, 70, 70));
	s_pebble_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_ICON);
	bitmap_layer_set_bitmap(s_pebble_image_layer, s_pebble_image);
	
	s_pebble_battery_layer = text_layer_create(GRect(26, 72, 30, 20));
	text_layer_set_background_color(s_pebble_battery_layer, GColorBlack);
	text_layer_set_text_color(s_pebble_battery_layer, GColorWhite);
	text_layer_set_text(s_pebble_battery_layer, "");
	
	s_pebble_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROUND_12));
	text_layer_set_font(s_pebble_battery_layer, s_pebble_font);
	
	text_layer_set_text_alignment(s_pebble_battery_layer, GTextAlignmentCenter);
	//Charging image
	s_pebble_charge_layer = bitmap_layer_create(GRect(30, 70, 20, 20));
	s_pebble_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE_ICON);
	bitmap_layer_set_bitmap(s_pebble_charge_layer, s_pebble_charge);
	
	//Plug image
	s_pebble_plug_layer = bitmap_layer_create(GRect(8, 84, 11, 43));
	s_pebble_plug = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PEBBLE_PLUG_ICON);
	bitmap_layer_set_bitmap(s_pebble_plug_layer, NULL);
	
	//=================================PHONE BATTERY
	s_phone_image_layer = bitmap_layer_create(GRect(70, 45, 70, 70));
	s_phone_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PHONE_ICON);
	bitmap_layer_set_bitmap(s_phone_image_layer, s_phone_image);
	
	s_phone_battery_layer = text_layer_create(GRect(90, 72, 30, 20));
	text_layer_set_background_color(s_phone_battery_layer, GColorBlack);
	text_layer_set_text_color(s_phone_battery_layer, GColorWhite);
	text_layer_set_text(s_phone_battery_layer, "");
	
	s_phone_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROUND_12));
	text_layer_set_font(s_phone_battery_layer, s_phone_font);
	
	text_layer_set_text_alignment(s_phone_battery_layer, GTextAlignmentCenter);
	
	//Plug image
	s_phone_plug_layer = bitmap_layer_create(GRect(102, 113, 32, 9));
	s_phone_plug_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PHONE_PLUG_ICON);
	bitmap_layer_set_bitmap(s_phone_plug_layer, NULL);
	
	//Disconnected image
	s_phone_disconnected_layer = bitmap_layer_create(GRect(95, 70, 20, 20));
	s_phone_disconnected_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED_ICON);
	bitmap_layer_set_bitmap(s_phone_disconnected_layer, s_phone_disconnected_image);
	
	//Ping the phone to send back phone battery and plugged state
	ping_phone(SIGNAL_TO_PHONE_KEY, 1);	
	
	//=================================CLEANUP
	//Add the Layers as children of the Window layer
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_pebble_image_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_pebble_battery_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_pebble_charge_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_pebble_plug_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_phone_image_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_phone_battery_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_phone_disconnected_layer));
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_phone_plug_layer));
	
	//Update the time when the window is loaded
	update_time();
	update_date();
	
	//Get the current battery level
  	battery_handler(battery_state_service_peek());
	//Get the BlueTooth connection status
	bluetooth_handler(bluetooth_connection_service_peek());
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Loaded all Window elements");
}

static void main_window_unload(Window *window) {
	//Unload the custom fonts
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	fonts_unload_custom_font(s_pebble_font);
	
	//Destroy the images
	gbitmap_destroy(s_pebble_image);
	gbitmap_destroy(s_pebble_charge);
	gbitmap_destroy(s_pebble_plug);
	gbitmap_destroy(s_phone_image);
	gbitmap_destroy(s_phone_disconnected_image);
	gbitmap_destroy(s_phone_plug_image);
	
	//Destroy the Layers
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	bitmap_layer_destroy(s_pebble_image_layer);
	text_layer_destroy(s_pebble_battery_layer);
	bitmap_layer_destroy(s_pebble_charge_layer);
	bitmap_layer_destroy(s_pebble_plug_layer);
	bitmap_layer_destroy(s_phone_image_layer);
	text_layer_destroy(s_phone_battery_layer);
	bitmap_layer_destroy(s_phone_disconnected_layer);
	bitmap_layer_destroy(s_phone_plug_layer);
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Unloaded all Window elements");
}

//Handler for every time a time unit changes (every minute)
static void time_update(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	
	//Utilizing the bitmask, if the date has changed, update the date as well
	if (units_changed & DAY_UNIT) {
		update_date();
	}
}

static void init() {
	//Create the main Window layer
	s_main_window = window_create();

	//Setup handlers for the window when it is loaded or unloaded
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	//Register AppMessage events
	app_message_register_inbox_received(in_received_handler);         
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

	//Set a TickTimerService so it updates every minute
	//Note: cannot set an additional TickTimerService for every day,
	//      so date will be updated along with it
	tick_timer_service_subscribe(MINUTE_UNIT, time_update);
	
	//Set the Battery State Service so we can get battery level
	battery_state_service_subscribe(battery_handler);
	
	//Set the BlueTooth Connection Service to see if it's connected to the phone
	bluetooth_connection_service_subscribe(bluetooth_handler);
	
	//Show the Window on the watch (animated=true)
	//NOTE: Should be called after everything else in init()
	window_stack_push(s_main_window, true);
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Initialized");
}

static void deinit() {
	// Destroy Window
	window_destroy(s_main_window);
	//Unsubscribe the handlers
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Deinitialized");
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}