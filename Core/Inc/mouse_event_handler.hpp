
#include "mouse_hid_reports.h"
#include "reporting_functions.h"

class MouseEventHandler {
private:
	// State Flags
	bool movement_event_generated = false;
	uint16_t num_keypad_movements_events_generated = 0;

	uint16_t tracking_pressed_key = 0; //This is non zero (and equal to the index of the key that was pressed.
									 //It is set to zero after the key is released

	bool short_press_duration_passed = false;
	bool long_press_min_duration_elapsed = false;
	bool long_press_max_duration_elapsed = false;

	int16_t accumulated_mouse_del_x = 0;
	int16_t accumulated_mouse_del_y = 0;
	int16_t accumulated_scroll_y = 0;

	uint8_t current_primary_button_state = 0;
	uint8_t previous_primary_button_state = 0;

	uint16_t current_keypad_state = 0;
	uint16_t previous_keypad_state = 0;

	Mouse_HID_Report_TypeDef* mouse_hid_report;

	Mouse_HID_Report_TypeDef* create_or_retreive_default_mouse_hid_report();
	Mouse_HID_Report_TypeDef* report_mouse_movement(Mouse_HID_Report_TypeDef*);
	Mouse_HID_Report_TypeDef* report_mouse_button_state(Mouse_HID_Report_TypeDef*);

	void report_altered_mouse_movement(void* parameters);
	void report_absolute_mouse_position(void* parameters);
	void report_keyboard_key_press_release(void* parameters);
	void report_movement_mod_as_keys_press_release(void* parameters);
	void dont_report_anything(void* b);

	void create_reporting_function_lookup_table();

	typedef  void (MouseEventHandler::*reporting_function_ptr)(void *);  // Please do this!
	reporting_function_ptr reporting_function_lookup_table[64];

	Key_Report_Event_Config_Typedef* key_reporting_lookup[NUM_APPLICATIONS_KEYPAD][NUM_EVENT_TYPES_KEYPAD][NUM_KEYS_KEYPAD];

public:
	void (*start_timer)();
	void (*stop_timer)();
	uint32_t (*time_elapsed_ms)();

	MouseEventHandler(void (*stop_timer)(), void (*start_timer)(), uint32_t (*time_elapsed_ms)()):
		start_timer(start_timer), stop_timer(stop_timer), time_elapsed_ms(time_elapsed_ms){};

	void update_state(int16_t dx, int16_t dy, int8_t dz, uint32_t button_state);
	void hid_poll_interval_timer_callback();
};
