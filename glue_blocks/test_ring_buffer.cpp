/**
 * Trim Enabled Ring Buffer Test
---
**/

#include <cstring>
#include "circular_buffers.hpp"
//#include "ring_buffer_instances.hpp"

void test2() {
	printf("\n\n\nTest1\n");
	printf("\nTesting trim enabled\n");
	HIDContinuousBlockCircularBuffer b;
	printf("B Pointer:%p\n", (void*)&b);

	//Initial Buffer Condition
	b.print_state(); //= > Empty

	b.copy_in_without_rollover(10, "TestString"); //Write TestString
	b.print_state(); // = > Queued(TestString, 10), Empty(6)
		
	b.transfer_out(4);//	Put 4 to Sending
	b.print_state(); //	= > Sending(Test, 4), Queued(String, 6), Empty(6)

	b.mark_transferred(4);	// Put 4 to Clear
	b.print_state(); // = > Empty(4), Queued(String, 6), Empty(6)

	b.copy_in_without_rollover(4, "Rock"); //Write Rock
	b.print_state(); // = > Empty(4), Queued(StringRock, 10), Empty(2)

	b.copy_in_without_rollover(4, "Dock");	// Write Dock,
	b.print_state(); //= > Queued(Dock, 4), Empty(0), Queued(StringRock, 10) :Trimmed to 14

	b.transfer_out(10);//	Put 10 to Sending
	b.print_state(); // = > Queued(Dock, 4), Empty(0), Sending(StringRock, 10)

	b.mark_transferred(10); //	10 to Clear
	b.print_state(); //  = > Queued(Dock, 4), Empty(10),

	b.copy_in_without_rollover(4, "Frog"); //	Write Frog
	b.print_state(); // = > Queued(DockFrog, 8), Empty(8) :Untrimmed back to 16

	b.transfer_out(6); //		6 to Sending
	b.print_state(); //  = > Sending(DockFr, 6), Queued(og, 2), Empty(8)

	b.mark_transferred(4); //	4 to Clear
	b.print_state(); // = > Empty(4), Sending(Fr, 2), Queued(og, 2), Empty(8)
}


void test1() {
	printf("\n\n\nTest2\n");
	printf("\nTesting trim diabled\n");
	UART_Tx_CircularBuffer b;
	printf("B Pointer:%p\n", (void*)&b);

	//Initial Buffer Condition
	b.print_state(); //= > Empty

	b.write_to_queue("TestString", 10); //Write TestString
	b.print_state(); // = > Queued(TestString, 10), Empty(6)

	b.longest_possible_send();//	Put 4 to Sending
	b.print_state(); //	= > Sending(Test, 4), Queued(String, 6), Empty(6)

	b.mark_transferred(4);	// Put 4 to Clear
	b.print_state(); // = > Empty(4), Queued(String, 6), Empty(6)

	b.write_to_queue("Rock", 4); //Write Rock
	b.print_state(); // = > Empty(4), Queued(StringRock, 10), Empty(2)

	b.write_to_queue("Dock", 4);	// Write Dock,
	b.print_state(); //= > Queued(Dock, 4), Empty(0), Queued(StringRock, 10) :Trimmed to 14

	b.longest_possible_send();//	Put 10 to Sending
	b.print_state(); // = > Queued(Dock, 4), Empty(0), Sending(StringRock, 10)

	b.mark_transferred(10); //	10 to Clear
	b.print_state(); //  = > Queued(Dock, 4), Empty(10),

	b.write_to_queue("Frog", 4); //	Write Frog
	b.print_state(); // = > Queued(DockFrog, 8), Empty(8) :Untrimmed back to 16

	b.longest_possible_send(); //		6 to Sending
	b.print_state(); //  = > Sending(DockFr, 6), Queued(og, 2), Empty(8)

	b.mark_transferred(4); //	4 to Clear
	b.print_state(); // = > Empty(4), Sending(Fr, 2), Queued(og, 2), Empty(8)
}
int main(){
	test2();
	test1();
}


