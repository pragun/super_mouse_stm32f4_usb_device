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

enum class Storage{
	ram, flash
};

using Node_Address = uint32_t;

#pragma pack(1)
struct Node_Header_Data{
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

	//uint8_t value[]; //This makes sense only in the case of a node read from flash

	template<Validity_Flag_Enum flag> bool flag_state() const;
	Link_State_Enum link_status(uint8_t link_id) const;
};

#pragma pack(1)
template <Storage NS>
struct Node_Header_Typedef : Node_Header_Data{
	bool mark_link_as(uint8_t link_id, Link_State_Enum link_state);
	template<Validity_Flag_Enum flag> bool validate_flag();
	template<Validity_Flag_Enum flag> bool invalidate_flag();
};

static_assert(std::is_standard_layout<Node_Header_Typedef<Storage::flash>>::value);
static_assert(std::is_standard_layout<Node_Header_Typedef<Storage::ram>>::value);

class Key_Value_Ram_Node{
private:
	Node_Address root_link_address;
	bool written_to_flash;

	std::array<Node_Address, MAX_NUM_CHILD_NODES> link_addresses;
	Node_Address implicit_link_address;
	uint8_t* data;
	Node_Header_Typedef<Storage::ram> header_obj;
	Node_Header_Typedef<Storage::ram>* header;
	Key_Value_Ram_Node(uint32_t key, uint8_t size, uint8_t* value);

public:
	static Key_Value_Ram_Node create_new_growth_node(uint32_t key, uint8_t size, uint8_t* value);
	bool write_to_address(Node_Address);

};

class Key_Value_Flash_Node {
private:
	Key_Value_Flash_Node(Node_Address address);
	uint8_t find_link_id_by_address(Node_Address address);
	bool mark_link_by_addr_as(Node_Address address, Link_State_Enum link_state);

	Node_Address address_on_flash;
	uint8_t size_on_flash;
	Node_Address root_link_address;
	uint8_t* data;
	std::array<Node_Address, MAX_NUM_CHILD_NODES> link_addresses;
	Node_Address implicit_link_address;

	Node_Header_Typedef<Storage::flash>* header;

public:
	static Key_Value_Flash_Node read_node_from_flash(Node_Address address);

	bool is_root() const;
	Node_Address next_root_node_link();

	std::array<Node_Address,MAX_NUM_CHILD_NODES> valid_children();

	bool mark_implicit_link_as_valid();
	Node_Address add_growth_link();
	static bool growth_node_find_func(Key_Value_Flash_Node &a);
	uint32_t furthest_memory_location();

	const uint32_t key() const;
	const uint8_t* value() const;
	const uint8_t value_size() const;
};
