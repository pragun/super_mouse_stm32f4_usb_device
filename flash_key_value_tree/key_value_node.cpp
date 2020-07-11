#include <key_value_node.hpp>


extern bool flash_write_byte(uint32_t flash_addr, uint8_t data);

extern bool memcpy_to_flash(uint32_t flash_addr, uint8_t* data, uint8_t size);

/** Node_Header_Data **/

template<Validity_Flag_Enum flag>
inline bool Node_Header_Data::flag_state() const{
	return (bool) (validity_bitmask & (0x1 << static_cast<uint8_t>(flag)));
}

template<>
inline bool Node_Header_Data::flag_state<Validity_Flag_Enum::growth_node>() const {
	return (link_status(Implicit_Link_ID) == Link_State_Enum::uninitialized);
	//return (bool) (header->validity_bitmask & (0x1 << static_cast<uint8_t>(flag)));
}

inline Link_State_Enum Node_Header_Data::link_status(uint8_t link_id) const {
	uint8_t status = 0;
	status = (node_link_status & (0b11 << (link_id*2))) >> (link_id*2);
	return static_cast<Link_State_Enum>(status);
}

/** Node_Header_Typedef **/
template<>
inline bool Node_Header_Typedef<Storage::ram>::mark_link_as(uint8_t link_id, Link_State_Enum link_state){
	uint8_t new_link_status = node_link_status & ~(0b11 << (link_id*2));
	node_link_status |= (static_cast<uint8_t>(link_state)) << (link_id*2);
	return true;
}

template<>
inline bool Node_Header_Typedef<Storage::flash>::mark_link_as(uint8_t link_id, Link_State_Enum link_state){
	uint8_t new_link_status = node_link_status & ~(0b11 << (link_id*2));
	new_link_status |= (static_cast<uint8_t>(link_state)) << (link_id*2);
	return flash_write_byte((uint32_t) &(node_link_status), new_link_status);
}


template<>
template<Validity_Flag_Enum flag>
inline bool Node_Header_Typedef<Storage::ram>::validate_flag(){
	validity_bitmask |=  (0x1 << static_cast<uint8_t>(flag));
	return true;
}

template<>
template<>
inline bool Node_Header_Typedef<Storage::ram>::validate_flag<Validity_Flag_Enum::growth_node>(){
	mark_link_as(Implicit_Link_ID, Link_State_Enum::uninitialized);
	return true;
}

template<>
template<Validity_Flag_Enum flag>
bool Node_Header_Typedef<Storage::flash>::invalidate_flag(){
	uint8_t new_validity_byte = validity_bitmask & (0b1 << flag);
	return flash_write_byte((uint32_t) &(validity_bitmask), new_validity_byte);
}

template<>
template<>
bool Node_Header_Typedef<Storage::flash>::invalidate_flag<Validity_Flag_Enum::growth_node>();


/** --- Key_Value_RAM_Node --- **/
Key_Value_Ram_Node::Key_Value_Ram_Node(uint32_t key, uint8_t size, uint8_t* data_ptr)
{
	header_obj = {{ 0x0, 0x0, key, size}}; //The status byte and the node_link_status are initially all cleared out
	header_obj.validate_flag<Validity_Flag_Enum::data_valid>();
	header_obj.validate_flag<Validity_Flag_Enum::growth_node>();
	data = data_ptr;
	header = &header_obj;
};

Key_Value_Ram_Node Key_Value_Ram_Node::create_new_growth_node(uint32_t key, uint8_t size, uint8_t* data){
	return Key_Value_Ram_Node(key, size, data);
}

bool Key_Value_Ram_Node::write_to_address(Node_Address addr){
	uint32_t offset = 0;
	uint32_t write_addr = (uint32_t) addr;
	bool retval = true;

	retval &= memcpy_to_flash(write_addr, (uint8_t*) &header_obj, sizeof(header_obj));
	write_addr += sizeof(header_obj);

	retval &= memcpy_to_flash(write_addr, data, header->value_size);
	write_addr += header->value_size;

	if(header->flag_state<Validity_Flag_Enum::root_node>()){
		retval &= memcpy_to_flash(write_addr, (uint8_t*) &root_link_address, 4);
		write_addr += 4;
	}

	for (uint8_t i = 1; i < MAX_NUM_CHILD_NODES; i++){ //The first link is implicit
		if(header->link_status(i) == Link_State_Enum::valid){
			retval &= memcpy_to_flash(write_addr, (uint8_t*) &link_addresses[i], 4);
			write_addr += 4;
		}
	}

	uint32_t size = write_addr - addr;
	return retval;
}


/** --- Key_Value_Flash_Node --- **/

inline Key_Value_Flash_Node::Key_Value_Flash_Node(Node_Address address):
address_on_flash(address),
header(reinterpret_cast<Node_Header_Typedef<Storage::flash>*>(address))
{
	uint8_t* byte_address = (uint8_t*) address;
	uint8_t offset = sizeof(Node_Header_Typedef<Storage::flash>);
	data = &byte_address[offset];

	offset += header->value_size;

	if(header->flag_state<Validity_Flag_Enum::root_node>()){
		root_link_address = *((Node_Address*) &byte_address[offset]);
		offset += 4;
	}else{
		root_link_address = 0;
	}

	Node_Address* link_addr = reinterpret_cast<Node_Address*>(&byte_address[offset]);
	for (uint8_t i = 1; i < MAX_NUM_CHILD_NODES; i++){ //The first link is implicit
		switch(header->link_status(i)){
		case Link_State_Enum::invalid:
			link_addresses[i] = (Node_Address)0;
			break;

		case Link_State_Enum::uninitialized:
			link_addresses[i] = (Node_Address)0;
			offset += 4;
			break;

		case Link_State_Enum::valid:
			link_addresses[i] = reinterpret_cast<Node_Address>(link_addr[i]);
			offset += 4;
			break;
		}
	}

	size_on_flash = offset;
	implicit_link_address = (Node_Address) (reinterpret_cast<uint32_t>(address) + size_on_flash);

	link_addresses[Implicit_Link_ID] = reinterpret_cast<Node_Address>(implicit_link_address);
}


std::array<Node_Address,MAX_NUM_CHILD_NODES> Key_Value_Flash_Node::valid_children(){
	std::array<Node_Address,MAX_NUM_CHILD_NODES> ret_array;
	for (uint8_t i = 1; i < MAX_NUM_CHILD_NODES; i++){ //The first link is implicit
		if(header->link_status(i) == Link_State_Enum::valid)
			ret_array[i] = link_addresses[i];
		else
			ret_array[i] = (Node_Address) 0;
	}

	if(header->link_status(Implicit_Link_ID) == Link_State_Enum::valid)
		ret_array[0] = implicit_link_address;
	else
		ret_array[0] = (Node_Address) 0;
	return ret_array;
}


uint8_t Key_Value_Flash_Node::find_link_id_by_address(Node_Address address){
	uint8_t link_id = 0;
	for(const auto addr : link_addresses){
		if (addr == address) break;
		link_id ++;
	}
	return link_id;
}

inline const uint32_t Key_Value_Flash_Node::key() const{
	return header->key;
}

inline const uint8_t Key_Value_Flash_Node::value_size() const{
	return header->value_size;
}

inline const uint8_t* Key_Value_Flash_Node::value() const{
	return data;
}

inline bool Key_Value_Flash_Node::is_root() const{
	return header->flag_state<Validity_Flag_Enum::active_as_root_node>();
}


inline bool Key_Value_Flash_Node::mark_link_by_addr_as(Node_Address address, Link_State_Enum link_state){
	uint8_t link_id = find_link_id_by_address(address);
	return header->mark_link_as(link_id, link_state);
}

//This is a static function
inline bool Key_Value_Flash_Node::growth_node_find_func(Key_Value_Flash_Node &a) {
	return (a.header->flag_state<Validity_Flag_Enum::growth_node>());
}

inline bool Key_Value_Flash_Node::mark_implicit_link_as_valid(){
	return header->mark_link_as(Implicit_Link_ID, Link_State_Enum::valid);
}

inline Node_Address Key_Value_Flash_Node::next_root_node_link(){
	return root_link_address;
}

Key_Value_Flash_Node Key_Value_Flash_Node::read_node_from_flash(Node_Address address){
	return Key_Value_Flash_Node(address);
}

inline uint32_t Key_Value_Flash_Node::furthest_memory_location(){
	return address_on_flash + size_on_flash;
}

Node_Address Key_Value_Flash_Node::add_growth_link(){
	if (header->flag_state<Validity_Flag_Enum::growth_node>()){
		mark_implicit_link_as_valid();
		return (link_addresses[Implicit_Link_ID]);
	}
	return (Node_Address) 0;
}
