#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

DWORD GetModuleBaseAddress(DWORD processId, const char* moduleName) {
    MODULEENTRY32 moduleEntry = { sizeof(MODULEENTRY32) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (Module32First(snapshot, &moduleEntry)) {
        do {
            if (strcmp(moduleEntry.szModule, moduleName) == 0) {
                CloseHandle(snapshot);
                return (DWORD)moduleEntry.modBaseAddr;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }
    CloseHandle(snapshot);
    return 0;
}

void InjectAndExecuteCode(const char* code, DWORD processId) {
    DWORD baseAddress = GetModuleBaseAddress(processId, "RobloxPlayerBeta.exe");
    if (baseAddress == 0) {
        std::cout << "Failed to get base address of Roblox process. Exiting..." << std::endl;
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, processId);
    if (hProcess == NULL) {
        std::cout << "Failed to open Roblox process." << std::endl;
        return;
    }

    LPVOID codeAddr = VirtualAllocEx(hProcess, NULL, strlen(code) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (codeAddr == NULL) {
        std::cout << "Failed to allocate memory in Roblox process." << std::endl;
        CloseHandle(hProcess);
        return;
    }

    WriteProcessMemory(hProcess, codeAddr, code, strlen(code) + 1, NULL);
    DWORD threadId;
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)codeAddr, NULL, 0, &threadId);

    if (hThread == NULL) {
        std::cout << "Failed to create remote thread. Exiting..." << std::endl;
        CloseHandle(hProcess);
        return;
    }

    std::cout << "Code executed in Roblox process." << std::endl;

    CloseHandle(hThread);
    CloseHandle(hProcess);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            CreateWindowA("BUTTON", "Inject", WS_VISIBLE | WS_CHILD,
                10, 420, 100, 25, hwnd, (HMENU)1, NULL, NULL);

            CreateWindowA("BUTTON", "Execute", WS_VISIBLE | WS_CHILD,
                120, 420, 100, 25, hwnd, (HMENU)2, NULL, NULL);

            CreateWindowA("EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                10, 10, 780, 400, hwnd, (HMENU)3, NULL, NULL);

            break;
        }
        case WM_COMMAND: {
            if (HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == 1) { // Inject button clicked
                    HWND hEdit = GetDlgItem(hwnd, 3);
                    char code[1000];
                    GetWindowTextA(hEdit, code, 1000);

                    DWORD processId;
                    GetWindowThreadProcessId(FindWindowA(NULL, "Roblox"), &processId);
                    InjectAndExecuteCode(code, processId);
                } else if (LOWORD(wParam) == 2) { // Execute button clicked
                    HWND hEdit = GetDlgItem(hwnd, 3);
                    char code[1000];
                    GetWindowTextA(hEdit, code, 1000);

                    // Exec
                    system(code);
                }
            }
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int main() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "Lua Slave";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "RobloxExecutor", "Roblox Executor", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 500, NULL, NULL, GetModuleHandle(NULL), NULL);

    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
