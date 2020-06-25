/*
 * keypad_event_handler.cpp
 *
 *  Created on: Jun 13, 2020
 *      Author: Pragun Goyal
 *
 */
#include "circular_buffers.hpp"
#include "usb_device.h"
#include "usbd_hid.h"
#include "mouse_event_handler.hpp"

// All these are in milliseconds
// For all of these  read  "Since the time the KeyWasFirstPressed
uint16_t single_click_max_interval = 50; 	// .... the user must release the key within this interval to register a single click
uint16_t long_press_min_interval = 100;  	// .... the user must release the key after this interval to register a long_press
uint16_t long_press_max_interval = 2000;	// .... the user must release the key within this interval to register a long_press
uint16_t movement_event_max_start_interval = 100; 	// .. the user must start moving the mouse to start a movement event chain
HIDContinuousBlockCircularBuffer hid_report_buf;

// The KeyPadHandler is called by SPI Mouse RX when a KeyPad change is seen

void MouseEventHandler::update_state(int16_t dx, int16_t dy, int8_t dz, uint32_t button_state){
	accumulated_mouse_del_x += dx;
	accumulated_mouse_del_y += dy;
	accumulated_scroll_y += dz;

	current_primary_button_state = button_state & 0x0F;
	current_keypad_state = (button_state >> 8) & 0x0FFF;
}

/*
absolute_mouse_hid_report = (Absolute_Mouse_HID_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Absolute_Mouse_HID_Report_TypeDef));
if (absolute_mouse_hid_report != nullptr){
	absolute_mouse_hid_report->report_id = 0x03;
	absolute_mouse_hid_report->buttons = 0x01;
	absolute_mouse_hid_report->mouse_x = 5000;
	absolute_mouse_hid_report->mouse_y = 5000;
	previous_keypad_button_state = current_keypad_button_state;
}
*/

Mouse_HID_Report_TypeDef* MouseEventHandler::create_or_retreive_mouse_hid_report(){
	if(mouse_hid_report == nullptr){
		mouse_hid_report = (Mouse_HID_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Mouse_HID_Report_TypeDef));
		if (mouse_hid_report != nullptr){
			mouse_hid_report->report_id = 0x01;

			//As the button state is always reported as it is
			//in other words, if there is a mouse_hid_report, it has the button state in it
			mouse_hid_report->buttons = current_primary_button_state;
			previous_primary_button_state = current_primary_button_state;

			mouse_hid_report->mouse_x = 0;
			mouse_hid_report->mouse_y = 0;
			mouse_hid_report->scroll_y = 0;
			mouse_hid_report->scroll_y = 0;
		}
	}
	return mouse_hid_report;
};

void MouseEventHandler::report_mouse_movement(){
	if (create_or_retreive_mouse_hid_report() != nullptr){
		mouse_hid_report->mouse_x = accumulated_mouse_del_x;
		mouse_hid_report->mouse_y = accumulated_mouse_del_y;
		mouse_hid_report->scroll_x = accumulated_scroll_y;
		mouse_hid_report->scroll_y = 0;

		accumulated_mouse_del_x = 0;
		accumulated_mouse_del_y = 0;
		accumulated_scroll_y = 0;
	}
}

void MouseEventHandler::report_mouse_button_state(){
	if (create_or_retreive_mouse_hid_report() != nullptr){
		// No need to do anything else, if a mouse hid report is created
		// it already includes button state
	}
}

void MouseEventHandler::hid_poll_interval_timer_callback(){
	if (previous_primary_button_state != current_primary_button_state){
		//If there is a change in the button_state, the following will make sure that a Mouse HID report is included
		report_mouse_button_state();
	}

	if (current_keypad_state != previous_keypad_state){ //There's been a change on the keypad
		if ((current_keypad_state == 0) && (num_keypad_movements_events_generated == 0)){ // The pressed key has been lifted. This could also be from
			uint32_t a = time_elapsed_ms();
			printf("Time:%d\n",a);
			stop_timer();
			tracking_pressed_key = 0; // No longer tracking a key_press

			if((not short_press_duration_passed)){
				// release short_press_event for the key
			}
			if((long_press_min_duration_elapsed) && (not long_press_max_duration_elapsed)){
				// release long_press_event for the key
			}
		}
		else if (previous_keypad_state == 0){ // A new key has been pressed
			// Report all un-reported mouse delx,dely,delz
			report_mouse_movement();
			start_timer();
			tracking_pressed_key = __builtin_ffs (current_keypad_state);
			printf("Key Pressed Index:%d\n",tracking_pressed_key);
			//Start KeyPress Timer
		}
		else{ //This means that the pressed key has been shifted to a new key
			//This should be a rare event and is ignored for now
		}
		previous_keypad_state = current_keypad_state;
	}

	if ((accumulated_mouse_del_x != 0)||(accumulated_mouse_del_y != 0)||(accumulated_scroll_y !=0)){ //Some movement is been accumulated on the mouse
		if (tracking_pressed_key == 0){ //No keypad key is pressed, this means that the movement can be reported as a regular mouse HID report
			report_mouse_movement();
		}
	}

	mouse_hid_report = nullptr;
	USB_HID_Send_Next_Report(&hUsbDeviceFS);
}
