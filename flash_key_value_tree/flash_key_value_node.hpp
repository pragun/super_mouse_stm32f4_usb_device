#include <cstdio>
//#include "reporting_functions.h"
#include <stack>
#include <tuple>
#include <functional>

#define MAX_NUM_LINKS 4

enum Validity_Flag_Enum : uint8_t{
	data_valid = 0,
	root_node = 1,
	active_as_root_node = 2,
	root_node_link_unwritten = 3,
	growth_node = 4,
};

enum class Link_State_Enum: uint8_t{
	uninitialized = 0b11,
	valid = 0b10,
	invalid = 0b00,
};

using Node_Address = uint32_t;

class Key_Value_Flash_Node {
private:
	Key_Value_Flash_Node(Node_Address address);
	uint8_t find_link_id_by_address(Node_Address address);
	bool mark_link_as(Node_Address address, Link_State_Enum link_state);

	Node_Address starting_address; //rename to flash_address;
	uint8_t size;
	Node_Address root_link_address;
	Node_Address* link_addresses;

	bool written_to_flash;
	const bool read_from_flash;

public:
	struct header_typedef{
		uint8_t validity_bitmask;
		uint8_t node_link_status;

		/**
		contains the state of 4 links[bb, bb, bb, bb]
		for link_indexes [0, 1, 2, 3]
		For each link bb means --
		0b11 means un-activated link
		0b10 means valid link
		0b00 means invalid link
		**/

		uint32_t key;
		uint8_t value_size;
		uint8_t value[];
	};


	const volatile header_typedef* header;
	std::array<std::tuple<Node_Address, Link_State_Enum>, MAX_NUM_LINKS> links_states_arr;

	template<enum Validity_Flag_Enum flag> bool flag_state();
	template<enum Validity_Flag_Enum flag> bool invalidate_flag();

	Link_State_Enum link_status(uint8_t link_id);

	bool mark_link_as_invalid(Node_Address address);

	bool mark_link_as_valid(Node_Address address);

	Node_Address next_root_node_link();

	static Key_Value_Flash_Node read_node_from_flash(Node_Address address);

	uint32_t furthest_memory_location();

	//This function is only used if the node is to be written to the flash
	static Key_Value_Flash_Node construct_for_writing_to_flash(uint8_t application_index, uint8_t keypad_keynum, uint8_t event_data_size, uint8_t* event_data);
};
