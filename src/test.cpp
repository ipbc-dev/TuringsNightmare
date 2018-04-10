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

#include "utility.h"
#include "StringTools.h"
#include "TuringsNightmare.h"
#include "TuringsNightmareCL.h"
#include <chrono>
#include <algorithm>
#include <thread>

std::string HashMe(const std::string& data) {
	char hash[HASH_SIZE];
	int retcode = Turings_Nightmare(data.c_str(), data.size(), hash);
	if (retcode == 0) {
		print("Failed to hash! 0 returned.");
		return "[FAIL]";
	}
	return StringTools::toHex(hash, HASH_SIZE);
}

std::string HashMeCL(const std::string& data) {
	char hash[HASH_SIZE];
	int retcode = Turings_NightmareCL(data.c_str(), data.size(), hash);
	if (retcode == 0) {
		print("Failed to hash! 0 returned.");
		return "[FAIL]";
	}
	return StringTools::toHex(hash, HASH_SIZE);
}

void TestHash(const std::string& data) {
	char hash[HASH_SIZE];
	auto start = std::chrono::high_resolution_clock::now();
	int retcode = Turings_Nightmare(data.c_str(), data.size(), hash);
	auto elapsed = std::chrono::high_resolution_clock::now() - start;

	if (retcode == 0) {
		print("Failed to hash! 0 returned.");
	}
	else {
		std::string hashstr = StringTools::toHex(hash, HASH_SIZE);
		long double milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
		print("Hashed " << data.size() << " chars in " << milliseconds << " ms, " << hashstr);
	}
}

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

size_t hash_counter = 0;

void Hash_Thread() {
	print("Thread start");
	for (int x = 0; x < 1000; x++) {
		std::string data = random_string(150);
		HashMe(data);
		hash_counter++;
	}
	print("Thread end");
}

int main(int argc, char* argv[]) {
	std::cout << "Checking sanity... " << std::endl;
	std::string sanity = random_string(50);
	if (HashMe(sanity) != HashMeCL(sanity)) {
		print("SANITY FAIL - OH GOD HELP US");
	}
	else {
		print("Sane");
	}

	/*
	std::string data1 = "dfsf235rawfef4asdfasdfsadf4gdfg";
	TestHash(data1);

	std::string data2 = "asd79f6a09 cz34iowhr9wh7o3znasdfasdfqohwvn978z 7zq8z9r82q78zhr7qw 3h78 3z2 q78gz3872gh2	jlö<k=Q U";
	TestHash(data2);

	std::string data3 = "0";
	TestHash(data3);

	std::string data4 = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
	TestHash(data4);
	*/
}