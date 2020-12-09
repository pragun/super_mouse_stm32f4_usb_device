#include <key_value_tree.hpp>

/*
	Key_Value_Store Internal operations:
		- load values, can find and mark the growth_node, and the furthest_occupied_memory_location
				- This needs a traversal_map that can introduce outside changes, and will visit each node
				- The finding of the growth_node will be tied to this

		- add new <Key, Value>:
				- First Make sure that the said key doesn't already exist'
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

/*: Deprecated, will use growth_node for this
uint32_t furthest_memory_location_used_func(Key_Value_Flash_Node &a, uint32_t previous_max_value){
	return (uint32_t) std::max(previous_max_value, a.furthest_memory_location());
}
*/


template<typename T>
bool always_true(T a){
	return true;
}

template<typename T>
bool always_false(T a){
	return false;
}

Flash_Key_Value_Tree::Flash_Key_Value_Tree(uint32_t address):
root_node_addr((Node_Address) 0),
growth_node_addr((Node_Address) 0),
root_node_addr_valid(false),
growth_node_addr_valid(false)
{
	tree_address = address;
	status = (uint32_t*) address;

	reload();
};

void Flash_Key_Value_Tree::reload(){
	if(has_first_node_written()){
			{
				auto [a, b]	= find_active_root_node();
				root_node_addr_valid = a;
				root_node_addr = b;
			}
			{
				auto [a, b]	= find_growth_node();
				growth_node_addr_valid = a;
				growth_node_addr = b;
			}
		}
}


template <typename T>
typename traversal_types<T>::return_t Flash_Key_Value_Tree::traverse_with_node_function(typename traversal_types<T>::func func, typename traversal_types<T>::accumulator value){
	std::stack<std::tuple<Node_Address,Node_Address>> traversal_stack;
	traversal_stack.push(std::make_tuple(root_node_addr,0));

	while(!traversal_stack.empty()){
		auto [node_addr,parent_addr] = traversal_stack.top();
		traversal_stack.pop();

		Key_Value_Flash_Node a = Key_Value_Flash_Node::read_node_from_flash(node_addr);

		if constexpr (traversal_types<T>::handling == Traversal_Type::fold)
						value = func(a,value);

		if constexpr (traversal_types<T>::handling == Traversal_Type::map)
						func(a);

		if constexpr (traversal_types<T>::handling == Traversal_Type::search)
						if(func(a)){
							return std::make_tuple(node_addr, parent_addr);
						};

		for(Nodes_Filter valid_children_links = a.valid_children_links(); !valid_children_links.reached_end(); ++valid_children_links){
			Node_Address child_node_addr = valid_children_links.value();
			traversal_stack.push(std::make_tuple(child_node_addr,node_addr));
		}
	}

	if constexpr(traversal_types<T>::handling != Traversal_Type::map)
						return value;
}

template <Tree_State T>
bool Flash_Key_Value_Tree::state(){
	return ! ( (bool) ((*status) & (0x1 << static_cast<uint8_t>(T))) );
}

/* Use this when some of the bits are to be read inverted
template <>
bool Flash_Key_Value_Tree::state<Tree_State::rooted>(){
	return !( (bool) ((*status) & (0x1 << static_cast<uint8_t>(Tree_State::rooted))) );
} */

std::tuple<bool,Node_Address> Flash_Key_Value_Tree::find_active_root_node(){
	Node_Address next_address = first_root_node_address();
	Node_Address address = (Node_Address) 0;
	bool found_root = false;
	do{
		{
			address = next_address;
			Key_Value_Flash_Node node = Key_Value_Flash_Node::read_node_from_flash(address);
			found_root = node.is_root();
			next_address = node.link_to_next_root_node();
		} // This extra layer of scope makes sure that the node
		// object created in the last loop iteration is taken off the stack
	}while(!found_root);

	return std::make_tuple(found_root,address);
}

inline bool Flash_Key_Value_Tree::has_first_node_written(){
	Node_Header_Typedef<Storage::flash>* first_header = reinterpret_cast<Node_Header_Typedef<Storage::flash>*>(first_root_node_address());
	return first_header->flag_state<Validity_Flag_Enum::node_written>();
}

inline bool Flash_Key_Value_Tree::has_active_root_node(){
	return root_node_addr_valid;
}

inline bool Flash_Key_Value_Tree::has_active_growth_node(){
	return growth_node_addr_valid;
}


inline Node_Address Flash_Key_Value_Tree::get_active_root_node_address(){
	return root_node_addr;
}

inline Node_Address Flash_Key_Value_Tree::get_active_growth_node_address(){
	return growth_node_addr;
}

inline void Flash_Key_Value_Tree::map_with_node_function(std::function<void(Key_Value_Flash_Node&)>func){
	traverse_with_node_function<void>(func,0);
}

void Flash_Key_Value_Tree::map_with_key_value_function(std::function<void(const uint32_t key, const uint8_t size, const uint8_t* value)> func){
	auto node_function = [func](const Key_Value_Flash_Node &a){
		func((const uint32_t) a.key(), (const uint8_t) a.value_size(), (const unsigned char*) a.value());
	};
	map_with_node_function(node_function);
}

void Flash_Key_Value_Tree::map_with_key_value_function2(void (*func)(const uint32_t key, const uint8_t size, const uint8_t* value) ){
	auto node_function = [func](const Key_Value_Flash_Node &a){
		func((const uint32_t) a.key(), (const uint8_t) a.value_size(), (const unsigned char*) a.value());
	};
	map_with_node_function(node_function);
}

std::tuple<Node_Address, Node_Address> Flash_Key_Value_Tree::find_node_and_parent_matching_key(uint32_t key){
	auto find_function = [key](Key_Value_Flash_Node &a) -> bool {
		return (a.key() == key);
	};
	return traverse_with_node_function<bool>(find_function, std::make_tuple((Node_Address)0,(Node_Address)0));
}

inline std::tuple<bool,Node_Address> Flash_Key_Value_Tree::find_growth_node(){
	auto [g_n_addr, __addr1] = traverse_with_node_function<bool>(Key_Value_Flash_Node::growth_node_find_func, std::make_tuple((Node_Address)0,(Node_Address)0));
	return std::make_tuple((g_n_addr != 0), g_n_addr);
}


/*	Edit <Key, Value> to new value:
		- Find the existing Node corresponding to the Key [edit_node]
		- Find the Node corresponding to the parent of the edit_node [edit_nodes_parent_node]
		- Find the growth_node -> use the already found growth_node [growth_node]
		- Find the root node -> use the active_root [active_root_node]
		- Find the starting_of_unwritten_flash

		- Invalidate the growth bit for the growth node
		- Invalidate the valid_data bit for the edit_node
		- Invalidate the active_root_node for the root node

		- Invalidate the link from the parent to the Key node [[needs more thought]]

		- Create a copy of the Key Node.
			- Mark as "active"root"node"
			- Mark as growth_node
			- Add a link to the root_node (previous root) as a child

		- Write the node to flash
		- Invalidate the "active_root_node" bit
		- Add a root_node_link to the new Key Node */

//See explanation:
// Ideally, the new_node should take care of including all valid links from the previous node being edited.
// + a link to the old root node, + leaving space for growth_nodes
// So the maximum number of links that it can take from the previous node are:
// MAX_NUM_LINKS - 2
//If the number of links exceed this on the previous node, then the previous node will be left in
// Special case:
// The active root is also the edit node, in which case no extra link is needed to
// point back to the root,
// So the maximum number of links that it can take from the previous node are:
// MAX_NUM_LINKS - 1
bool Flash_Key_Value_Tree::update_node(const uint32_t key, const uint8_t size, const uint8_t* value, Node_Address edit_node_addr, Node_Address parent_addr){

	auto active_root_node_addr = get_active_root_node_address();
	uint8_t max_num_links_that_can_be_transferred_in_from_old_node = (active_root_node_addr == edit_node_addr)? MAX_NUM_CHILD_NODES - 1 : MAX_NUM_CHILD_NODES - 2;

	Key_Value_Flash_Node edit_node = Key_Value_Flash_Node::read_node_from_flash(edit_node_addr);
	edit_node.mark_data_as_invalid();
	Nodes_Filter valid_children_links = edit_node.valid_children_links();

	if(valid_children_links.num_items() <= max_num_links_that_can_be_transferred_in_from_old_node){
		//In this case this will be added as a new root node that will take over all the previous links
		Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_root_node(key, size, value);

		for(; !valid_children_links.reached_end(); ++valid_children_links){
			new_node.add_valid_non_growth_child_link(valid_children_links.value());
		}

		if(active_root_node_addr != edit_node_addr){
			new_node.add_valid_non_growth_child_link(active_root_node_addr);
		}

		//We'll mark the link from the parent node pointing to the edit_node as invalid
		//as the new node will take care of the responsibility for each of the edit node's children
		if(parent_addr != (Node_Address) 0){
			auto edit_nodes_parent_node = Key_Value_Flash_Node::read_node_from_flash(parent_addr);
			edit_nodes_parent_node.mark_invalid_link_to(edit_node_addr);
		}

		Key_Value_Flash_Node&& active_root_node = (active_root_node_addr == edit_node_addr)? edit_node : Key_Value_Flash_Node::read_node_from_flash(active_root_node_addr);
//		active_root_node.mark_active_root_as_invalid();

		Node_Address growth_node_addr = get_active_growth_node_address();
		auto growth_node = Key_Value_Flash_Node::read_node_from_flash(growth_node_addr);
		Node_Address new_node_addr = growth_node.mark_growth_link_INVALID_and_return_address_it_points_to();
		new_node.write_to_address(new_node_addr);

		active_root_node.point_next_root_link_to(new_node_addr);

	}else{
		//In this case the new node will be added as a regular-ass growth node, while leaving behind
		//the edit_node's carcass to point to its children
		add_growth_node_with_key_value(key, size, value);
	}
	//incomplete
}


bool Flash_Key_Value_Tree::add_growth_node_with_key_value(const uint32_t key, const  uint8_t size, const uint8_t* value){
	/*	- add new <Key, Value>:
				- First Make sure that the said key doesn't already exist'
				- Find the growth node
				- Create a RAM copy of the new node
				{
					- Find the starting_location_of_unwritten_flash
					- Add a link from the growth node to the new node
					(These two need to be done together because the second)
					(one depends on the first one and vice versa)
				}
				- Write the node to flash at starting_location_of_unwritten_flash */

	Node_Address growth_node_addr = get_active_growth_node_address();
	auto growth_node = Key_Value_Flash_Node::read_node_from_flash(growth_node_addr);
	Node_Address new_node_addr = growth_node.mark_growth_link_VALID_and_return_address_it_points_to();
	Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_growth_node(key, size, value);
	return new_node.write_to_address(new_node_addr);
}

Node_Address Flash_Key_Value_Tree::first_root_node_address(){
	return (Node_Address) (tree_address + 4);
}

bool Flash_Key_Value_Tree::add_first_root_node(uint32_t key, uint8_t size, uint8_t* value){
	Node_Address new_node_addr = first_root_node_address();
	Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_root_node(key, size, value);
	return new_node.write_to_address(new_node_addr);
}

bool Flash_Key_Value_Tree::add_edit_key_value(uint32_t key, uint8_t size, uint8_t* value){
	flash_unlock();

	if (!has_first_node_written()){
		return add_first_root_node(key, size, value);
	}

	if(has_active_root_node() && has_active_growth_node()){
		auto [node_addr, parent_addr] = find_node_and_parent_matching_key(key);
		if(node_addr != 0)
			return update_node(key,size,value,node_addr,parent_addr);
		else
			return add_growth_node_with_key_value(key, size, value);
	}

	flash_lock();

	return false;
}
