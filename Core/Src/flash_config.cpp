#include <cstdio>
#include <tuple>
#include <functional>
#include "stm32f4xx_hal.h"

void flash_erase(){
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Sector = FLASH_SECTOR_1;
	EraseInitStruct.NbSectors = 1;

	uint32_t PAGEError = 0;

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	if(HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
			    //Erase error!
	}
}

void flash_unlock(){
	HAL_FLASH_Unlock();
}

void flash_lock(){
	HAL_FLASH_Lock();
}

void check_on_config(){
	asm("nop;");
}

bool flash_write_byte(uint32_t flash_addr, uint8_t data){
	HAL_StatusTypeDef a = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_addr, data);
	return (a == HAL_OK);
}

bool memcpy_to_flash(uint32_t flash_addr, const uint8_t* data, uint8_t size){
	for(uint8_t i = 0; i < size; i++){
		if(!flash_write_byte(flash_addr, data[i]))
			return false;
		flash_addr += 1;
	}
	return true;
}


char data[] = "This is a test string";
uint8_t size = 20;

//__attribute__((section(".config_sector1")))
//uint32_t clear_config_flash_sector1[4096] = {0xFFFFFFFF};

//__attribute__((section(".config_sector2")))
//uint32_t clear_config_flash_sector2[4096] = {0xFFFFFFFF};


/*
void test_config(uint8_t test, void (*traversal_func)(const uint32_t key, const uint8_t len, const uint8_t* data)){
	switch(test){
	case 1:
	{
		Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_growth_node(0x1, size, (uint8_t*) data);
		HAL_StatusTypeDef a = HAL_FLASH_Unlock();
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR | FLASH_FLAG_PGPERR);
		new_node.write_to_address(r1);
		HAL_FLASH_Lock();
		break;
	}

	case 2:
	{
		Key_Value_Flash_Node r_new_node = Key_Value_Flash_Node::read_node_from_flash(r1);
		break;
	}

	case 3:
	{



		Flash_Key_Value_Tree r_tree = Flash_Key_Value_Tree((uint32_t)r1,&flash_unlock,&flash_lock);

		r_tree.add_edit_key_value(7, 20, (uint8_t*) data);
		r_tree.reload();
		r_tree.add_edit_key_value(9, 20, (uint8_t*) data);
		r_tree.reload();
		r_tree.add_edit_key_value(1, 20, (uint8_t*) data);
		r_tree.reload();
		r_tree.add_edit_key_value(9, 20, (uint8_t*) data);
		r_tree.reload();
		r_tree.add_edit_key_value(9, 20, (uint8_t*) data);
		r_tree.reload();


		break;
	}

	case 4:
		Flash_Key_Value_Tree r_tree = Flash_Key_Value_Tree((uint32_t)r1);
		//std::function<void(const uint32_t, const uint8_t, const uint8_t*)> fptr2 = traversal_func;
		//r_tree.map_with_key_value_function(fptr2);
		r_tree.map_with_key_value_function2(traversal_func);
		break;
	}
}
*/
