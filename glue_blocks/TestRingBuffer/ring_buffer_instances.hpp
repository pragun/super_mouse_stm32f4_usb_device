#include "ring_buffer.hpp"

enum class RingState { ClearToWrite, Queued, Sending };

class TrimmedRingBufferforHID {
private:
	RingBuffer<char, uint16_t, 16, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> ring_buf;
public:
	void copy_in_without_rollover(uint16_t n, char* txt);
	void print_state();
	void mark_transferred(uint16_t n);
	char* transfer_out(uint16_t n);
};

class RingBufferforUART {
private:
	RingBuffer<char, uint16_t, 16, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> ring_buf;
public:
	void copy_in_with_rollover(uint16_t n, char* txt);
	void print_state();
	void mark_transferred(uint16_t n);
	std::tuple<char*, uint16_t> transfer_out_next_chunk();
};
