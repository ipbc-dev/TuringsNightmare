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
#ifndef __TURINGS_NIGHTMARE_H__
#define __TURINGS_NIGHTMARE_H__
#pragma once

#include <cstdint>

#define HASH_SIZE 32

#define MEMORY_SIZE 1024 * 1024 * 1 
//#define MEMORY_SIZE 150

#define MIN_CYCLES 1
#define NRM_CYCLES 2
#define MAX_CYCLES 4

union hash_state {
	uint8_t b[200];
	uint64_t w[25];
};

typedef struct {
	uint64_t instruction_ptr;
	uint64_t step_counter;
	uint64_t step_limit;

	uint64_t step_limit_max;
	uint64_t step_limit_min;

	uint64_t input_size;
	uint64_t memory_size;
	hash_state hs;

	uint64_t register_a;
	uint64_t register_b;
	uint64_t register_c;
	uint64_t register_d;

	uint8_t memory[MEMORY_SIZE];
} VM_State;

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

VM_State *TN_VM_Init(const char *in, const size_t in_len);
void TN_VM_Finalize(const VM_State *state, char *out);

#endif