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
#include "cuda/TuringsNightmareCUDA.h"

#include <cstring>

// TODO: cleanup utility dependencies
#include <chrono>
#include <iostream>

#define ENTANGLED_UINT8 (uint8_t)(state->step_counter ^ state->register_a ^ state->register_b ^ state->register_c ^ state->register_d ^ state->hs.w[state->step_counter % 25] ^ state->hs.b[state->step_counter % 200] ^ state->step_limit ^ state->input_size)
#define ENTANGLED_UINT32 (uint32_t)(state->step_counter ^ state->register_a ^ state->register_b ^ state->register_c ^ state->register_d ^ state->hs.w[state->step_counter % 25] ^ state->hs.b[state->step_counter % 200] ^ state->step_limit ^ state->input_size)
#define ENTANGLED_UINT64 (uint64_t)(state->step_counter ^ state->register_a ^ state->register_b ^ state->register_c ^ state->register_d ^ state->hs.w[state->step_counter % 25] ^ state->hs.b[state->step_counter % 200] ^ state->step_limit ^ state->input_size)

__device__
inline uint8_t* TN_AtRelPos(VM_State *state, int position) {
	uint64_t pos = state->instruction_ptr + position;
	if (pos >= state->memory_size) pos %= state->memory_size;
	return state->memory + pos;
}

#define MEM(relpos) *TN_AtRelPos(state, relpos)

__device__
inline void TN_AdjustCycleLimit(VM_State *state, int change) {
	state->step_limit += change;

	if (state->step_limit < state->step_limit_min) state->step_limit = state->step_limit_min;
	else if (state->step_limit > state->step_limit_max) state->step_limit = state->step_limit_max;
}

#define MODCYCLES(change) TN_AdjustCycleLimit(state, change)

__device__
inline void TN_ParseInstruction(VM_State *state, VM_Instruction inst) {
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
		state->instruction_ptr = state->instruction_ptr * ENTANGLED_UINT64 % state->memory_size;
		break;
	case JUMP:
		state->instruction_ptr = ((state->instruction_ptr + ((ENTANGLED_UINT8 % 200) - 100)) % state->memory_size);
		break;
	case REGA_XOR:
		MEM(0) ^= state->register_a;
		state->register_a ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case REGB_XOR:
		MEM(0) ^= state->register_b;
		state->register_b ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case REGC_XOR:
		MEM(0) ^= state->register_c;
		state->register_c ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case REGD_XOR:
		MEM(0) ^= state->register_d;
		state->register_d ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
		break;
	case CYCLEADD:
		MODCYCLES(1 * ENTANGLED_UINT8);
		break;
	case CYCLESUB:
		MODCYCLES(-1 * ENTANGLED_UINT8);
		break;
	default:
	case NOOP:
		break;
	}
}

__device__
VM_Instruction TN_GetInstruction(VM_State *state) {
	return (VM_Instruction)((state->memory[state->instruction_ptr] ^ ENTANGLED_UINT64) % _LAST);
}

__global__
void TN_VM_Execute_CUDA(VM_State *mem) {
	VM_State *state = &mem[threadIdx.x];
	for (; state->step_counter <= state->step_limit; state->step_counter++) {
		VM_Instruction inst = TN_GetInstruction(state);
		TN_ParseInstruction(state, inst);
		state->instruction_ptr = (state->instruction_ptr + 1) % state->memory_size;
	}
}

void DeviceCUDA::run(const size_t N, VM_State *states) {
	VM_State *buf;
	cudaMalloc((void**)&buf, sizeof(VM_State) * N);
	cudaMemcpy(buf, states, sizeof(VM_State) * N, cudaMemcpyHostToDevice);

	dim3 dimBlock(N, 1);
	dim3 dimGrid(1, 1);

	TN_VM_Execute_CUDA<<<dimGrid, dimBlock>>>(buf);

	cudaDeviceSynchronize();
	cudaMemcpy(states, buf, sizeof(VM_State) * N, cudaMemcpyDeviceToHost);
	cudaFree(buf);
}