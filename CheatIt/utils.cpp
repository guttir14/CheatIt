#include "utils.h"
#include <Psapi.h>

bool Compare(byte* data, byte* sig, uint64_t size)
{
    for (auto i = 0ull; i < size; i++) { if (data[i] != sig[i] && sig[i] != 0x00) { return false; } }
    return true;
}

byte* FindSignature(byte* start, byte* end, byte* sig, uint64_t size)
{
    for (auto it = start; it < end - size; it++) { if (Compare(it, sig, size)) { return it; }; }
    return nullptr;
}

void* FindPointer(HMODULE mod, byte* sig, uint64_t size, int addition)
{
    MODULEINFO info;
    if (!K32GetModuleInformation(GetCurrentProcess(), mod, &info, sizeof(MODULEINFO))) { return nullptr; };
    auto base = static_cast<byte*>(info.lpBaseOfDll);
    auto address = FindSignature(base, base + info.SizeOfImage - 1, sig, size);
    if (!address) { return nullptr; }
    auto k = 0;
    for (; sig[k]; k++);
    auto offset = *reinterpret_cast<int*>(address + k);
    return address + k + 4 + offset + addition;
}

bool PatchMem(void* address, void* bytes, uint64_t size)
{
    DWORD oldProtection;
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtection)) { return false; }
    memcpy(address, bytes, size);
    return VirtualProtect(address, size, oldProtection, &oldProtection);
}