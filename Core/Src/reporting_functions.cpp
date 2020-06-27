/*
 * mouse_event_handler.cpp
 *
 *  Created on: Jun 13, 2020
 *      Author: Pragun Goyal
 *
 */
#include "circular_buffers.hpp"
#include "usb_device.h"
#include "usbd_hid.h"
#include "mouse_event_handler.hpp"
#include "reporting_functions.h"

void MouseEventHandler::report_altered_mouse_movement(void* report_ptr, void* parameters){
	altered_mouse_movement_params *params = (altered_mouse_movement_params*) parameters;
	Mouse_HID_Report_TypeDef *report = create_or_retreive_default_mouse_hid_report((Mouse_HID_Report_TypeDef *) report_ptr);
	if (report != nullptr){
		report->mouse_x = accumulated_mouse_del_x * params->x_factor;
		report->mouse_y = accumulated_mouse_del_y * params->y_factor;
		report->scroll_x = accumulated_scroll_y * params->z_factor;
		report->scroll_y = 0;

		accumulated_mouse_del_x = 0;
		accumulated_mouse_del_y = 0;
		accumulated_scroll_y = 0;
	}
}

void MouseEventHandler::report_absolute_mouse(void* report, void* parameters){

}

void MouseEventHandler::report_keyboard_key_press_release(void* report, void* parameters){

}

void MouseEventHandler::report_modulo_movement_as_keys_press_release(void* report, void* parameters){

}


