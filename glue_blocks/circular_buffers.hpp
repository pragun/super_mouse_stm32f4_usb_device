#include <tuple>
#include "ring_buffer.hpp"

enum class RingState { ClearToWrite, Queued, Sending };

constexpr uint16_t uart_buffer_size = 512;
constexpr uint16_t hid_buffer_size = 64;

/*
template <int16_t max>
class UART_Tx_CircularBuffer
{
	private:
	char buffer[max];
	int16_t s, q, e; //See above text for a better understanding of what these do
	// s: sending, q: queued, e: empty

	public:
	UART_Tx_CircularBuffer():s(0),q(0),e(0){};

	int16_t length_of_empty_region();
	int16_t length_of_ongoing_transmission();
	int16_t length_of_queue();

	int16_t write_to_queue(char* a, int16_t);
	std::tuple<char*, int16_t> longest_possible_send();
	void send_complete();

}; */

class HIDContinuousBlockCircularBuffer {
private:
	RingBuffer<char, uint16_t, hid_buffer_size, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> ring_buf;
public:
	void copy_in_without_rollover(uint16_t n, char* txt);
	void print_state();
	void mark_transferred(uint16_t n);
	char* transfer_out(uint16_t n);
};

class UART_Tx_CircularBuffer {
private:
	RingBuffer<char, uint16_t, uart_buffer_size, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> ring_buf;

public:

	uint16_t length_of_empty_region();
	uint16_t length_of_ongoing_transmission();
	uint16_t length_of_queue();

	std::tuple<char*, uint16_t> longest_possible_send();
	uint16_t write_to_queue(char* txt, uint16_t n);
	void print_state();
	uint16_t mark_transferred(uint16_t n);
	uint16_t last_send_complete();

};
