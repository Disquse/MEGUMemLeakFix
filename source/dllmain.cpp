#include "dllmain.h"

namespace
{
std::set<void*> g_allocations{};
bool g_recordAllocations = false;
}

void* PatchVtableFunction(void** vtable, int index, void* method)
{
	DWORD dwOldProtection = 0;
	VirtualProtect(vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &dwOldProtection);

	const auto original = vtable[index];
	vtable[index] = method;

	VirtualProtect(vtable[index], sizeof(void*), dwOldProtection, &dwOldProtection);

	return original;
}

void* PatchOffsetFunction(uintptr_t* address, void* method)
{
	DWORD dwOldProtection = 0;
	VirtualProtect(address, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwOldProtection);

	const auto original = *(uintptr_t**)(address);
	*address = (uintptr_t)method;

	VirtualProtect(address, sizeof(void*), dwOldProtection, &dwOldProtection);

	return original;
}

void* (*g_origPhysicalMem_Allocate)(uint32_t, const char*, int, int);
void* PhysicalMem_Allocate(uint32_t size, const char* fileName, int line, int type)
{
	const auto result = g_origPhysicalMem_Allocate(size, fileName, line, type);

	if (g_recordAllocations)
	{
		g_allocations.insert(result);
	}

	return result;
}

void (*g_origPhysicalMem_Free)(void*);
void PhysicalMem_Free(void* address)
{
	if (g_recordAllocations)
	{
		const auto it = g_allocations.find(address);
		if (it != g_allocations.end())
		{
			g_allocations.erase(it);
		}
	}

	g_origPhysicalMem_Free(address);
}

typedef char(__thiscall* TLoadFuncType)(void*, char, char);
void** g_origGameManager_Load = nullptr;

// The "_edx" argument here is to compensate the difference between "__fastcall" and "__thiscall" calling conventions.
char __fastcall GameManager_Load(void* self, void* _edx, char unk1, char unk2)
{
	DEBUG_PRINT("TGameManagerImplementation::Load(%p, %d, %d)\n", self, unk1, unk2)

	g_recordAllocations = true;
	return ((TLoadFuncType)g_origGameManager_Load)(self, unk1, unk2);
}

typedef char(__thiscall* TUnloadFuncType)(void*, char);
void** g_origGameManager_Unload = nullptr;

// Same as above.
char __fastcall GameManager_Unload(void* self, void* _edx, char unk1)
{
	DEBUG_PRINT("TGameManagerImplementation::Unload(%p, %d)\n", self, unk1)

	const auto result = ((TUnloadFuncType)g_origGameManager_Unload)(self, unk1);
	g_recordAllocations = false;

	DEBUG_PRINT("MemLeakFix: Clearing %d leftovers after unload\n", g_allocations.size());

	// Clear all leftover physical memory.
	for (const auto address : g_allocations)
	{
		PhysicalMem_Free(address);
	}

	g_allocations.clear();

	return result;
}

uintptr_t* MemLeakFix::GetMemAllocateAddress()
{
	// mov     dword ptr [edx+0BCh], offset _PhysicalMem_Allocate
	return (uintptr_t*)(0x409AA9 + 6);
}

uintptr_t* MemLeakFix::GetMemFreeAddress()
{
	// mov     dword ptr [edx+0C4h], offset _PhysicalMem_Free
	return (uintptr_t*)(0x409AC9 + 6);
}

void** MemLeakFix::GetGameManagerVtable()
{
	// TGameManagerImplementation vtable.
	return (void**)0x54A45C;
}

bool MemLeakFix::IsCompatibleGameVersion()
{
	// Rough game version check.
	return (*(uint32_t*)0x4F3410 == 0x33F841D9);
}

void MemLeakFix::Attach()
{
	DEBUG_PRINT("MemLeakFix: Attaching\n");

	if (!IsCompatibleGameVersion())
	{
		DEBUG_PRINT("MemLeakFix: Version check failed\n");
		MessageBox(
			GetActiveWindow(),
			L"GettingUp.MemLeakFix: This game version is not supported. You can continue to play, "
			L"but the mod will not work! Remove \"GettingUp.MemLeakFix.asi\" if you don't need this mod.", 
			L"GettingUp.MemLeakFix", MB_ICONWARNING | MB_OK);

		// Bail out.
		return;
	}

	// Replace "PhysicalMem_Allocate" and "PhysicalMem_Free" functions in "StdInterfaceReset" to track allocations.
	g_origPhysicalMem_Allocate = (decltype(g_origPhysicalMem_Allocate))PatchOffsetFunction(GetMemAllocateAddress(), (void*)&PhysicalMem_Allocate);
	g_origPhysicalMem_Free = (decltype(g_origPhysicalMem_Free))PatchOffsetFunction(GetMemFreeAddress(), (void*)&PhysicalMem_Free);

	// Hook "Load" and "Unload" vtable functions of "TGameManagerImplementation" to handle memory leaks during level unloading.
	g_origGameManager_Load = (decltype(g_origGameManager_Load))PatchVtableFunction(GetGameManagerVtable(), 1, (void*)&GameManager_Load);
	g_origGameManager_Unload = (decltype(g_origGameManager_Unload))PatchVtableFunction(GetGameManagerVtable(), 2, (void*)&GameManager_Unload);

	DEBUG_PRINT("MemLeakFix: Patched the game\n");
}

void MemLeakFix::Detach()
{
	DEBUG_PRINT("MemLeakFix: Detaching\n");

	if (!IsCompatibleGameVersion())
	{
		return;
	}

	// Replace back "PhysicalMem_Allocate" and "PhysicalMem_Free" functions in "StdInterfaceReset" to originals.
	if (g_origPhysicalMem_Allocate != nullptr)
	{
		PatchOffsetFunction(GetMemAllocateAddress(), (void*)g_origPhysicalMem_Allocate);
	}

	if (g_origPhysicalMem_Free != nullptr)
	{
		PatchOffsetFunction(GetMemFreeAddress(), (void*)g_origPhysicalMem_Free);
	}

	// Remove hooked "Load" and "Unload" vtable functions of "TGameManagerImplementation".
	if (g_origGameManager_Load != nullptr)
	{
		PatchVtableFunction(GetGameManagerVtable(), 1, (void**)g_origGameManager_Load);
	}

	if (g_origGameManager_Unload)
	{
		PatchVtableFunction(GetGameManagerVtable(), 2, (void**)g_origGameManager_Unload);
	}

	g_allocations.clear();
	g_recordAllocations = false;

	DEBUG_PRINT("MemLeakFix: Removed all patches\n");
}

BOOL WINAPI DllMain(_In_ void* _DllHandle, _In_ unsigned long _Reason, _In_opt_ void* _Reserved)
{
	if (_Reason == DLL_PROCESS_ATTACH)
	{
		MemLeakFix::Attach();
	}
	else if (_Reason == DLL_PROCESS_DETACH)
	{
		MemLeakFix::Detach();
	}

	return TRUE;
}
