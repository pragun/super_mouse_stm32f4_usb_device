#include "flash_key_value_node.hpp"

extern bool flash_write_byte(void* address, uint8_t data);

Key_Value_Flash_Node::Key_Value_Flash_Node(Node_Address address):
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

uint8_t Key_Value_Flash_Node::find_link_id_by_address(Node_Address address){
	uint8_t link_id = 0;
	for(const auto [addr, state] : links_states_arr){
		if (addr == address) break;
		link_id ++;
	}
	return link_id;
}

bool Key_Value_Flash_Node::mark_link_as(Node_Address address, Link_State_Enum link_state){
	uint8_t link_id = find_link_id_by_address(address);
	uint8_t new_link_status = header->node_link_status & ~(0b11 << (link_id*2));
	new_link_status |= (static_cast<uint8_t>(link_state)) << (link_id*2);
	return flash_write_byte((void*) &(header->node_link_status), new_link_status);
}

template<enum Validity_Flag_Enum flag>
bool Key_Value_Flash_Node::flag_state(){
	return (bool) (header->validity_bitmask & (0x1 << static_cast<uint8_t>(flag)));
}

template<enum Validity_Flag_Enum flag>
bool Key_Value_Flash_Node::invalidate_flag(){
	uint8_t new_validity_byte = header->validity_bitmask & (0b1 << flag);
	return flash_write_byte((void*) &(header->validity_bitmask), new_validity_byte);
}

Link_State_Enum Key_Value_Flash_Node::link_status(uint8_t link_id){
	uint8_t status = 0;
	status = (header->node_link_status & (0b11 << (link_id*2))) >> (link_id*2);
	return static_cast<Link_State_Enum>(status);
}

bool Key_Value_Flash_Node::mark_link_as_invalid(Node_Address address){
	return mark_link_as(address, Link_State_Enum::invalid);
}

bool Key_Value_Flash_Node::mark_link_as_valid(Node_Address address){
	return mark_link_as(address, Link_State_Enum::valid);
}

Node_Address Key_Value_Flash_Node::next_root_node_link(){
	return root_link_address;
}

Key_Value_Flash_Node Key_Value_Flash_Node::read_node_from_flash(Node_Address address){
	return Key_Value_Flash_Node(address);
}

uint32_t Key_Value_Flash_Node::furthest_memory_location(){
	return starting_address + size;
}

//This function is only used if the node is to be written to the flash
Key_Value_Flash_Node Key_Value_Flash_Node::construct_for_writing_to_flash(uint8_t application_index, uint8_t keypad_keynum, uint8_t event_data_size, uint8_t* event_data){
	return Key_Value_Flash_Node(0x0);
}
