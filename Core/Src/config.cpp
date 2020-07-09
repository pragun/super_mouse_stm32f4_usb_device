#include <cstdio>
#include "reporting_functions.h"
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

bool flash_write_byte(void* address, uint8_t data){
	//HAL_FLASH_Program_IT(FLASH_TYPEPROGRAM_BYTE, (uint32_t) address, Data);
	return true;
}


enum class Sector_State_Enum: uint8_t{
	INVALID = 0x00,
	VALID = 0xF0,
	ERASED = 0xFF,
};

using Node_Address = uint32_t;

class Key_Value_Flash_Node {
private:
	Key_Value_Flash_Node(Node_Address address):
		starting_address(address),
		written_to_flash(false),
		read_from_flash(true),
		header(reinterpret_cast<header_typedef*>(address))
		{
			uint8_t offset = sizeof(header_typedef) + header->value_size;
			uint8_t* byte_address = (uint8_t*) address;

			if(flag_state<Validity_Flag_Enum::root_node>()){
				root_link_address = *((Node_Address*) &byte_address[offset]);
				offset += 4;
			}else{
				root_link_address = 0;
			}

			link_addresses = reinterpret_cast<Node_Address*>(&byte_address[offset]);

			for (uint8_t i = 0; i < MAX_NUM_LINKS; i++){
				links_states_arr[i] = std::make_tuple(reinterpret_cast<Node_Address>(link_addresses[i]),link_status(i));
				if(link_status(i) != Link_State_Enum::uninitialized){
					offset += 4;
				}
			}

			size = offset;
		}

	uint8_t find_link_id_by_address(Node_Address address){
		uint8_t link_id = 0;
		for(const auto [addr, state] : links_states_arr){
			if (addr == address) break;
			link_id ++;
		}
		return link_id;
	}

	bool mark_link_as(Node_Address address, Link_State_Enum link_state){
		uint8_t link_id = find_link_id_by_address(address);
		uint8_t new_link_status = header->node_link_status & ~(0b11 << (link_id*2));
		new_link_status |= (static_cast<uint8_t>(link_state)) << (link_id*2);
		return flash_write_byte((void*) &(header->node_link_status), new_link_status);
	}

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

	Node_Address starting_address;
	uint8_t size;
	const volatile header_typedef* header;
	Node_Address root_link_address;
	Node_Address* link_addresses;

	bool written_to_flash;
	const bool read_from_flash;

	std::array<std::tuple<Node_Address, Link_State_Enum>, MAX_NUM_LINKS> links_states_arr;

	template<enum Validity_Flag_Enum flag>
	bool flag_state(){
		return (bool) (header->validity_bitmask & (0x1 << static_cast<uint8_t>(flag)));
	}

	template<enum Validity_Flag_Enum flag>
	bool invalidate_flag(){
		uint8_t new_validity_byte = header->validity_bitmask & (0b1 << flag);
		return flash_write_byte((void*) &(header->validity_bitmask), new_validity_byte);
	}

	Link_State_Enum link_status(uint8_t link_id){
		uint8_t status = 0;
		status = (header->node_link_status & (0b11 << (link_id*2))) >> (link_id*2);
		return static_cast<Link_State_Enum>(status);
	}

	bool mark_link_as_invalid(Node_Address address){
		return mark_link_as(address, Link_State_Enum::invalid);
	}

	bool mark_link_as_valid(Node_Address address){
		return mark_link_as(address, Link_State_Enum::valid);
	}

	Node_Address next_root_node_link(){
		return root_link_address;
	}

	static Key_Value_Flash_Node read_node_from_flash(Node_Address address){
		return Key_Value_Flash_Node(address);
	}

	uint32_t furthest_memory_location(){
		return starting_address + size;
	}

	//This function is only used if the node is to be written to the flash
	static Key_Value_Flash_Node construct_for_writing_to_flash(uint8_t application_index, uint8_t keypad_keynum, uint8_t event_data_size, uint8_t* event_data);
};

uint32_t furthest_memory_location_used(Key_Value_Flash_Node &a, uint32_t previous_max_value){
	return (uint32_t) std::max(previous_max_value, a.furthest_memory_location());
}

template<typename T>
bool always_true(T a){
	return true;
}

template<typename T>
bool always_false(T a){
	return false;
}

using useless_type = int;

enum class Traversal_Type {fold, search, map};

template <typename FT>
struct traversal{
	typedef std::function<FT(Key_Value_Flash_Node&, FT)> func_type;
	typedef FT return_type;
	typedef return_type accumulator_type;
	constexpr static Traversal_Type traversal_type = Traversal_Type::fold;
};

template<>
struct traversal<bool>{ //The case of a searching type function
	typedef std::function<bool(Key_Value_Flash_Node&)> func_type;
	typedef std::tuple<Node_Address,Node_Address> return_type;
	typedef return_type accumulator_type;
	constexpr static Traversal_Type traversal_type = Traversal_Type::search;
};

template<>
struct traversal<void>{
	typedef std::function<void(Key_Value_Flash_Node&)> func_type;
	typedef void return_type;
	typedef int accumulator_type;
	constexpr static Traversal_Type traversal_type = Traversal_Type::map;
};

/*
	Key_Value_Store operations:
		- load values, can find and mark the growth_node, and the furthest_occupied_memory_location
				- This needs a traversal_map that can introduce outside changes, and will visit each node
				- The finding of the growth_node will be tied to this

		- add new <Key, Value>:
			- Find the growth node
			- Create a RAM copy of the new node
			{
				- Find the starting_location_of_unwritten_flash
				- Add a link from the growth node to the new node
				(These two need to be done together because the second)
				(one depends on the first one and vice versa)
			}
			- Write the node to flash at starting_location_of_unwritten_flash

		- edit <Key, Value> to new value:
			- Find the Node corresponding to the Key,
			- Find the Node corresponding to the parent of the Key Node
			- Find the growth_node -> use the already found growth_node
			- Find the root node -> use the active_root
			- Find the starting_of_unwritten_flash

			- Invalidate the link from the parent to the Key node
			- Invalidate the growth bit for the growth node

			- Create a copy of the Key Node.
				- Mark as "active"root"node"
				- Mark as growth_node
				- Add a link to the root_node (previous root) as a child

			- Write the node to flash
			- Invalidate the "active_root_node" bit
			- Add a root_node_link to the new Key Node
 */


class Flash_Key_Value_Store {
private:
	Node_Address active_root;
	Node_Address growth_node;
	uint32_t furthest_used_memory_location;

public:
	Flash_Key_Value_Store(Node_Address address):
		furthest_used_memory_location(0)
	{
		// build-out the tree
		active_root = find_active_root_node(address);
		//growth_node = find_growth_node();
	}

	template <typename T>
	typename traversal<T>::return_type traverse_with_node_function(typename traversal<T>::func_type func, typename traversal<T>::accumulator_type value){
		std::stack<std::tuple<Node_Address,Node_Address>> traversal_stack;
		traversal_stack.push(std::make_tuple(active_root,0));

		while(traversal_stack.size() != 0){
			auto [node_addr,parent_addr] = traversal_stack.top();
			traversal_stack.pop();

			Key_Value_Flash_Node a = Key_Value_Flash_Node::read_node_from_flash(node_addr);

			if constexpr (traversal<T>::traversal_type == Traversal_Type::fold)
				value = func(a,value);

			if constexpr (traversal<T>::traversal_type == Traversal_Type::map)
				func(a);

			if constexpr (traversal<T>::traversal_type == Traversal_Type::search)
				if(func(a)){
					return std::make_tuple(node_addr, parent_addr);
				};

			for(const auto [link,state] : a.links_states_arr){
				if(state == Link_State_Enum::valid){
					traversal_stack.push(std::make_tuple(link,node_addr));
				}
			}
		}

		if constexpr(traversal<T>::traversal_type != Traversal_Type::map)
				return value;
	}

	static Node_Address find_active_root_node(Node_Address address){
		Node_Address next_address = address;
		bool found_root = false;
		do{
			{
				address = next_address;
				Key_Value_Flash_Node node = Key_Value_Flash_Node::read_node_from_flash(address);
				found_root = node.flag_state<Validity_Flag_Enum::active_as_root_node>();
				next_address = node.next_root_node_link();
			} // This extra layer of scope makes sure that the node
			  // object created in the last loop iteration is taken off the stack
		}while(!found_root);
		return address;
	}

	bool active_root_found(){
		return (active_root != 0);
	}

	template <typename T, bool enable_filter = false>
	T filter_fold_with_node_function(std::function< T (Key_Value_Flash_Node&, T)>fold_function, T value, std::function<bool(T&)> filter_function){
			std::stack<std::tuple<Node_Address,Node_Address>> traversal_stack;
			traversal_stack.push(std::make_tuple(active_root,0));

			while(traversal_stack.size() != 0){
				auto [node_addr,parent_addr] = traversal_stack.top();
				Key_Value_Flash_Node a = Key_Value_Flash_Node::read_node_from_flash(node_addr);

				value = fold_function(a,value);

				if constexpr(enable_filter){
					if (filter_function(value)){
						return value;
					}
				}

				traversal_stack.pop();
				for(const auto [link,state] : a.links_states_arr){
					if(state == Link_State_Enum::valid){
						traversal_stack.push(std::make_tuple(link,node_addr));
					}
				}
			}

			return value;
	}

	//If FT is void, then treat it as if it is a map
	//If FT is bool, then treat it as if it search and return std::tuple<Node_Address,Node_Address>
	//If FT is T, then treat it as if T is the result of a fold

	void map_with_node_function(std::function<void(Key_Value_Flash_Node&)>func){
		traverse_with_node_function<void>(func,0);
	}

	void map_with_key_value_function(std::function<void(const uint32_t key, const uint8_t size, const uint8_t* value)> func){
		auto node_function = [func](const Key_Value_Flash_Node &a){
			func(a.header->key, a.header->value_size, (const unsigned char*) &a.header->value[0]);
		};
		map_with_node_function(node_function);
	}

	uint32_t find_furthest_used_memory_location(){
		/* auto node_function = [](Key_Report_Flash_Node &a, uint32_t previous_max_value){
			return (uint32_t) std::max(previous_max_value, a.furthest_memory_location());
		}; */

		return traverse_with_node_function<uint32_t>(furthest_memory_location_used, (uint32_t) 0);
	}

	std::tuple<Node_Address, Node_Address> find_node_and_parent_matching_key(uint32_t key){
		auto find_function = [key](Key_Value_Flash_Node &a) -> bool {
			return (a.header->key == key);
		};

		return traverse_with_node_function<bool>(find_function, std::make_tuple((Node_Address)0,(Node_Address)0));
	}

	std::tuple<Node_Address, Node_Address> find_growth_node(){
		auto find_function = [](Key_Value_Flash_Node &a) -> bool {
			return (a.flag_state<Validity_Flag_Enum::growth_node>());
		};

		return traverse_with_node_function<bool>(find_function, std::make_tuple((Node_Address)0,(Node_Address)0));
	}


	bool update_key_value(const uint32_t key, const uint8_t size, const uint8_t* value){

	}
};


void check_on_config(){
	asm("nop;");
}

//constexpr auto b = a;

//constexpr storage test_struct = storage();
//[[using gnu : section(".config_sector1") , used]] volatile const storage storage_data = test_struct;
