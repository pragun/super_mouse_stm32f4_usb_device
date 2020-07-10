#include <cstdio>
//#include "reporting_functions.h"
#include <stack>
#include <tuple>
#include <functional>

#define MAX_NUM_CHILD_NODES 4

constexpr uint8_t Implicit_Link_ID = 0;

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

enum class Node_Storage{
	ram, flash
};

using Node_Address = uint32_t;

#pragma pack(1)
struct Node_Header_Typedef{
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

	uint8_t value[]; //This makes sense only in the case of a node read from flash

	Link_State_Enum link_status(uint8_t link_id) const;
	template<Validity_Flag_Enum flag> bool flag_state() const;
	template<Node_Storage> bool mark_link_as(uint8_t link_id, Link_State_Enum link_state);
};

static_assert(std::is_standard_layout<Node_Header_Typedef>::value);
static_assert(std::is_standard_layout<Node_Header_Typedef>::value);

class Key_Value_Ram_Node{
private:
	Node_Address root_link_address;
	bool written_to_flash;

	std::array<Node_Address, MAX_NUM_CHILD_NODES> link_addresses;
	Node_Address implicit_link_address;
	uint8_t* data;
	Node_Header_Typedef header;
	Key_Value_Ram_Node(uint32_t key, uint8_t size, uint8_t* value);

public:
	static Key_Value_Ram_Node create_new_growth_node(uint32_t key, uint8_t size, uint8_t* value);
	bool write_to_address(Node_Address);
	template<enum Validity_Flag_Enum flag> void validate_flag();
	//void mark_link_by_index_as(uint8_t link_id, Link_State_Enum link_state);
};

class Key_Value_Flash_Node {
private:
	Key_Value_Flash_Node(Node_Address address);
	uint8_t find_link_id_by_address(Node_Address address);
	bool mark_link_by_addr_as(Node_Address address, Link_State_Enum link_state);
	//bool mark_link_by_index_as(uint8_t link_id, Link_State_Enum link_state);

	Node_Address address_on_flash;
	uint8_t size_on_flash;
	Node_Address root_link_address;
	//Node_Address* link_addresses;

	std::array<Node_Address, MAX_NUM_CHILD_NODES> link_addresses;
	Node_Address implicit_link_address;

	Node_Header_Typedef* header;

public:

//	template<enum Validity_Flag_Enum flag> bool flag_state();
//	template<enum Validity_Flag_Enum flag> bool invalidate_flag(); // do not use this use the corresponding function from header instead

//	Link_State_Enum link_status(uint8_t link_id);
//	bool mark_link_as_invalid(Node_Address address);
//	bool mark_link_as_valid(Node_Address address);

	bool mark_implicit_link_as_valid();

	Node_Address next_root_node_link();

	std::array<Node_Address,MAX_NUM_CHILD_NODES> valid_children();

	static Key_Value_Flash_Node read_node_from_flash(Node_Address address);

	Node_Address add_growth_link();

	uint32_t furthest_memory_location();

	uint32_t key() const;

	uint8_t* value() const;

	uint8_t value_size() const;

	bool is_root() const;

	static bool growth_node_find_func(Key_Value_Flash_Node &a);
	//This function is only used if the node is to be written to the flash
	//static Key_Value_Flash_Node construct_for_writing_to_flash(uint32_t key, uint8_t data_size, uint8_t* data);
};
