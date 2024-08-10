#include "pch.h"
#include "../include/MinHook.h"
#include "patch.h"
#include "ini.h"
#include <thread>
#include <atomic>
#include <string>
#include <unordered_map>
#include <windows.h>
#include <filesystem>
#include <iostream> // Include for console output
#include <algorithm> // Include for std::transform

#pragma comment(lib, "libMinHook.x86.lib")
#pragma optimize("", off)
typedef int(*originalCall_t)();
originalCall_t originalCall;

std::atomic<bool> g_running(true); // Atomic flag to control the loop

std::unordered_map<std::string, uint8_t> lastAppliedValues; // Store last applied values to avoid console spam
std::unordered_map<std::string, std::unordered_map<std::string, uint8_t>> controlsMap; // Store controls for each state

// Convert a string to lowercase
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::unordered_map<std::string, uintptr_t> addressMap = {
    {"action", 0x0234E79C},
    {"attackprimary", 0x0234E894},
    {"attacksecondary", 0x0234E89C},
    {"recruitdismissfollowers", 0x0234E91C},
    {"cancelactivity", 0x0234E924},
    {"showmap", 0x0234E81C},
    {"pausemenu", 0x0234E824},
    {"startbutton", 0x0234EA04},
    {"radialmenu", 0x0234E7AC},
    {"menuselect", 0x0234E96C},
    {"jump", 0x0234E7A4},
    {"sprint", 0x0234E80C},
    {"reload", 0x0234E7EC},
    {"crouch", 0x0234E7F4},
    {"fineaim", 0x0234E7FC},
    {"grabhuman", 0x0234E804},
    {"taunt", 0x0234E92C},
    {"compliment", 0x0234E934},
    {"zoomin", 0x0234E98C},
    {"zoomout", 0x0234E994},
    {"fightclubaction3", 0x0234E94C},
    {"fightclubaction4", 0x0234E954},
    {"fightambulanceaction1", 0x0234E95C},
    {"fightambulanceaction2", 0x0234E964},
    {"drivingaccelerate", 0x0234E8A4},
    {"brake", 0x0234E8AC},
    {"handbrake", 0x0234E7B4},
    {"cruisecontrol", 0x0234E854},
    {"horn", 0x0234E84C},
    {"nextradio", 0x0234E86C},
    {"previousradio", 0x0234E864},
    {"lookbehind", 0x0234E904},
    {"hydraulics", 0x0234E93C},
    {"nitrous", 0x0234E944},
    {"airaccelerate", 0x0234E90C},
    {"decelerate", 0x0234E914},
    {"rudderleft", 0x0234E8F4},
    {"rudderright", 0x0234E8FC}
};

std::unordered_map<std::string, uint8_t> buttonMap = {
    {"A", 0},
    {"B", 1},
    {"X", 2},
    {"Y", 3},
    {"LB", 4},
    {"RB", 5},
    {"SELECT", 6},
    {"START", 7},
    {"RS", 9},
    {"LS", 8},
    {"LT", 10},
    {"RT", 11},
    {"DPadRight", 16},
    {"DPadUp", 17},
    {"DPadLeft", 18},
    {"DPadDown", 19}
};

std::string getCurrentDirectory() {
    wchar_t path[MAX_PATH];
    HMODULE hm = NULL;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&getCurrentDirectory, &hm) == 0) {
        int ret = GetLastError();
        MessageBoxW(NULL, (L"GetModuleHandle failed, error = " + std::to_wstring(ret)).c_str(), L"Error", MB_OK);
        exit(1);
    }
    GetModuleFileNameW(hm, path, sizeof(path) / sizeof(wchar_t));
    std::wstring::size_type pos = std::wstring(path).find_last_of(L"\\/");
    return std::string(path, path + pos); // Convert to std::string
}

// Function to read the INI files and set the mappings
bool readIniFile(const std::string& filePath, std::unordered_map<std::string, uint8_t>& controlMap) {
    if (!std::filesystem::exists(filePath)) {
        // If the file does not exist, return false
        #ifdef _DEBUG
        std::cout << "File does not exist: " << filePath << std::endl;
#endif
        return false;
    }

#ifdef _DEBUG
    std::cout << "Reading INI file: " << filePath << std::endl;
#endif

    mINI::INIFile iniFile(filePath);
    mINI::INIStructure iniStructure;
    if (iniFile.read(iniStructure)) {
        if (iniStructure.has("Controls")) {
            for (const auto& kv : iniStructure["Controls"]) {
                std::string key = toLower(kv.first); // Convert key to lowercase
                if (buttonMap.find(kv.second) != buttonMap.end()) {
                    controlMap[key] = buttonMap[kv.second];
#ifdef _DEBUG
                    std::cout << "Set " << key << " to " << (int)controlMap[key] << std::endl;
#endif
                }
                else {
#ifdef _DEBUG
                    std::cout << "Button " << kv.second << " not found in buttonMap" << std::endl;
#endif
                }
            }
            return true;
        }
        else {
#ifdef _DEBUG
            std::cout << "No 'Controls' section found in INI file" << std::endl;
#endif
        }
    }
    else {
        MessageBoxW(NULL, L"Failed to read INI file", L"Error", MB_OK | MB_ICONERROR);
    }
    return false;
}

void loadControls() {
    std::string currentDir = getCurrentDirectory();

    // Load default controls
    readIniFile(currentDir + "\\RowInput\\Default.ini", controlsMap["Default"]);

    // Load specific controls with fallback to default
    if (!readIniFile(currentDir + "\\RowInput\\Vehicle.ini", controlsMap["Vehicle"])) {
        controlsMap["Vehicle"] = controlsMap["Default"];
#ifdef _DEBUG
        std::cout << "Fallback to Default controls for Vehicle" << std::endl;
#endif
    }

    if (!readIniFile(currentDir + "\\RowInput\\Boat.ini", controlsMap["Boat"])) {
        controlsMap["Boat"] = controlsMap["Vehicle"];
#ifdef _DEBUG
        std::cout << "Fallback to Vehicle controls for Boat" << std::endl;
#endif
    }

    if (!readIniFile(currentDir + "\\RowInput\\Helicopter.ini", controlsMap["Helicopter"])) {
        controlsMap["Helicopter"] = controlsMap["Default"];
#ifdef _DEBUG
        std::cout << "Fallback to Default controls for Helicopter" << std::endl;
#endif
    }

    if (!readIniFile(currentDir + "\\RowInput\\Plane.ini", controlsMap["Plane"])) {
        controlsMap["Plane"] = controlsMap["Default"];
#ifdef _DEBUG
        std::cout << "Fallback to Default controls for Plane" << std::endl;
#endif
    }

    if (!readIniFile(currentDir + "\\RowInput\\HumanShield.ini", controlsMap["HumanShield"])) {
        controlsMap["HumanShield"] = controlsMap["Default"];
#ifdef _DEBUG
        std::cout << "Fallback to Default controls for HumanShield" << std::endl;
#endif
    }

    if (!readIniFile(currentDir + "\\RowInput\\Bullets.ini", controlsMap["Bullets"])) {
        controlsMap["Bullets"] = controlsMap["Default"];
#ifdef _DEBUG
        std::cout << "Fallback to Default controls for Bullets" << std::endl;
#endif
    }
}

void setControlValue(uintptr_t address, uint8_t value) {
    if (*(uint8_t*)address != value) {
        patchByte((BYTE*)address, value);
#ifdef _DEBUG
        std::cout << "Patched address " << std::hex << address << " with value " << std::dec << (int)value << std::endl;
#endif
        lastAppliedValues[std::to_string(address)] = value;
    } else {
#ifdef _DEBUG
        if (lastAppliedValues[std::to_string(address)] != value) {
            std::cout << "Address " << std::hex << address << " already has value " << std::dec << (int)value << std::endl;
            lastAppliedValues[std::to_string(address)] = value;
        }
#endif
    }
}

void applyControls(const std::unordered_map<std::string, uint8_t>& controls, const std::unordered_map<std::string, uintptr_t>& addressMap) {
    for (const auto& kv : controls) {
        if (addressMap.find(kv.first) != addressMap.end()) {
            uintptr_t address = addressMap.at(kv.first);
            if (lastAppliedValues[std::to_string(address)] != kv.second) {
                setControlValue(address, kv.second);
            }
        } else {
#ifdef _DEBUG
            std::cout << "Address for control " << kv.first << " not found" << std::endl;
#endif
        }
    }
}
#pragma optimize("", off)
__declspec(noinline) void SchemeB() {
    uint8_t* player_status =  (uint8_t*)0x00E9A5BC;
    uint8_t* holdingbullets = (uint8_t*)0x031ABC44;
    uint8_t* menu_status = (uint8_t*)0x00EBE860;
    if (*menu_status == 2) {
        switch (*player_status) {
        case 3:
            applyControls(controlsMap["Vehicle"], addressMap); // Vehicle controls
            break;
        case 5:
            applyControls(controlsMap["Boat"], addressMap); // Boat controls
            break;
        case 6:
            applyControls(controlsMap["Helicopter"], addressMap); // Helicopter controls
            break;
        case 8:
            applyControls(controlsMap["Plane"], addressMap); // Plane controls
            break;
        case 22:
            applyControls(controlsMap["HumanShield"], addressMap); // HumanShield controls
            break;
        default:
            if (*holdingbullets == 0) {
                applyControls(controlsMap["Bullets"], addressMap); // Bullets controls
            }
            else {
                applyControls(controlsMap["Default"], addressMap); // Default controls
            }
            break;
        }
    }
}
#pragma optimize("", on)
int myDetour() {
    // Call the original function
    return originalCall();
}

DWORD WINAPI SchemeBLoop(LPVOID) {
#ifdef _DEBUG
    while (g_running) {
        loadControls(); // Reload controls in debug builds
        SchemeB();
        Sleep(33); // Sleep 
}
#else
    loadControls(); // Load controls once initially in release builds
    while (g_running) {
        SchemeB();
        Sleep(20); // Sleep
    }
#endif
    return 0;
}
HANDLE g_thread = NULL;

void setupHook() {
    if (MH_Initialize() != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO INITIALIZE", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (MH_CreateHook((LPVOID)0x520450, &myDetour, (LPVOID*)&originalCall) != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO HOOK", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        MessageBoxW(NULL, L"FAILED TO ENABLE", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

#ifdef _DEBUG
    // Allocate a console for debug output
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    std::cout << "Hook setup complete. Starting SchemeB loop..." << std::endl;
    loadControls(); // Load controls initially in debug builds
#endif

    // Create the thread to run SchemeB in a loop
    g_thread = CreateThread(NULL, 0, SchemeBLoop, NULL, 0, NULL);
    if (g_thread == NULL) {
        MessageBoxW(NULL, L"FAILED TO CREATE THREAD", L"Error", MB_OK | MB_ICONERROR);
    }
    else {
#ifdef _DEBUG
        MessageBoxW(NULL, L"RowInput thread created", L"Success", MB_OK | MB_ICONINFORMATION);

        std::cout << "RowInput thread created" << std::endl;
#endif
    }
}

void stopHook() {
    g_running = false;
    WaitForSingleObject(g_thread, INFINITE); // Wait for the thread to finish
    CloseHandle(g_thread);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
#ifdef _DEBUG
    std::cout << "Hook stopped and uninitialized." << std::endl;
    // Free the console
    FreeConsole();
#endif
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        setupHook();
        break;
    case DLL_PROCESS_DETACH:
        stopHook();
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
