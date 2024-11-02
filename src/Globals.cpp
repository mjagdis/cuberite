
// Globals.cpp

// This file is used for precompiled header generation in MSVC environments

#include "Globals.h"




#ifdef TRACY_ENABLE

void * operator new(decltype(sizeof(0)) count)
{
	auto ptr = malloc(count);
	TracyAllocS(ptr, count, 3);
	return ptr;
}





void operator delete(void * ptr) noexcept
{
	TracyFreeS(ptr, 3);
	free(ptr);
}

#endif
