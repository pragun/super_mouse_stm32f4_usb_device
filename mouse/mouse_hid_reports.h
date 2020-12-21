/*
 * mouse_hid_reports.h
 *
 *  Created on: Jun 24, 2020
 *      Author: Pragun
 */

#ifndef INC_MOUSE_HID_REPORTS_H_
#define INC_MOUSE_HID_REPORTS_H_


typedef struct
{
    uint8_t report_id = 1;
	uint8_t buttons : 3;
    int16_t mouse_x;
    int16_t mouse_y;
    int8_t scroll_y;
    int8_t scroll_x;
}
Mouse_HID_Report_TypeDef;

typedef struct
{
    uint8_t report_id = 3;
	uint8_t buttons : 1;
    uint16_t mouse_x;
    uint16_t mouse_y;
}
Absolute_Mouse_HID_Report_TypeDef;

typedef struct
{
	uint8_t report_id = 2;
	uint8_t modifier_keys;
	uint8_t key_press[3];
}
Keyboard_Report_TypeDef;

#endif /* INC_MOUSE_HID_REPORTS_H_ */
