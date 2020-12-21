/*
 * mouse_event_handler.cpp
 *
 *	Edited on: Dec 12,2020
 *  Created on: Jun 13, 2020
 *      Author: Pragun Goyal
 *
 */
#include "usb_device.h"
#include "usbd_hid.h"
#include "mouse_event_handler.hpp"
#include "circular_buffers.hpp"
#include "default_config.h"

// All these are in milliseconds
// For all of these  read  "Since the time the KeyWasFirstPressed
uint16_t single_click_max_interval = 50; 	// .... the user must release the key within this interval to register a single click
uint16_t long_press_min_interval = 200;  	// .... the user must release the key after this interval to register a long_press
uint16_t long_press_max_interval = 2000;	// .... the user must release the key within this interval to register a long_press
uint16_t movement_event_max_start_interval = 100; 	// .. the user must start moving the mouse to start a movement event chain
HIDContinuousBlockCircularBuffer hid_report_buf;
// The KeyPadHandler is called by SPI Mouse RX when a KeyPad change is seen

template<uint8_t idx>
constexpr MouseEventHandler::Rprting_Fptr MouseEventHandler::get_fptr_from_idx(){
	return &MouseEventHandler::Reporting_Function<static_cast<ReportingFunctionEnum>(idx)>;
}

template <size_t... Indices>
constexpr std::array<MouseEventHandler::Rprting_Fptr, MouseEventHandler::NUM_REPORTING_FUNCS>
MouseEventHandler::func_idx_helper(std::index_sequence<Indices...>) {
    return { get_fptr_from_idx<Indices>()... };
}

constexpr std::array<MouseEventHandler::Rprting_Fptr, MouseEventHandler::NUM_REPORTING_FUNCS>
MouseEventHandler::func_idx_builder() {
    return func_idx_helper(
        // make the sequence type sequence<0, 1, 2, ..., N-1>
        std::make_index_sequence<NUM_REPORTING_FUNCS>{}
        );
}


void MouseEventHandler::update_state(int16_t dx, int16_t dy, int8_t dz, uint32_t button_state){
	accumulated_mouse_del_x += dx;
	accumulated_mouse_del_y += dy;
	accumulated_scroll_y += dz;

	current_primary_button_state = button_state & 0x0F; //this captures the first three buttons
	current_primary_button_state |= (button_state & 0x20) >> 2;
	current_keypad_state = (button_state >> 8) & 0x0FFF;
}

Mouse_HID_Report_TypeDef* MouseEventHandler::create_or_retreive_default_mouse_hid_report(){
	Mouse_HID_Report_TypeDef*& report = mouse_hid_report;
	if(report == nullptr){
		report = (Mouse_HID_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Mouse_HID_Report_TypeDef));
		if (report != nullptr){
			report->report_id = 0x01;

			//As the button state is always reported as it is
			//in other words, if there is a mouse_hid_report, it has the button state in it
			report->buttons = current_primary_button_state;
			previous_primary_button_state = current_primary_button_state;

			report->mouse_x = 0;
			report->mouse_y = 0;
			report->scroll_y = 0;
			report->scroll_y = 0;
		}
	}
	return report;
};


Mouse_HID_Report_TypeDef* MouseEventHandler::report_mouse_movement(Mouse_HID_Report_TypeDef* report){
	report = create_or_retreive_default_mouse_hid_report();
	if(report != nullptr){
		report->mouse_x = accumulated_mouse_del_x;
		report->mouse_y = accumulated_mouse_del_y;
		report->scroll_y = accumulated_scroll_y;
		report->scroll_x = 0;

		accumulated_mouse_del_x = 0;
		accumulated_mouse_del_y = 0;
		accumulated_scroll_y = 0;
	}
	return report;
}


inline Mouse_HID_Report_TypeDef* MouseEventHandler::report_mouse_button_state(Mouse_HID_Report_TypeDef* report){
	// No need to do anything else, if a mouse hid report is created
	// it already includes button state
	return create_or_retreive_default_mouse_hid_report();
}

void MouseEventHandler::register_config_entry(const uint32_t key, const uint8_t size, const uint8_t* data){
	uint8_t application_id = key & (0xFF << 8);
	uint8_t keypad_key =  key & (0xFF);
	if ((application_id <NUM_APPLICATIONS_KEYPAD) && (keypad_key < NUM_KEYS_KEYPAD)){
		event_handler_table[application_id][keypad_key] = (Keypad_Event_Table*) data;
		if(application_id == 0){
			for(uint8_t i = 0; i<NUM_APPLICATIONS_KEYPAD; i++){
				event_handler_table[i][keypad_key] = (Keypad_Event_Table*) data;
			}
		}
	}
}

void MouseEventHandler::dispatch_application_event_type(uint8_t event_type){
	uint8_t key_index = tracking_pressed_key;

	if (current_primary_button_state & (0x01 << (RING_BUTTON - 1))){
		key_index = RING_BUTTON_KEYPAD_IDX;
	}

	else if (current_primary_button_state & (0x01 << (SCROLL_BUTTON - 1))){
		key_index = SCROLL_BUTTON_KEYPAD_IDX;
	}

	else{
		key_index = tracking_pressed_key;
	}

	Keypad_Event_Table* event_table = event_handler_table[current_application_id][key_index];
	uint8_t event_table_entry_offset = event_table->entry_offsets[event_type];

	Keypad_Event_Table_Entry_Typedef* event_table_entry = reinterpret_cast<Keypad_Event_Table_Entry_Typedef*>(&(event_table->payload[event_table_entry_offset]));
	Rprting_Fptr reporting_func = reporting_function_table[event_table_entry->reporting_function_index];
	return (this->*reporting_func)(event_table_entry->parameters);
}


void MouseEventHandler::hid_poll_interval_timer_callback(){
	mouse_hid_report = nullptr;
	if (previous_primary_button_state != current_primary_button_state){
		//If there is a change in the button_state, the following will make sure that a Mouse HID report is included
		mouse_hid_report = report_mouse_button_state(mouse_hid_report);
	}

	if (current_keypad_state != previous_keypad_state){ //There's been a change on the keypad
		if(current_keypad_state == 0){
			if (modifier_keys_state != 0){
				release_all_keys();
			}
		}
		if ((current_keypad_state == 0) && (num_keypad_movements_events_generated == 0)){
			// The pressed key has been lifted, and no movement significant
			// enough to report while the key was down was observed
			// This means, that it is fine to lookup and report the key_up_event
			// corresponding to how long was the key pressedReportingEventTypes

			uint32_t a = time_elapsed_ms();

			printf("Time:%d\n",a);
			stop_timer();
			tracking_pressed_key = 0; // No longer tracking a key_press

			if((not short_press_duration_passed)){
				// release short_press_event for the key
				dispatch_application_event_type(ReportingEventTypes::SHORT_PRESS_UP);
			}
			if((long_press_min_duration_elapsed) && (not long_press_max_duration_elapsed)){
				// release long_press_event for the key
				dispatch_application_event_type(ReportingEventTypes::LONG_PRESS_UP);
			}
		}
		else if (previous_keypad_state == 0){ // A new key has been pressed
			// Report all un-reported mouse delx,dely,delz
			report_mouse_movement(mouse_hid_report);
			//Start KeyPress Timer
			start_timer();
			tracking_pressed_key = __builtin_ffs (current_keypad_state);

			// in case there is a key_down_event for this key
			// release it
			dispatch_application_event_type(ReportingEventTypes::KEY_DOWN);

			printf("Key Pressed Index:%d\n",tracking_pressed_key);

		}
		else{ //This means that the pressed key has been shifted to a new key
			//This should be a rare event and is ignored for now
		}
		previous_keypad_state = current_keypad_state;
	}

	if ((accumulated_mouse_del_x != 0)||(accumulated_mouse_del_y != 0)||(accumulated_scroll_y !=0)){ //Some movement is been accumulated on the mouse
		/* if (tracking_pressed_key == 0){ //No keypad key is pressed, this means that the movement can be reported as a regular mouse HID report
			report_mouse_movement(mouse_hid_report);
		}
		else{ */
			dispatch_application_event_type(ReportingEventTypes::MOUSE_MOVEMENT);
		//}
	}

	mouse_hid_report = nullptr;

	/*
	if(USB_HID_Ready_To_TX_Next_Report(&hUsbDeviceFS)){
		auto [report, len] = hid_report_buf.transfer_out_next_report();
		if (len > 0){
			Assuming_Endpoint_Ready_Send_HID_Report(&hUsbDeviceFS, report, len);
		}
	}*/

	USB_HID_Send_Next_Report(&hUsbDeviceFS);
}

void MouseEventHandler::set_application_id(uint8_t app_id){
	current_application_id = app_id;
}

MouseEventHandler::MouseEventHandler(void (*stop_timer)(), void (*start_timer)(), uint32_t (*time_elapsed_ms)()):
				start_timer{start_timer},
				stop_timer{stop_timer},
				time_elapsed_ms{time_elapsed_ms},
				reporting_function_table{MouseEventHandler::func_idx_builder()},
				event_handler_table{}{
					fill_default_cfg();
					for(uint8_t i = 0; i <NUM_APPLICATIONS_KEYPAD ; i++){
						for(uint8_t j = 0; j<NUM_KEYS_KEYPAD ; j++){
							event_handler_table[i][j] = (Keypad_Event_Table*) default_cfg;
						}
					}

};

