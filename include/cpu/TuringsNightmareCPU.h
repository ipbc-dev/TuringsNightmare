#ifndef __TURINGS_NIGHTMARE_CPU_H__
#define __TURINGS_NIGHTMARE_CPU_H__
#pragma once

#include "TuringsNightmare.h"

class DeviceCPU {
public:
	const char *name() { return "CPU"; }
	void run(const size_t N, VM_State *states);
};

#endif