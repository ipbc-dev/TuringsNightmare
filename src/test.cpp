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

#include "misc/utility.h"
#include "misc/StringTools.h"

#include "TuringsNightmare.h"
#include "cpu/TuringsNightmareCPU.h"
#include "opencl/TuringsNightmareCL.h"
#include "cuda/TuringsNightmareCUDA.h"

#include <chrono>
#include <algorithm>

std::string random_string(size_t length)
{
	auto randchar = []() -> char
	{
		const char charset[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};
	std::string str(length, 0);
	std::generate_n(str.begin(), length, randchar);
	return str;
}

void TestTNSanity(const std::string &input) {
	VM_State *tnCPU = TN_VM_Init(input.c_str(), input.length());
	VM_State *tnCUDA = TN_VM_Init(input.c_str(), input.length());
	VM_State *tnCL = TN_VM_Init(input.c_str(), input.length());

	char cpuHash[HASH_SIZE], cudaHash[HASH_SIZE], clHash[HASH_SIZE];

	DeviceCPU cpu;
	cpu.run(1, tnCPU);

	TN_VM_Finalize(tnCPU, cpuHash);

	std::cout << "Sanity checking CUDA... ";
	std::cout.flush();

	DeviceCUDA cuda;
	cuda.run(1, tnCUDA);
	TN_VM_Finalize(tnCUDA, cudaHash);
	if (strncmp(cpuHash, cudaHash, HASH_SIZE)) {
		std::cout << "FAILED!!!" << std::endl;
	} else {
		std::cout << "Sane" << std::endl;
	}

	std::cout << "Sanity checking OpenCL... ";
	std::cout.flush();

	DeviceCL cl;
	cl.run(1, tnCL);
	TN_VM_Finalize(tnCL, clHash);
	if (strncmp(cpuHash, cudaHash, HASH_SIZE)) {
		std::cout << "FAILED!!!" << std::endl;
	}
	else {
		std::cout << "Sane" << std::endl;
	}
}

template <typename Device> void TestTNSpeed(const size_t N, const bool divergent, const std::string &input) {
	Device dev;

	VM_State *base = TN_VM_Init(input.c_str(), input.length());

	VM_State *mem = new VM_State[N];
	for (size_t i = 0; i < N; ++i) {
		memcpy(mem + i, base, sizeof(VM_State));
		if (divergent) mem[i].memory[0] ^= i;
	}

	auto start = std::chrono::high_resolution_clock::now();

	dev.run(N, mem);

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

	std::cout << dev.name() << " running " << N << " instances took " << milliseconds << "ms" << std::endl;
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main(int argc, char* argv[]) {
	std::string input = random_string(50);

	TestTNSanity(input);

	size_t sizes[] = { 1, 5, 10, 20 };

	std::cout << std::endl << "Running speed tests (no divergence)" << std::endl;
	for (auto N : sizes) {
		std::cout << std::endl;
		TestTNSpeed<DeviceCPU>(N, false, input);
		TestTNSpeed<DeviceCUDA>(N, false, input);
		TestTNSpeed<DeviceCL>(N, false, input);
	}

	std::cout << std::endl << "Running speed tests (divergent)" << std::endl;
	for (auto N : sizes) {
		std::cout << std::endl;
		TestTNSpeed<DeviceCPU>(N, true, input);
		TestTNSpeed<DeviceCUDA>(N, true, input);
		TestTNSpeed<DeviceCL>(N, true, input);
	}
}