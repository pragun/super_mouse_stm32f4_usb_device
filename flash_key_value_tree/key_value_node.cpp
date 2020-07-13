#include <key_value_node.hpp>


extern bool flash_write_byte(uint32_t flash_addr, uint8_t data);

extern bool memcpy_to_flash(uint32_t flash_addr, const uint8_t* data, uint8_t size);

template <typename T, int N>
void LazyFilter<T,N>::find_next(){
	for(; index < base_size; index++){
		if(filter_func(base_array[index],index))
			break;
	}
}

template <typename T, int N>
LazyFilter<T,N>::LazyFilter(std::array<T,N>& base_arr, std::function<bool(T, uint8_t)> ffunc):
base_array(base_arr),
base_size(base_array.size()),
num_items_calculated(false),
num_items_val(0),
filter_func(ffunc),
index(0){
	find_next();
};

template <typename T, int N>
T LazyFilter<T,N>::value(){
	return base_array[index];
};

template <typename T, int N>
bool LazyFilter<T,N>::reached_end(){
	return !(index <= base_size);
};

template <typename T, int N>
void LazyFilter<T,N>::operator++(){
	index++;
	find_next();
};

template <typename T, int N>
uint8_t LazyFilter<T,N>::num_items(){
	if(!num_items_calculated){
		num_items_val = 0;
		for(uint8_t i = 0; i<base_size; i++){
			if(filter_func(base_array[i]))
				num_items_val++;
		}
		num_items_calculated = true;
	}
	return num_items_val;
}

template <typename T, int N>
void LazyFilter<T,N>::reset_to_start(){
	index = 0;
	find_next();
}

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
Key_Value_Ram_Node::Key_Value_Ram_Node(uint32_t key, uint8_t size, const uint8_t* data_ptr)
{
	header_obj = {{ 0x0, 0x0, key, size}}; //The status byte and the node_link_status are initially all cleared out

	// All new nodes that are created have these following flags validated
	header_obj.validate_flag<Validity_Flag_Enum::data_valid>();
	header_obj.validate_flag<Validity_Flag_Enum::growth_node>();

	data = data_ptr;
	header = &header_obj;
};

Key_Value_Ram_Node Key_Value_Ram_Node::create_new_growth_node(uint32_t key, uint8_t size, uint8_t* data){
	return Key_Value_Ram_Node(key, size, data);
}

Key_Value_Ram_Node Key_Value_Ram_Node::create_new_root_node(uint32_t key, uint8_t size, const uint8_t* value){
	auto retval = Key_Value_Ram_Node(key, size, value);

	retval.header_obj.validate_flag<Validity_Flag_Enum::root_node>();
	retval.header_obj.validate_flag<Validity_Flag_Enum::active_as_root_node>();
	retval.header_obj.validate_flag<Validity_Flag_Enum::root_node_link_unwritten>();

	return retval;
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

bool Key_Value_Flash_Node::valid_children_filter_func_typedef::operator ()(Node_Address n_addr, uint8_t index){
	return (header->link_status(index) == Link_State_Enum::valid);
}

inline Key_Value_Flash_Node::Key_Value_Flash_Node(Node_Address address):
header(reinterpret_cast<Node_Header_Typedef<Storage::flash>*>(address)),
data(nullptr),
root_link_address((Node_Address) 0),
size_on_flash(0),
valid_children_filter_func{this->header},
valid_children_lazyfltr(link_addresses, valid_children_filter_func)
{
	uint8_t* byte_address = (uint8_t*) address;
	uint8_t offset = sizeof(Node_Header_Typedef<Storage::flash>);
	data = &byte_address[offset];

	offset += header->value_size;

	if(header->flag_state<Validity_Flag_Enum::root_node>()){
		root_link_address = *((Node_Address*) &byte_address[offset]);
		offset += 4;
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
	//implicit_link_address =

	link_addresses[Implicit_Link_ID] = (Node_Address) (reinterpret_cast<uint32_t>(address) + size_on_flash);
}

Nodes_Filter& Key_Value_Flash_Node::valid_children_links(){
	return valid_children_lazyfltr;
}

Node_Address Key_Value_Flash_Node::flash_address(){
	return reinterpret_cast<Node_Address>(header);
}

/*
uint8_t Key_Value_Flash_Node::num_valid_children_links(){
	uint8_t ret_val = 0;
	for (uint8_t i = 1; i < MAX_NUM_CHILD_NODES; i++){ //The first link is implicit
		if(header->link_status(i) == Link_State_Enum::valid)
			ret_val ++;
	}

	if(header->link_status(Implicit_Link_ID) == Link_State_Enum::valid)
		ret_val++;

	return ret_val;
}*/

/*
std::array<Node_Address,MAX_NUM_CHILD_NODES> Key_Value_Flash_Node::valid_children_links(){
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
}*/



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

inline bool Key_Value_Flash_Node::mark_uninitialized_growth_link_as_valid(){
	return header->mark_link_as(Implicit_Link_ID, Link_State_Enum::valid);
}

inline Node_Address Key_Value_Flash_Node::link_to_next_root_node(){
	return root_link_address;
}

Key_Value_Flash_Node Key_Value_Flash_Node::read_node_from_flash(Node_Address address){
	return Key_Value_Flash_Node(address);
}

inline uint32_t Key_Value_Flash_Node::furthest_memory_location(){
	return flash_address() + size_on_flash;
}

Node_Address Key_Value_Flash_Node::mark_growth_link_valid_and_return_address_it_points_to(){
	if (header->flag_state<Validity_Flag_Enum::growth_node>()){
		mark_uninitialized_growth_link_as_valid();
		return (link_addresses[Implicit_Link_ID]);
	}
	return (Node_Address) 0;
}
