#include "pch.h"
#include "patch.h"

void patchNop(void* addr, size_t size) {
    DWORD oldProtect;
    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset(addr, NOP, size);
    VirtualProtect(addr, size, oldProtect, &oldProtect);
}

void patchInst(void* addr, uint8_t inst) {
    DWORD oldProtect;
    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = inst;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchByte(void* addr, uint8_t val) {
    DWORD oldProtect;
    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchBytesM(BYTE* addr, BYTE* val, unsigned int size) {
    DWORD oldProtect;
    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(addr, val, size);
    VirtualProtect(addr, size, oldProtect, &oldProtect);
}

void patchDWord(void* addr, uint32_t val) {
    DWORD oldProtect;
    VirtualProtect(addr, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint32_t*)addr = val;
    VirtualProtect(addr, sizeof(uint32_t), oldProtect, &oldProtect);
}

void patchInt(void* addr, int val) {
    DWORD oldProtect;
    VirtualProtect(addr, sizeof(int), PAGE_EXECUTE_READWRITE, &oldProtect);
    *(int*)addr = val;
    VirtualProtect(addr, sizeof(int), oldProtect, &oldProtect);
}

void patchFloat(void* addr, float val) {
    DWORD oldProtect;
    VirtualProtect(addr, sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect);
    *(float*)addr = val;
    VirtualProtect(addr, sizeof(float), oldProtect, &oldProtect);
}

void patchDouble(void* addr, double val) {
    DWORD oldProtect;
    VirtualProtect(addr, sizeof(double), PAGE_EXECUTE_READWRITE, &oldProtect);
    *(double*)addr = val;
    VirtualProtect(addr, sizeof(double), oldProtect, &oldProtect);
}

void patchCall(void* addr, void* func) {
    DWORD oldProtect;
    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = CALL;
    *(uint32_t*)((uint8_t*)addr + 1) = (uint32_t)func - (uint32_t)addr - 5;
    VirtualProtect(addr, 5, oldProtect, &oldProtect);
}

void patchJmp(void* addr, void* func) {
    DWORD oldProtect;
    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = JMP9;
    *(uint32_t*)((uint8_t*)addr + 1) = (uint32_t)func - (uint32_t)addr - 5;
    VirtualProtect(addr, 5, oldProtect, &oldProtect);
}

void patchThisToCdecl(void* addr, void* func) {
    DWORD oldProtect;
    VirtualProtect(addr, 7, PAGE_EXECUTE_READWRITE, &oldProtect);

    // PUSH ECX (0x51)
    *((uint8_t*)addr) = 0x51;

    // CALL (func)
    *((uint8_t*)addr + 1) = CALL;
    *(uint32_t*)((uint8_t*)addr + 2) = (uint32_t)func - (uint32_t)((uint8_t*)addr + 1) - 5;

    // POP ECX (0x59)
    *((uint8_t*)addr + 6) = 0x59;

    VirtualProtect(addr, 7, oldProtect, &oldProtect);
}

void callFunc(void* addr) {
    // calls function with the assumption of it returning and accepting void
    // void (*fp)() = (*fp)(addr) addr;
    // fp();
}

/*void* copyFunc(uint32_t func_start, uint32_t func_end, void* new_func) {
    uint32_t func_size = func_end - func_start;
    void* copied = (void*)calloc(func_size, 1);
    memcpy(copied, (void*)func_start, func_size);
    patchJmp((void*)func_start, new_func);
    return copied;
}*/
