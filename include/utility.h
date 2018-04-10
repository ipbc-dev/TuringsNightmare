#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

#define print(msg) {std::stringstream ss; ss << msg << std::endl; std::cout << ss.str(); }

void inline sleep(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}