/**
 * Title: Circular Buffer
 * Author: Pragun Goyal, December 31, 2019
 * Description: A simple circular buffer implementation specifically tailored to be used with DMA UART transmission
 * License:
 */


/**
 * Circular buffer data transmission scheme
 *
 * At some generic point in time:
 *
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X||X|X||X|
 * |--- already tx'ed----|--- tx'ing ----|--tx queue --- |---- already tx`ed-----|
 *  0                     s               q               e                       max
 *
 *  Loop invariants:
 *  0: start of array
 *  s: start of tx'ing, data is being sent out in the ongoing tx request
 *  q-1: end of data that is being sent out in the ongoing tx request
 *  q: Start of data waiting to be sent out
 *  e-1: End of data waiting to be sent out
 *  e: Start of writable area
 *
 *  max: size of the array (the last index is max-1)
 *
 *  array[s:q-1]: data that is tx'ing
 *  array[0:s-1, e:max]: new data can be written here
 *  array[q:e-1]: data that is in the buffer waiting to be put on the next sent request
 *
 * Lets assume that at this point, there is a printf to print "cat".
 * Its easy, it would look like this
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
 * |--- already tx'ed----|--- tx'ing ----|--tx queue --------- | already tx`ed --|
 *  0                     s               q                     e                 max
 *
 * Now let us assume that at this point the TX Complete interrupt handler comes and
 * says, hurray! we've finished sending the previous block of data, so it would look
 * like
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
 * |--- already tx'ed--------------------|--tx queue --------- | already tx`ed --|
 *  0                     s               q                     e                 max
 *
 * For just a bit, and then we can dispatch another tx request from `q to `e, and
 * it would look like
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
 * |--- already tx'ed--------------------|--tx'ing ----------- ||already tx`ed --|
 *  0                                     s                     q,e               max
 *
 * Note, that the `q and `e indices are exactly the same, because there is no
 * data waiting in the tx-queue.
 * So, now, lets assume that at this point there is a printf for "Hello World!!!".
 * Which is a longer string than we can hold in the tailing part of the buffer
 *
 * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
 * |tx queue-|--- already tx'ed----------|--tx'ing ----------- |---- tx queue ---|
 *  0         e                           s                     q                 max
 *
 * So then how would we send out the next batch of data?
 * Well, we'd know that when `e < `q, then it must have wrapped itself over
 * we can send the data from `q:`max safely as it is all increasing indices for the
 * TX function. So, whenever we receive the next TX Complete Interrupt, we'd fire
 * off a partial TX like this
 *
 * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
 * |tx queue-|--- already tx'ed------------------------------- |---- tx'ing -----|
 *  0,q     e                                                   s                 max
 *
 *
 *  For new string of size < (max - e)
 *
 */

#include <tuple>
#include "circular_buffer.hpp"
#include <cstring>

template <int16_t max>
int16_t CircularBuffer<max>::length_of_empty_region(){
	//Lets determine where the end of the empty region is
	if (s <= e){
		/**
		 * This means that the buffer looks like
		 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
		 * |--- already tx'ed----|--- tx'ing ----|--tx queue --------- | already tx`ed --|
		 *  0                     s               q                     e                 max
 		 *  */
		return (max - e) + s;
	}

	if (s > e){
		/**
		 * This means that the buffer looks like
		 * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
		 * |tx queue-|--- already tx'ed----------|--tx'ing ----------- |---- tx queue ---|
		 *  0         e                           s                     q                 max
		 *  */
		return (s - e);
	}

	return 0;
}

template <int16_t max>
int16_t CircularBuffer<max>::length_of_ongoing_transmission(){
	//the way this is written right now, the ongoing transmission region should never wrap around
	//hence
	/** either this
	  * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
	  * |tx queue-|--- already tx'ed----------|--tx'ing ----------- |---- tx queue ---|
	  *  0         e                           s                     q                 max
	  *
	  * or
	  *
	  * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
	  * |tx queue-|--- already tx'ed------------------------------- |---- tx'ing -----|
	  *  0,q     e                                                   s                 max
	  *
	 */

	//assert( (s <= q) || (q==0) );
	if(s <= q){
		return (q - s);
	}
	else{
		return (max - s);
	}
}

template <int16_t max>
int16_t CircularBuffer<max>::length_of_queue(){
	//assert( (s <= q) || (q == 0) ); // See note in length_of_ongoing_transmission
	if(q <= e){
		return (e - q);
	}else{
		return (max - q) + e;
	}
}

template <int16_t max>
int16_t CircularBuffer<max>::write_to_queue(char* a, int16_t size){
	//First check if we have enough empty space to write the string
	if ((length_of_empty_region() > size) && (size > 0)){
		if(
			((s <= e)  &&  ((max - e) >= size))
			/**
			 * This means that the buffer looks like
			 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
			 * |--- already tx'ed----|--- tx'ing ----|--tx queue --------- | already tx`ed --|
			 *  0                     s               q                     e                 max
	 		 *  */
			//Yet there is enough space till the end of the array
			//for us to copy the new string

			|| (s >  e)
			/**
			 * This means that the buffer looks like
			 * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
			 * |tx queue-|--- already tx'ed----------|--tx'ing ----------- |---- tx queue ---|
			 *  0         e                           s                     q                 max
			 *  */
		) // if condition, we just need a single memcpy
		{
			std::memcpy(&buffer[e],a,size);
			e = e + size;
		}
		else{
			//assert((s <= e)  &&  ((max - e) < size));
			//we need two memcpys
			uint8_t s1 = (max - e);
			std::memcpy(&buffer[e], a, s1);
			e = (e + s1) % max;

			//assert(e == 0); //at this point
			//In the absence of assert, we'll do this to create a breakpoint for testing purposes
			if(e != 0){
				asm("nop");
			}

			uint8_t s2 = size - s1;
			std::memcpy(&buffer[e], &a[s1],s2);
			e = e + s2;
		}
	}else{
		return 0;
	}

	return 0;
}

template <int16_t max>
std::tuple<char*, int16_t> CircularBuffer<max>::longest_possible_send(){
// this makes sense to be called only when there is nothing that is being sent out
// in other words, right after we've heard acknowledgement that the previous tx has completed
	//assert(s == q);
	int16_t s1 = 0;
	int16_t old_q = q;
	if(q <= e){
		s1 = (e - q);
		q = e;
		return std::make_tuple(&buffer[old_q],s1);
	}else{
		s1 = (max - q);
		q = 0; //wrap around
		return std::make_tuple(&buffer[old_q],s1);
	}
}


template <int16_t max>
void CircularBuffer<max>::send_complete(){
	//this will move s upto q
	s = q;
}

template class CircularBuffer<512>;
