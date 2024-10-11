#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <set>

#ifdef _DEBUG
#define DEBUG_PRINT(...) \
{ \
	char buffer[256] = {}; \
	sprintf(buffer, __VA_ARGS__); \
	OutputDebugStringA(buffer); \
}
#else
#define DEBUG_PRINT(...)
#endif

class MemLeakFix
{
	static inline uintptr_t* GetMemAllocateAddress();
	static inline uintptr_t* GetMemFreeAddress();
	static inline void** GetGameManagerVtable();
	static inline bool IsCompatibleGameVersion();
public:
	static void Attach();
	static void Detach();
};
