/**
 * Title: Circular Buffer
 * Author: Pragun Goyal, December 31, 2019
 * Description: A simple circular buffer implementation specifically tailored
 * to be used with memory constrained environments (like the STM32) without
 * malloc (or where using malloc is not the best idea).
 * License:
 */

#include <tuple>
#include "ring_buffer.hpp"

enum class RingState { ClearToWrite, Queued, Sending };

#ifdef TESTING //This should be set by CMAKE when this is built for unit-testing purposes
constexpr uint16_t uart_buffer_size = 16;
constexpr uint16_t hid_buffer_size = 16;
#else
constexpr uint16_t uart_buffer_size = 512;
constexpr uint16_t hid_buffer_size = 64;
#endif

/*
Note on HIDContinuousBlockCircularBuffer:
1. This stores each report in a contiguous section of the queue.
2. If the ClearToWrite segment of the queue wraps over, and the requested
   size is only available in the second lobe of the ClearToWrite segment.
   The buffer_size will be temporarily trimmed and the second lobe of
   the ClearToWriteSegment will be used.
3. Reports can be variable sized. For each report,
   <uint16_t> (2 bytes) sized space is used in the queue before the report is 
   written to the queue.
4. These (2 bytes) are used to see how many bytes should be read off
   the queue to read exactly the complete report.
*/

class HIDContinuousBlockCircularBuffer {
private:
	RingBuffer<char, uint16_t, hid_buffer_size, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> ring_buf;
public:
	uint16_t copy_in_report(uint16_t n, char* txt);
	void print_state();
	uint16_t mark_transferred(uint16_t n);
	uint16_t last_send_complete();
	std::tuple<char*, uint16_t> transfer_out_next_report();
};

class UART_Tx_CircularBuffer {
private:
	RingBuffer<char, uint16_t, uart_buffer_size, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> ring_buf;

public:
	/* No longer needed as the USRT TX checks on HAL_DMA state to make sure that it is ok to start a transfer
	uint16_t length_of_empty_region();
	uint16_t length_of_ongoing_transmission();
	uint16_t length_of_queue();
	*/

	std::tuple<char*, uint16_t> longest_possible_send();
	uint16_t write_to_queue(char* txt, uint16_t n);
	void print_state();
	uint16_t mark_transferred(uint16_t n);
	uint16_t last_send_complete();

};
