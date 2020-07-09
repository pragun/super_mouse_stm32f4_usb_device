#include <cstdio>
#include "reporting_functions.h"
#include "flash_key_value_tree.hpp"
#include <stack>
#include <tuple>
#include <functional>

void check_on_config(){
	asm("nop;");
}

//constexpr auto b = a;

//constexpr storage test_struct = storage();
//[[using gnu : section(".config_sector1") , used]] volatile const storage storage_data = test_struct;
