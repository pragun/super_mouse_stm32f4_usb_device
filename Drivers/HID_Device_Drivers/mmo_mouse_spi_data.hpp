/*
 * hid_device_drivers.h
 *
 *  Created on: Jan 4, 2020
 *      Author: Pragun Goyal
 *      Original file: mmo_mouse_hid_driver.hpp, in the USB-HOST Program
 */


#ifndef HID_DEVICE_DRIVERS_MMO_MOUSE_SPI_DATA_HPP_
#define HID_DEVICE_DRIVERS_MMO_MOUSE_SPI_DATA_HPP_

#pragma pack(1)
typedef struct
{
	int16_t dx;
	int16_t dy;
	int8_t dz;
	uint32_t buttons;
}
MMO_Mouse_State_TypeDef;

#endif /* HID_DEVICE_DRIVERS_MMO_MOUSE_SPI_DATA_HPP_ */
