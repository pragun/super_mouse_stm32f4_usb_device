#include <cstdio>
#include "reporting_functions.h"

#define NUM_ROOT_PTRS_INDEX_LINK 8

namespace SectorState {
	enum SectorState : uint8_t{
		INVALID = 0x00,
		VALID = 0xF0,
		ERASED = 0xFF,
	};
}

struct RootIndexLink_Typedef{
	uint8_t sector_state;
	uint8_t validity_bitmask;
	uint16_t _reserved_;
	volatile const Key_Report_Flash_Config_Node_Typedef* root_ptrs[NUM_ROOT_PTRS_INDEX_LINK];
	volatile const RootIndexLink_Typedef* next_root_index_link;

	/* constexpr RootIndexLink_Typedef():
			sector_state(SectorState::VALID),
			validity_bitmask(0xFF),
			_reserved_(),
			root_ptrs {(Key_Report_Flash_Config_Node_Typedef*) 0x00},
			next_root_index_link ((RootIndexLink_Typedef*) 0x00)
			{}
	*/
};

/*
 * When validity:0x00, go to next_tree_root_index_node to find node
 * When validity:0xFF, the first node address in the list is valid. Clz = 0
 * When validity:0x01, the last node address in the list is valid. Clz = 7
 * When 0x01 <validity < 0xFF, the Clz(validity) index in node_ptrs is valid
 *
 * All valid configuration nodes should always be accessible from the node pointed
 * by the valid node_ptr in this struct, or by a successive RootNode_Node
 * down the links
 */



//extern volatile const Key_Report_Flash_Config_Node_Typedef FirstNode;

//[[using gnu : section(".config_sector1") , used]] volatile const RootIndexLink_Typedef Root_RootIndexLink = { SectorState::VALID, 0xFF, 0x0000, {/*&FirstNode*/,0,0,0,0,0,0,0}, nullptr};

//[[using gnu : section(".config_sector1") , used]] volatile const Key_Report_Flash_Config_Node_Typedef FirstNode = {0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00, 0x00}};

//[[using gnu : section(".config_sector1") , used]] volatile const Key_Report_Flash_Config_Node_Typedef SecondNode = {0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00, 0x00}};

//[[using gnu : section(".config_sector2") , used]] volatile const uint8_t SectorState2 = SectorState::ERASED;

template<int N>
struct default_nodes{
	Key_Report_Flash_Config_Node_Typedef node_data;
	volatile const Key_Report_Flash_Config_Node_Typedef* node_link[N];
};

typedef default_nodes<1> Default_Node_Typedef;
typedef default_nodes<2> Root_Node_Typedef;


struct storage{

	RootIndexLink_Typedef root_root_index_link;
	EventType_Offset_Table default_event_table;

	Key_Report_Event_Config_Typedef2<empty_struct> default_short_press;
	Key_Report_Event_Config_Typedef2<empty_struct> default_long_press;
	Key_Report_Event_Config_Typedef2<Altered_Mouse_Movement_Params_Typedef> default_movement;

	Default_Node_Typedef default_nodes[NUM_KEYS_KEYPAD-1];
	Root_Node_Typedef root_node;

	constexpr storage():
		root_root_index_link {SectorState::VALID, 0xFF, 0x0000,
								{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
								nullptr},
		default_event_table { .__reserved__ = 0,
			.event_table = {
				offsetof(storage,default_short_press) - offsetof(storage,default_event_table),
				offsetof(storage,default_long_press) - offsetof(storage,default_event_table),
				offsetof(storage,default_movement) - offsetof(storage,default_event_table) }
			},
		default_short_press {
				.reporting_function_index = 0,
				.parameter_struct_size = 0,
				.parameters = {}
			},
		default_long_press {
				.reporting_function_index = 0,
				.parameter_struct_size = 0,
				.parameters = {}
			},
		default_movement {
				.reporting_function_index = 1,
				.parameter_struct_size = sizeof(Altered_Mouse_Movement_Params_Typedef),
				.parameters = {.x_factor = 1, .y_factor = 1, .z_factor = 1}
			},
		default_nodes(),
		root_node()
		{
			for (auto i = 0; i != NUM_KEYS_KEYPAD-1; ++i){
				default_nodes[i] = {
						{.redirect=1,
						.valid=1,
						.node_ptr_validity_bitmask=0x1,
						.application_index = 0,
						.keypad_keynum = i,
						.node_links_struct_offset = 0,
						.ptr_to_report_offset_table = &default_event_table},{nullptr}
				};
				if (i >= 1){
					default_nodes[i].node_link[0] = (Key_Report_Flash_Config_Node_Typedef*) &default_nodes[i-1];
				}
			}

			root_node =
					{
						{
						.redirect=1,
						.valid=1,
						.node_ptr_validity_bitmask=0x1,
						.application_index = 0,
						.keypad_keynum = NUM_KEYS_KEYPAD-1,
						.node_links_struct_offset = 0,
						.ptr_to_report_offset_table = &default_event_table},
						//{(Key_Report_Flash_Config_Node_Typedef*) 0xFFFFFFFF, (Key_Report_Flash_Config_Node_Typedef*) 0xFFFFFFFF}
						//{(Key_Report_Flash_Config_Node_Typedef*) &default_nodes[NUM_KEYS_KEYPAD-2], (Key_Report_Flash_Config_Node_Typedef*) 0xFFFFFFFF}
						{(Key_Report_Flash_Config_Node_Typedef*) &default_nodes[NUM_KEYS_KEYPAD-2], (Key_Report_Flash_Config_Node_Typedef*) &default_nodes[NUM_KEYS_KEYPAD-2]}
					};

			for (auto j = 0; j != NUM_ROOT_PTRS_INDEX_LINK; j++){
				//root_root_index_link.root_ptrs[j] =  (Key_Report_Flash_Config_Node_Typedef*) 0xFFFFFFFF;
				root_root_index_link.root_ptrs[j] = (Key_Report_Flash_Config_Node_Typedef*) &default_nodes[NUM_KEYS_KEYPAD-2];
			}
		}
};

/*

 */

void check_on_config(){
	asm("nop;");
}

//constexpr auto b = a;

constexpr storage test_struct = storage();
//[[using gnu : section(".config_sector1") , used]] volatile const storage storage_data = test_struct;
