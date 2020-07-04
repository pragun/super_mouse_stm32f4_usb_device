/*
 * mouse_event_handler.cpp
 *
 *  Created on: Jun 13, 2020
 *      Author: Pragun Goyal
 *
 */
#include "usb_device.h"
#include "usbd_hid.h"
#include "mouse_event_handler.hpp"
#include "../../circular_buffer/circular_buffers.hpp"

extern HIDContinuousBlockCircularBuffer hid_report_buf;

void MouseEventHandler::create_reporting_function_lookup_table(){
	reporting_function_lookup_table[ReportingFunctionIndex::NO_REPORT] = &MouseEventHandler::dont_report_anything;
	reporting_function_lookup_table[ReportingFunctionIndex::ALTERED_MOUSE_MOVEMENT] = &MouseEventHandler::report_altered_mouse_movement;
	reporting_function_lookup_table[ReportingFunctionIndex::ABSOLUTE_MOUSE_POSITIION] = &MouseEventHandler::report_absolute_mouse_position;
	reporting_function_lookup_table[ReportingFunctionIndex::KEYBOARD_PRESS_RELEASE] = &MouseEventHandler::report_keyboard_key_press_release;
	reporting_function_lookup_table[ReportingFunctionIndex::MOTION_MOD_KEY_PRESS_RELEASE] = &MouseEventHandler::report_movement_mod_as_keys_press_release;
}

void MouseEventHandler::dont_report_anything(void* b){

}

void MouseEventHandler::report_altered_mouse_movement(void* params){
	Altered_Mouse_Movement_Params_Typedef *parameters = (Altered_Mouse_Movement_Params_Typedef*) params;
	Mouse_HID_Report_TypeDef *report = create_or_retreive_default_mouse_hid_report();
	if (report != nullptr){
		report->mouse_x = accumulated_mouse_del_x * parameters->x_factor;
		report->mouse_y = accumulated_mouse_del_y * parameters->y_factor;
		report->scroll_x = accumulated_scroll_y * parameters->z_factor;
		report->scroll_y = 0;

		accumulated_mouse_del_x = 0;
		accumulated_mouse_del_y = 0;
		accumulated_scroll_y = 0;
	}
}

void MouseEventHandler::report_absolute_mouse_position(void* params){
	Absolute_Mouse_Params_Typedef *parameters = (Absolute_Mouse_Params_Typedef*) params;
	Absolute_Mouse_HID_Report_TypeDef *absolute_mouse_hid_report = (Absolute_Mouse_HID_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Absolute_Mouse_HID_Report_TypeDef));
	if (absolute_mouse_hid_report != nullptr){
		absolute_mouse_hid_report->report_id = 0x03;
		absolute_mouse_hid_report->buttons = parameters->button;
		absolute_mouse_hid_report->mouse_x = parameters->x;
		absolute_mouse_hid_report->mouse_y = parameters->y;
		previous_keypad_state = current_keypad_state;
	}
}

void MouseEventHandler::report_keyboard_key_press_release(void* params){
	Keyboard_Press_Release_Params_Typedef *parameters = (Keyboard_Press_Release_Params_Typedef*) params;

	//Press Report
	Keyboard_Report_TypeDef *keyboard_report1 = (Keyboard_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Keyboard_Report_TypeDef));
	if ( keyboard_report1 != nullptr){
		keyboard_report1->report_id = 0x02;
		keyboard_report1->modifier_keys = parameters->modifier_keys;
		for(uint8_t i = 0; i<3; i++){
			keyboard_report1->key_press[i] = parameters->keys[i];
		}
	}

	//Lift Key Report
	Keyboard_Report_TypeDef *keyboard_report2 = (Keyboard_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Keyboard_Report_TypeDef));
	if ( keyboard_report2 != nullptr){
		keyboard_report2->report_id = 0x02;
		keyboard_report2->modifier_keys = 0;
		for(uint8_t i = 0; i<3; i++){
			keyboard_report2->key_press[i] = 0;
		}
	}
}

void MouseEventHandler::report_movement_mod_as_keys_press_release(void* params){

}

