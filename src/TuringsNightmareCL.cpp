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

#include <iostream>
#include <vector>
#include <string>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include "TuringsNightmareCL.h"
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
	cl_ulong instruction_ptr;
	cl_ulong step_counter;
	cl_ulong step_limit;

	cl_ulong step_limit_max;
	cl_ulong step_limit_min;

	cl_ulong input_size;
	cl_ulong memory_size;
	union {
		cl_uchar b[200];
		cl_ulong w[25];
	} hs;

	cl_ulong register_a;
	cl_ulong register_b;
	cl_ulong register_c;
	cl_ulong register_d;

	cl_uchar memory[MEMORY_SIZE];
} VM_State;

template<typename T>
inline T TN_GetEntangledType(const VM_State& state) {
	return (T)(state.step_counter ^ state.register_a ^ state.register_b ^ state.register_c ^ state.register_d ^ state.hs.w[state.step_counter % 25] ^ state.hs.b[state.step_counter % 200] ^ state.step_limit ^ state.input_size);
}

#define ENTANGLED_UINT8 TN_GetEntangledType<uint8_t>(state)
#define ENTANGLED_UINT32 TN_GetEntangledType<uint32_t>(state)
#define ENTANGLED_UINT64 TN_GetEntangledType<uint64_t>(state)

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

cl::Device getCLDevice() {
	std::vector<cl::Platform> platform;
	cl::Platform::get(&platform);

	if (platform.empty()) {
		throw std::runtime_error("No OpenCL platforms found.");
	}

	cl::Context context;
	for (auto p = platform.begin(); p != platform.end(); p++) {
		std::vector<cl::Device> pldev;

		p->getDevices(CL_DEVICE_TYPE_GPU, &pldev);

		for (auto d = pldev.begin(); d != pldev.end(); d++) {
			if (!d->getInfo<CL_DEVICE_AVAILABLE>()) continue;

			return *d;
		}

	}
	throw std::runtime_error("No OpenCL device found.");
}

static const char *source =
#include "TuringsNightmareCL.cl"
;

int Turings_NightmareCL(const char* in, const size_t in_len, char* out) {
	if (in_len == 0) { return 0; }
	else if (in_len >= MEMORY_SIZE) { return 0; }

	size_t N = 100;

	VM_State *mem = new VM_State[N];
	VM_State &state = *mem;
	memset(mem, 0, sizeof(VM_State));
	// Copy input to memory (TODO: Blow up with AES? Somehow mess with?)
	state.memory_size = MEMORY_SIZE; // in_len;
	state.input_size = in_len;

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

	for (size_t i = 1; i < N; ++i) {
		memcpy(&mem[i], &mem[0], sizeof(VM_State));
	}

	cl::Device device = getCLDevice();
	cl::Context context = cl::Context(device);

	cl::CommandQueue queue(context, device);
	cl::Program program(context, cl::Program::Sources(1, std::make_pair(source, strlen(source))));

	try {
		program.build("-cl-std=CL2.0");
	}
	catch (const cl::Error &err) {
		std::cerr
			<< "OpenCL error: "
			<< err.what() << "(" << err.err() << ")"
			<< std::endl;

		cl::STRING_CLASS buildLog;
		program.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);

		std::cerr << buildLog << std::endl;

		return 0;
	}

	cl::Kernel tn(program, "Turings_Nightmare");
	cl::Buffer buf(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(VM_State) * N, mem);

	tn.setArg(0, buf);
	tn.setArg(1, N);

	auto start = std::chrono::high_resolution_clock::now();

	try {
		queue.enqueueNDRangeKernel(tn, cl::NullRange, N, cl::NullRange);
		queue.finish();
	}
	catch (const cl::Error &err) {
		std::cerr
			<< "OpenCL error: "
			<< err.what() << "(" << err.err() << ")"
			<< std::endl;
	}

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

	queue.enqueueReadBuffer(buf, CL_TRUE, 0, sizeof(VM_State) * N, (void*)mem);

	std::cout << "GPU inner loop took " << milliseconds << "ms for " << state.step_counter << " steps" << std::endl;

	//print(std::endl << std::string((char*)&state, sizeof(VM_State)));
	//print("State (" << sizeof(VM_State) << ") = \r\n" << std::string((char*)&state, sizeof(VM_State)));
	//print("Ptr " << (uint64_t)state.memory);

	// Hash serialized state for end result
	TN_FinalHash(state, (uint8_t*)mem, sizeof(VM_State), (uint8_t*)out);

	delete[] mem;

	// Profit
	return 1;
}