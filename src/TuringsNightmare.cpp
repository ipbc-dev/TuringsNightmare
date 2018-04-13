/*
Copyright 2018 Interplanetary Broadcast Coin SL

This file is part of Turings Nightmare
Authors: Fritjof Harms, Markus Behm

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "TuringsNightmare.h"
#include <cstring>
#include <stdexcept>

extern "C" {
#include "crypto/jh.h"
#include "crypto/blake256.h"
#include "crypto/groestl.h"
#include "crypto/keccak.h"
}

VM_State *TN_VM_Init(const char *in, const size_t in_len) {
	if (in_len == 0 || in_len >= MEMORY_SIZE) {
		throw std::runtime_error("Invalid TN input size.");
	}

	VM_State *state = new VM_State;
	memset(state, 0, sizeof(VM_State) - MEMORY_SIZE);

	state->memory_size = MEMORY_SIZE;
	state->step_limit_max = state->memory_size * MAX_CYCLES;
	state->step_limit_min = state->memory_size * MIN_CYCLES;
	state->step_limit = state->memory_size * NRM_CYCLES;

	// Copy input to memory (TODO: Blow up with AES? Somehow mess with?)
	size_t blocks = MEMORY_SIZE / in_len;
	for (size_t i = 0; i < blocks; ++i) memcpy(state->memory + i * in_len, in, in_len);
	size_t filled = blocks * in_len;
	if (filled < MEMORY_SIZE) memcpy(state->memory + filled, in, MEMORY_SIZE - filled);

	// Keccak state (TODO: Use for blow up? Do rounds on data?)
	keccak1600(state->memory, MEMORY_SIZE, state->hs.b);

	return state;
}

template<typename T>
inline T TN_GetEntangledType(const VM_State& state) {
	return (T)(state.step_counter ^ state.register_a ^ state.register_b ^ state.register_c ^ state.register_d ^ state.hs.w[state.step_counter % 25] ^ state.hs.b[state.step_counter % 200] ^ state.step_limit ^ state.input_size);
}

#define ENTANGLED_UINT8 TN_GetEntangledType<uint8_t>(state)

inline void TN_FinalHash(const VM_State& state, const uint8_t* data, const size_t data_len, uint8_t* out) {
	switch (ENTANGLED_UINT8 % 3) {
	case 0:
		jh_hash(HASH_SIZE * 8, data, 8 * data_len, (uint8_t*)out);
		break;
	case 1:
		blake256_hash((uint8_t*)out, data, data_len);
		break;
	case 2:
		groestl(data, (data_len * 8), (uint8_t*)out);
		break;
	}
}

void TN_VM_Finalize(const VM_State *state, char *out) {
	// Hash serialized state for end result
	TN_FinalHash(*state, (uint8_t*)state, sizeof(VM_State), (uint8_t*)out);

	delete state;
}