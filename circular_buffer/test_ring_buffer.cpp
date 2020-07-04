/**
 * Trim Enabled Ring Buffer Test
---
**/

#include <cstring>
#include "circular_buffers.hpp"
//#include "ring_buffer_instances.hpp"


/**

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

**/



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

void test3() {
	printf("\n\n\nTest1\n");
	printf("\nTesting trim enabled\n");
	HIDContinuousBlockCircularBuffer b;
	printf("B Pointer:%p\n", (void*)&b);

	//Initial Buffer Condition
	b.print_state(); //= > Empty

	/* Note on notation:
	RingState("a,string...",b),...
	a is the length of the next string-segment
	b is the total length of this RingState. So for example
	Queued("4,Test,6,String", 14), Empty(2)
	Would mean, that there are 14 bytes in queued, which includes
	2 bytes for 4 -- which denotes the length of "Test", followed by
	2 bytes for 6 -- which denotes the length of "String". 
	Making it a total of 2+4+2+6 = 14.
	*/

	b.copy_in_report(4, "Test"); //Write Test
	b.print_state(); // = > Queued("4,Test" , 6), Empty(10)

	b.copy_in_report(6, "String"); //Write String
	b.print_state(); // = > Queued("4,Test,6,String", 14), Empty(2)

	{ auto [c1, s1] = b.transfer_out_next_report(); 
	printf("Next report:%s, size:%d\n", c1, s1); }
	b.print_state(); //	= > Sending("4,Test", 6), Queued("6,String", 8), Empty(2)

	b.mark_transferred(4);
	b.print_state(); //	= > Empty(6), Queued("6,String", 8), Empty(2)

	b.copy_in_report(4, "Rock"); //Write Rock
	b.print_state(); // = > Queued("4,Rock",6) , Queued("6,String", 8): Trimmed to 14

	{ auto [c1, s1] = b.transfer_out_next_report();//	Put 4 to Sending
	printf("Next report:%s, size:%d\n", c1, s1); } 
	b.print_state(); // =>  // Queued("4,Rock", 6), Sending("6,String", 8)

	b.copy_in_report(4, "Frog"); //	Write Frog
	b.print_state(); // Should not work, as there is no empty space

	b.mark_transferred(6);
	b.print_state(); // = > Queued("4,Rock",6), Empty(8) 
	
	b.copy_in_report(4, "Frog"); //	Write Frog
	b.print_state(); // = > Queued("4,Rock,4,Frog",12), Empty(4)  : Untrimmed to 16

}


int main(){
	//test2();
	//test1();
	test3();
}


