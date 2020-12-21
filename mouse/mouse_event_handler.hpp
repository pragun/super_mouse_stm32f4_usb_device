#ifndef __MOUSE_EVENT_HANDLER__
#define __MOUSE_EVENT_HANDLER__

#include "mouse_hid_reports.h"
#include "reporting_functions.h"

#define SCROLL_BUTTON 3
#define RING_BUTTON 4

class MouseEventHandler {
public:
	void (*start_timer)();
	void (*stop_timer)();
	uint32_t (*time_elapsed_ms)();

	constexpr static uint8_t NUM_REPORTING_FUNCS = ReportingFunctionEnumList.size();

	MouseEventHandler(void (*stop_timer)(), void (*start_timer)(), uint32_t (*time_elapsed_ms)());

	void update_state(int16_t dx, int16_t dy, int8_t dz, uint32_t button_state);

	void hid_poll_interval_timer_callback();

	void register_config_entry(const uint32_t key, const uint8_t size, const uint8_t* data);

	void set_application_id(uint8_t app_id);

private:
	// State Flags
	bool movement_event_generated = false;
	uint16_t num_keypad_movements_events_generated = 0;

	uint16_t tracking_pressed_key = 0; //This is non zero (and equal to the index of the key that was pressed.
									 //It is set to zero after the key is released

	uint16_t current_application_id = 0;

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
	uint8_t modifier_keys_state = 0;

	Mouse_HID_Report_TypeDef* create_or_retreive_default_mouse_hid_report();
	Mouse_HID_Report_TypeDef* report_mouse_movement(Mouse_HID_Report_TypeDef*);
	Mouse_HID_Report_TypeDef* report_mouse_button_state(Mouse_HID_Report_TypeDef*);

	void release_all_keys();

	void dispatch_application_event_type(uint8_t event_type);

	typedef  void (MouseEventHandler::*Rprting_Fptr)(uint8_t *);  // Please do this!
	std::array<Rprting_Fptr,NUM_REPORTING_FUNCS> reporting_function_table;

	Keypad_Event_Table* event_handler_table[NUM_APPLICATIONS_KEYPAD][NUM_KEYS_KEYPAD];

	template <ReportingFunctionEnum>
	void Reporting_Function(uint8_t* params);

	static constexpr std::array<MouseEventHandler::Rprting_Fptr, MouseEventHandler::NUM_REPORTING_FUNCS>
	func_idx_builder();

	template <size_t... Indices>
	static constexpr std::array<MouseEventHandler::Rprting_Fptr, MouseEventHandler::NUM_REPORTING_FUNCS>
	func_idx_helper(std::index_sequence<Indices...>);

	template<uint8_t idx>
	static constexpr Rprting_Fptr get_fptr_from_idx();
};

#endif
