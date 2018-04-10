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
#include "utility.h"

extern "C" {
#include "jh.h"
#include "blake256.h"
#include "groestl.h"
#include "keccak.h"
}

#define MEMORY_SIZE 1024 * 1024 * 1 
//#define MEMORY_SIZE 150

#define MIN_CYCLES 1
#define NRM_CYCLES 2
#define MAX_CYCLES 10

typedef struct {
	uint64_t instruction_ptr;
	size_t step_counter;
	size_t step_limit;

	size_t step_limit_max;
	size_t step_limit_min;

	size_t input_size;
	size_t memory_size;
	hash_state hs;
	
	uint64_t register_a;
	uint64_t register_b;
	uint64_t register_c;
	uint64_t register_d;

	uint8_t memory[MEMORY_SIZE];
} VM_State;

template<typename T>
inline T TN_GetEntangledType(const VM_State& state) {
	return (T)(state.step_counter ^ state.register_a ^ state.register_b ^ state.register_c ^ state.register_d ^ state.hs.w[state.step_counter % 25] ^ state.hs.b[state.step_counter % 200] ^ state.step_limit ^ state.input_size);
}

#define ENTANGLED_UINT8 TN_GetEntangledType<uint8_t>(state)
#define ENTANGLED_UINT32 TN_GetEntangledType<uint32_t>(state)
#define ENTANGLED_UINT64 TN_GetEntangledType<uint64_t>(state)

inline uint8_t& TN_AtRelPos(VM_State& state, int position) {
	long int pos = state.instruction_ptr;
	pos += position;
	if (pos >= state.memory_size) pos = pos % state.memory_size; // TODO: Sign mismatch :< Better wrapping!
	while (pos < 0) { pos += state.memory_size; }

	if (pos < 0 || pos > state.memory_size) {
		print("AtRelPos fucked up! " << pos);
	}

	return state.memory[pos];
}

#define MEM(relpos) TN_AtRelPos(state, relpos)

inline void TN_AdjustCycleLimit(VM_State& state, int change) {
	//print("Pre: " << state.step_limit << " " << change);
	state.step_limit += change;

	if (state.step_limit < state.step_limit_min) state.step_limit = state.step_limit_min;
	else if (state.step_limit > state.step_limit_max) state.step_limit = state.step_limit_max;
	//print("Post: " << state.step_limit << " -- " << state.step_limit_min << " - " << state.step_limit_max << " (" << state.step_counter << ")" );
}

#define MODCYCLES(change) TN_AdjustCycleLimit(state, change)

inline void TN_FinalHash(const VM_State& state, const uint8_t* data, const size_t data_len, uint8_t* out) {
	switch (ENTANGLED_UINT8 % 3) {
	default:
		print("Hash WTF???");
	case 0:
		//print("JH");
		jh_hash(HASH_SIZE * 8, data, 8 * data_len, (uint8_t*)out);
		break;
	case 1:
		//print("Blake");
		blake256_hash((uint8_t*)out, data, data_len);
		break;
	case 2:
		//print("Groestl");
		groestl(data, (data_len * 8), (uint8_t*)out);
		break;
	}
}

typedef enum {
	NOOP = 0,
	XOR,
	XOR2,
	XOR3,
	DIV,
	ADD,
	SUB,
	INSTPTR,
	JUMP,
	REGA_XOR,
	REGB_XOR,
	REGC_XOR,
	REGD_XOR,
	CYCLEADD,
	CYCLESUB,
	_LAST
} VM_Instruction;

inline void TN_ParseInstruction(VM_State& state, VM_Instruction inst) {
	switch (inst) {
	case XOR:
		MEM(0) ^= MEM(MEM(MEM(-1)));
		break;
	case XOR2:
		MEM(1) ^= MEM(2);
		MEM(0) ^= MEM(1);
		break;
	case XOR3:
		MEM(0) ^= MEM(ENTANGLED_UINT8);
		break;
	case DIV:
		MEM(0) = MEM(0) ^ (MEM(1) / (MEM(ENTANGLED_UINT8) + 1));
		break;
	case ADD:
		MEM(1) += MEM(2);
		MEM(0) += MEM(1);
		break;
	case SUB:
		MEM(1) -= MEM(2);
		MEM(0) -= MEM(1);
		break;
	case INSTPTR:
		state.instruction_ptr = state.instruction_ptr * ENTANGLED_UINT64 % state.memory_size;
		break;
	case JUMP:
		state.instruction_ptr = ((state.instruction_ptr + ((ENTANGLED_UINT8 % 200) - 100)) % state.memory_size);
		break;
	case REGA_XOR:
		MEM(0) ^= state.register_a;
		state.register_a ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case REGB_XOR:
		MEM(0) ^= state.register_b;
		state.register_b ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case REGC_XOR:
		MEM(0) ^= state.register_c;
		state.register_c ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case REGD_XOR:
		MEM(0) ^= state.register_d;
		state.register_d ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case CYCLEADD:
		MODCYCLES(1 * ENTANGLED_UINT8);
		break;
	case CYCLESUB:
		MODCYCLES(-1 * ENTANGLED_UINT8);
		break;
	default:
		print("Illegal Instruction " << (int)inst);
	case NOOP:
		break;
	}
}

inline VM_Instruction TN_GetInstruction(const VM_State& state) {
	return (VM_Instruction)((state.memory[state.instruction_ptr] ^ ENTANGLED_UINT64) % _LAST);
}

int Turings_Nightmare(const char* in, const size_t in_len, char* out) {
	if (in_len == 0) { return 0; }
	else if (in_len >= MEMORY_SIZE) { return 0; }

	VM_State *mem = new VM_State;
	VM_State &state = *mem;
	memset(mem, 0, sizeof(VM_State));
	// Copy input to memory (TODO: Blow up with AES? Somehow mess with?)
	state.memory_size = MEMORY_SIZE; // in_len;
	state.input_size = in_len;

	// Copy input to memory (TODO: Blow up with AES? Somehow mess with?)
	// state.memory_size = MEMORY_SIZE; // in_len;
	// state.memory = new uint8_t[state.memory_size];

	int times = MEMORY_SIZE / in_len;
	for (int x = 0; x < times; x++) {
		memcpy(state.memory + (x * in_len), in, in_len);
	}
	size_t filled = times * in_len;
	if (filled != state.memory_size) {
		memcpy(state.memory + filled, in, state.memory_size - filled);
	}
	//print("Memory: " << std::string((char*)state.memory, state.memory_size));
	
	// Keccak state (TODO: Use for blow up? Do rounds on data?)
	keccak1600(state.memory, state.memory_size, state.hs.b);

	// Set step limits
	// state.step_counter = 0;
	state.step_limit_max = state.memory_size * MAX_CYCLES;
	state.step_limit = state.memory_size * NRM_CYCLES;
	state.step_limit_min = state.memory_size * MIN_CYCLES;

	auto start = std::chrono::high_resolution_clock::now();
	
	// Execute
	for (; state.step_counter <= state.step_limit; state.step_counter++) {
		VM_Instruction inst = TN_GetInstruction(state); // Get instruction for current state.
		//print("Step: " << state.step_counter << " / " << state.step_limit << " InstPtr: " << state.instruction_ptr << " AtLoc: " << std::to_string(state.memory[state.instruction_ptr]) << " (" << state.memory[state.instruction_ptr] << ") Inst: " << (int)inst);
		TN_ParseInstruction(state, inst); // Parse instruction
		state.instruction_ptr = (state.instruction_ptr + 1) % state.memory_size; // Move on to next command
	}

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	std::cout << "CPU inner loop took " << milliseconds << "ms for " << state.step_counter << " steps" << std::endl;

	//print(std::endl << std::string((char*)&state, sizeof(VM_State)));
	//print("State (" << sizeof(VM_State) << ") = \r\n" << std::string((char*)&state, sizeof(VM_State)));
	//print("Ptr " << (uint64_t)state.memory);

	// Hash serialized state for end result
	TN_FinalHash(state, (uint8_t*)mem, sizeof(VM_State), (uint8_t*)out);

	delete mem;

	// Profit
	return 1;
}