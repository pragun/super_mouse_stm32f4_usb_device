#include <cstdio>
#include "reporting_functions.h"

struct RootNode_Node{
	uint8_t validity_bitmask;
	volatile const Key_Reporting_Configuration_Node_Typedef* node_ptrs[8];
	volatile const RootNode_Node* next_tree_root_index_node;
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

namespace SectorState {
	enum SectorState : uint8_t{
		INVALID = 0x00,
		VALID = 0xF0,
		ERASED = 0xFF,
	};
}

extern volatile const Key_Reporting_Configuration_Node_Typedef FirstNode;

[[using gnu : section(".config_sector1") , used]] volatile const uint8_t SectorState1 = SectorState::VALID ;
[[using gnu : section(".config_sector2") , used]] volatile const uint8_t SectorState2 = SectorState::ERASED ;
[[using gnu : section(".config_sector1") , used]] volatile const RootNode_Node FirstRootNode = { 0xFF, {&FirstNode,0,0,0,0,0,0,0}, nullptr};
[[using gnu : section(".config_sector1") , used]] volatile const Key_Reporting_Configuration_Node_Typedef FirstNode = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};



