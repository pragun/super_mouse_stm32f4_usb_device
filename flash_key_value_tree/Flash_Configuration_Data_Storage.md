# Configuration Flash  Key Value Store [UCFKVS]

## Background
The UCFKVS allows one to permanently store, retrieve and edit key-value pairs on the STM324xx Flash memory. The keys have to be fixed size [4bytes, uint_32t in the current implementation]. The values can be variable sized.

The STM32F4xx does not have any EEPROM, which is ideal for saving user configuration data as it can be read and written practically infinite times. Flash Memory however is more tricky to write to. First it can endure a limited number of write cycles [10,000-100,000], second is in the way the STM32F4xx flash is organized.

For example, in RAM, it is perfectly ok to read the value stored at some address, lets say `[0x04000000]`:

```
uint32_t* a = reinterpret_cast<uint32_t>(0x04000000);
uint32_t b = *a;
//b holds the uint32_t value at the address pointed to by a
```
It is also perfectly ok to write the value stored at an address in RAM:
```
uint32_t* a = reinterpret_cast<uint32_t>(0x04000000);
*a = 5;
//set the memory location pointed at by a to 5 in uint32_t
```

1. We can not do this with the STM32F4xx Flash. While we can still read the value of a flash memory location exactly as we would if that memory location is in RAM. However, we have to use HAL_FlashWrite_xxx routines to write to an address on the Flash.

2. Using the HAL_FlashWrite_xxx routines, we can set the value of a bit from 1 to 0, but we can not flip a 0 stored in flash back to 1. The next point will explain how to flip bits back to 1.

3. The Flash on the STM334xx is organized in sectors. For example the STM32401CCUx has 256kb of flash organized in several sectors (as shown in the below). While we can not flip an individual bit on the flash back to 1 (or the bits of a byte, a half-word (2bytes) or a word(4bytes)), we can flip (or erase from here on) an entire Sector back to 1.


| Sector | Size | Memory Address Range|
| ----- | ------| --------------------|
|Sector 0 | 16kb | 0x0800 0000 - 0x0800 3FFF |
|Sector 1 | 16kb | 0x0800 4000 - 0x0800 7FFF |
|Sector 2 | 16kb | 0x0800 8000 - 0x0800 BFFF |
|Sector 3 | 16kb | 0x0800 C000 - 0x0800 FFFF |
|Sector 5 | 64kb | 0x0801 0000 - 0x0801 FFFF |
|Sector 6 | 128kb | 0x0802 0000 - 0x0805 FFFF |


>*Refer to the STM32F4xx datasheet, page 73, Embedded Flash Memory Interface for more details.
[STM32F4xx_Datasheet](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf).*

Flash memory in the STM32 is organized in sectors. While a single byte can be written to an erased sector, it is not possible to erase a single byte. A (minimum of) a full sector has to be erased in a single operation.

If one only needs to write somewhere in the range of [several to several thousand (a few kb)] worth of data, erasing a sector (which would mean erasing at-least 16kb of memory) might not be the most efficient way to do this.
1. Also consider that erasing a sector of flash can take several hundred ms to as close as a thousand ms (1s), take or give a few hundred.

2. Furthermore, flash memory is not designed to be written to indefinitely (an indefinite number of times) as RAM is. So, if we're going to write some (lets say 2-3kb) of data before we end up needing to edit it, it might be desirable to make the edits such that (almost) all the space on the sector ends up getting used before we use erase the sector.

The points 1 & 2 set the stage for introducing the UCFKVS:

##TL;DR
## Who/When would someone use this?
- If you are working on an STM32F4 (or related embedded MCU project, do your own research )
- Sf you need a way to persistently (saved across power-cycles, power-off and resets) store user configuration data.
- Such data that the user might want to change/edit, intermittently (say several times a day at-most). For example keymap configuration for a USB Keyboard, mouse parameters such as dpi, refresh rate, macro functions, temperature values for PID temperature controller -- to name a few.
- There might be several to several tens-of-thousands configuration values that the user might want to change individually
- The user might (obviously) prefer to not have to wait unreasonably long to edit a single user configuration value
- The user might (obviously) prefer that it doesn't take unreasonably long for the device to be functional after a power-cycle or a reset condition

##What features does the UCFKVS provide?
1. The UCFKVS provides a simple interface to edit a value (variable sized bytearray, length of bytearray) for a key (a uint32_t value). The same interface can be used to add_key_values. Insertion/Editing is also O(1).
```
bool = UCFKVS_storage_object.add_edit_key_value(conts uint32_t key, const uint8_t size, const uint8_t* data);
```

2. The UCFKVS provides a simple interface to recall a value (variable sized bytearray, length of bytearray) for a key (a uint32_t value). Lookup is O(log n).

```
std::tuple<length,const uint8_t*> = UCFKVS_storage_object.lookup_key_value_for(uint32_t key);
```
But using this interface is not recommended (at all). It takes O(log n) to lookup a key_value. Instead, it is recommended that at application startup [or at other key moments] a traversal be performed which loads all key_values to their functional places in the code. Even better, the application could keep a pointer to where to find a particular value in the flash. This way, all (n) user_configuration key_values can be loaded at application startup in (O n) time, as opposed to looking up n values independently (nlog n) time.

##How to use the UCFKVS_storage_object?
1. Your linker script will have to be modified to allocate several sectors for user_configuration_data.
2. On startup you should create a UCFKVS_storage_object at the addresses you've allocated for the object.
```
UCFKVS UCFKVS_storage_object = UCFKVS::initialize_on_sector();
```

## Limitations
To be written
