#include "pch.h"
#include "../include/MinHook.h"
#include <windows.h>
#include <thread>
#include <atomic>

#pragma comment (lib, "libMinHook.x86.lib")

typedef int(*originalCall_t)();
originalCall_t originalCall;



int myDetour() {
    typedef int(__cdecl* resetfineaimT)(int a1);
    resetfineaimT resetfineaim = (resetfineaimT)0x9D9FD0;

    int player = *(int*)(0x21703D4);
    // menu_status = 2 gameplay
    uint8_t* menu_status = (uint8_t*)0x00EBE860;
    uint8_t* statusaim = (uint8_t*)0x02347AFA;

    if (*menu_status == 2 && *statusaim == 0) {
        resetfineaim(player);
    }

    return originalCall();
}

void setupHook() {
    if (MH_Initialize() != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO INITIALIZE", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (MH_CreateHook((LPVOID)0x520450, &myDetour, (LPVOID*)&originalCall) != MH_OK) { //Tervel's hook
        MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    //MessageBoxW(NULL, L"YOU WILL NEVER ENCOUNTER THIS", L"Success", MB_OK | MB_ICONINFORMATION);
}

void cleanupHook() {
    MH_DisableHook((LPVOID)0x520450);
    MH_Uninitialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        setupHook();
        break;
    case DLL_PROCESS_DETACH:
        cleanupHook();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    default:
        break;
    }
    return TRUE;
}
