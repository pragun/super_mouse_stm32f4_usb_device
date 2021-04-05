## USB HID Device using STM32F4 
This repository holds the USB Device side of firmware necessary for the "Super Mouse" usb converter. It uses the ST32CubeMX generated HAL libraries for initialization and accessing USB and other peripherals.

## Folder structure
``
-- circular_buffer: used by UART DMA for RX & TX, SPI for RX, and USB HID for queuing HID reports
-- flash_key_value_tree: allows using some of the flash sectors as a (key, value). Used to store configuration values 
-- Core: autogen by the STM32CubeMX, contains main.cpp
-- Drivers: autogenerated, houses CMSIS & ST's HAL Libraries
-- Middlewares: autogenerated
-- Mouse: The core library, contains the event pipeline for generating HID reports and initial loading of configuration values from the flash
-- rpc: Thin RPC wrapper to allow RPC calls from the computer
-- USB_DEVICE: autogenerated, modified heavily to support multiple HID report IDs, also interfaces with the RPC layer
-- utils: Utility headers that did not have anywhere else to go

## Birds-eye view of how things connect
1. The USB Device (powered by the firmware in this repo), connects to the computer. It receives mouse state change data over SPI from the USB Host Device. 
2. The *mouse* layer processes mouse state changes, maintains a FSM of the current mouse state, and generates HID reports. It needs to look at configuration data, which is read into the *mouse* layer at device startup by the *flash_key_value_tree*. This is one time bootup a O(n) operation, n being the number of configuration entries. And, its better than looking at a configuration entry as needed, which would be a O(log n) operation per lookup. 
3. For each HID GetReport, the report data is passed onto *rpc* which calls update/add operations on the *flash_key_value_tree*. I made a simple terminal utility for Windows to write/edit flash key,values. [https://github.com/pragun/super_mouse_utilities](super_mouse_utilities)

## References
1. [Doc for the flash_key_value_tree](flash_key_value_tree/flash_readme.md) 
2. [Doc for circular_buffer](circular_buffer/cbuffer_readme.md)
3. [Repo for the USB Host Interface](https://github.com/pragun/super_mouse_stm32f4_usb_host)