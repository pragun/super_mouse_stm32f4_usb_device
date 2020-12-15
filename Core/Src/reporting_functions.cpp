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
#include "circular_buffers.hpp"

extern HIDContinuousBlockCircularBuffer hid_report_buf;

#define DEF_Reporting_FUNC(Y) template <>\
auto MouseEventHandler::Reporting_Function<ReportingFunctionEnum::Y>

#define PARAMS_STRUCT(X) Reporting_Func_Params_Typedef<ReportingFunctionEnum::X>


DEF_Reporting_FUNC(NO_REPORT)(uint8_t* params)->void {

}

DEF_Reporting_FUNC(ALTERED_MOUSE_MOVEMENT)(uint8_t* params)->void {
	PARAMS_STRUCT(ALTERED_MOUSE_MOVEMENT) *parameters = (PARAMS_STRUCT(ALTERED_MOUSE_MOVEMENT)*) params;
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

DEF_Reporting_FUNC(ABSOLUTE_MOUSE_POSITIION)(uint8_t* params)->void {
	PARAMS_STRUCT(ABSOLUTE_MOUSE_POSITIION) *parameters = (PARAMS_STRUCT(ABSOLUTE_MOUSE_POSITIION)*) params;

	Absolute_Mouse_HID_Report_TypeDef *absolute_mouse_hid_report = (Absolute_Mouse_HID_Report_TypeDef*) hid_report_buf.allocate_space_for_report((uint16_t) sizeof(Absolute_Mouse_HID_Report_TypeDef));
	if (absolute_mouse_hid_report != nullptr){
		absolute_mouse_hid_report->report_id = 0x03;
		absolute_mouse_hid_report->buttons = parameters->button;
		absolute_mouse_hid_report->mouse_x = parameters->x;
		absolute_mouse_hid_report->mouse_y = parameters->y;
		previous_keypad_state = current_keypad_state;
	}
}

DEF_Reporting_FUNC(KEYBOARD_PRESS_RELEASE)(uint8_t* params)->void {
	PARAMS_STRUCT(KEYBOARD_PRESS_RELEASE) *parameters = (PARAMS_STRUCT(KEYBOARD_PRESS_RELEASE)*) params;

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

DEF_Reporting_FUNC(MOTION_MOD_KEY_PRESS_RELEASE)(uint8_t* params)->void {
	PARAMS_STRUCT(MOTION_MOD_KEY_PRESS_RELEASE) *parameters = (PARAMS_STRUCT(MOTION_MOD_KEY_PRESS_RELEASE)*) params;

	int8_t div_x = (accumulated_mouse_del_x / parameters->x_divisor);
	accumulated_mouse_del_x = (accumulated_mouse_del_x % parameters->x_divisor);

	int8_t div_y = (accumulated_mouse_del_y / parameters->y_divisor);
	accumulated_mouse_del_y = (accumulated_mouse_del_y % parameters->y_divisor);

	int8_t div_z = (accumulated_scroll_y / parameters->z_divisor);
	accumulated_scroll_y = (accumulated_scroll_y % parameters->z_divisor);

	#define keyboard_press_release_func Reporting_Function<ReportingFunctionEnum::KEYBOARD_PRESS_RELEASE>
	#define keyboard_press_release_for(V) \
	if(div_ ## V > 0){\
		keyboard_press_release_func((uint8_t*) &parameters->V ##_movement_keys[1]);\
	}\
	if(div_ ##V < 0){\
		keyboard_press_release_func((uint8_t*) &parameters->V ##_movement_keys[0]);\
	}\

	keyboard_press_release_for(x);
	keyboard_press_release_for(y);
	keyboard_press_release_for(z);

}

