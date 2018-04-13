#include "cpu/TuringsNightmareCPU.h"

// TODO: cleanup utility dependencies
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>

template<typename T>
inline T TN_GetEntangledType(const VM_State& state) {
	return (T)(state.step_counter ^ state.register_a ^ state.register_b ^ state.register_c ^ state.register_d ^ state.hs.w[state.step_counter % 25] ^ state.hs.b[state.step_counter % 200] ^ state.step_limit ^ state.input_size);
}

#define ENTANGLED_UINT8 TN_GetEntangledType<uint8_t>(state)
#define ENTANGLED_UINT32 TN_GetEntangledType<uint32_t>(state)
#define ENTANGLED_UINT64 TN_GetEntangledType<uint64_t>(state)

inline uint8_t& TN_AtRelPos(VM_State& state, int position) {
	size_t pos = state.instruction_ptr + position;
	if (pos >= state.memory_size) pos %= state.memory_size;
	return state.memory[pos];
}

#define MEM(relpos) TN_AtRelPos(state, relpos)

inline void TN_AdjustCycleLimit(VM_State& state, int change) {
	state.step_limit += change;

	if (state.step_limit < state.step_limit_min) state.step_limit = state.step_limit_min;
	else if (state.step_limit > state.step_limit_max) state.step_limit = state.step_limit_max;
}

#define MODCYCLES(change) TN_AdjustCycleLimit(state, change)

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
	case NOOP:
		break;
	}
}

inline VM_Instruction TN_GetInstruction(const VM_State& state) {
	return (VM_Instruction)((state.memory[state.instruction_ptr] ^ ENTANGLED_UINT64) % _LAST);
}

void DeviceCPU::run(const size_t N, VM_State *states) {
	std::vector<std::thread> threads;
	for (size_t i = 0; i < N; ++i) {
		threads.emplace_back([](VM_State *state) {
			for (; state->step_counter <= state->step_limit; state->step_counter++) {
				VM_Instruction inst = TN_GetInstruction(*state);
				TN_ParseInstruction(*state, inst);
				state->instruction_ptr = (state->instruction_ptr + 1) % state->memory_size;
			}
		}, states + i);
	}
	for (auto &t : threads) t.join();
}