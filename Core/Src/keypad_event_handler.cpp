/*
 * keypad_event_handler.cpp
 *
 *  Created on: Jun 13, 2020
 *      Author: Pragun Goyal
 *
 */

// All these are in milliseconds
// For all of these  read  "Since the time the KeyWasFirstPressed
uint16_t single_click_max_interval = 50; 	// .... the user must release the key within this interval to register a single click
uint16_t long_press_min_interval = 100;  	// .... the user must release the key after this interval to register a long_press
uint16_t long_press_max_interval = 2000;	// .... the user must release the key within this interval to register a long_press
uint16_t movement_event_max_start_interval = 100; 	// .. the user must start moving the mouse to start a movement event chain

// The KeyPadHandler is called by SPI Mouse RX when a KeyPad change is seen


class KeyPadEventHandler {
private:
	// State Flags
	bool movement_event_generated = False;
	uint16_t movement_tracking_on_key_mask = 0x00; //This is zero when a single KeyPad event is still being tracked

public:
	void HandleKeyPadChange();


};


