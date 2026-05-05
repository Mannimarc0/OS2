#include <stdio.h>
#include <io.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <windows.h>
#include <vector>
#include <iomanip>
#ifndef _O_U16TEXT
#define _O_U16TEXT 0x20000
#endif
bool SafeWrite(DWORD* target, DWORD value) {
    __try {
        *target = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void printError(const std::wstring& operation) {
    DWORD err = GetLastError();
    std::wstring reason;
    switch (err) {
    case ERROR_ACCESS_DENIED:        reason = L"Access denied.";            break;
    case ERROR_INVALID_PARAMETER:    reason = L"Invalid parameter.";        break;
    case ERROR_NOT_ENOUGH_MEMORY:    reason = L"Not enough memory.";        break;
    case ERROR_COMMITMENT_LIMIT:     reason = L"Commitment limit reached."; break;
    case ERROR_INVALID_ADDRESS:      reason = L"Invalid address.";          break;
    default:                         reason = L"Unknown error.";            break;
    }
    std::wcout << L"  [ERROR] Failed to " << operation
        << L". Code " << err << L": " << reason << std::endl;
}

std::wstring memStateStr(DWORD s) {
    if (s == MEM_COMMIT)  return L"MEM_COMMIT";
    if (s == MEM_FREE)    return L"MEM_FREE";
    if (s == MEM_RESERVE) return L"MEM_RESERVE";
    return L"UNKNOWN";
}

std::wstring memTypeStr(DWORD t) {
    if (t == MEM_IMAGE)   return L"MEM_IMAGE";
    if (t == MEM_MAPPED)  return L"MEM_MAPPED";
    if (t == MEM_PRIVATE) return L"MEM_PRIVATE";
    return L"—";
}

std::wstring protectStr(DWORD p) {
    std::wstring s;
    DWORD base = p & 0xFF; // базовые биты защиты без модификаторов
    switch (base) {
    case PAGE_NOACCESS:          s = L"PAGE_NOACCESS";          break;
    case PAGE_READONLY:          s = L"PAGE_READONLY";          break;
    case PAGE_READWRITE:         s = L"PAGE_READWRITE";         break;
    case PAGE_WRITECOPY:         s = L"PAGE_WRITECOPY";         break;
    case PAGE_EXECUTE:           s = L"PAGE_EXECUTE";           break;
    case PAGE_EXECUTE_READ:      s = L"PAGE_EXECUTE_READ";      break;
    case PAGE_EXECUTE_READWRITE: s = L"PAGE_EXECUTE_READWRITE"; break;
    case PAGE_EXECUTE_WRITECOPY: s = L"PAGE_EXECUTE_WRITECOPY"; break;
    default:                     s = L"UNKNOWN";                break;
    }
    if (p & PAGE_GUARD)        s += L" | PAGE_GUARD";
    if (p & PAGE_NOCACHE)      s += L" | PAGE_NOCACHE";
    if (p & PAGE_WRITECOMBINE) s += L" | PAGE_WRITECOMBINE";
    return s;
}

void showSystemInfo() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    std::wcout << L"\n=== System Information (GetSystemInfo) ===" << std::endl;
    std::wcout << L"  Processor Architecture:     ";
    switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:   std::wcout << L"x64 (AMD or Intel)"; break;
    case PROCESSOR_ARCHITECTURE_ARM:     std::wcout << L"ARM";                break;
    case PROCESSOR_ARCHITECTURE_ARM64:   std::wcout << L"ARM64";              break;
    case PROCESSOR_ARCHITECTURE_IA64:    std::wcout << L"Intel Itanium";      break;
    case PROCESSOR_ARCHITECTURE_INTEL:   std::wcout << L"x86";                break;
    default:                             std::wcout << L"Unknown";            break;
    }
    std::wcout << std::endl;
    std::wcout << L"  Page Size:                  " << si.dwPageSize << L" bytes" << std::endl;
    std::wcout << L"  Min App Address:            0x" << std::hex << (ULONG_PTR)si.lpMinimumApplicationAddress << std::dec << std::endl;
    std::wcout << L"  Max App Address:            0x" << std::hex << (ULONG_PTR)si.lpMaximumApplicationAddress << std::dec << std::endl;
    std::wcout << L"  Active Processor Mask:      0x" << std::hex << si.dwActiveProcessorMask << std::dec << std::endl;
    std::wcout << L"  Number of Processors:       " << si.dwNumberOfProcessors << std::endl;
    std::wcout << L"  Processor Type:             " << si.dwProcessorType << std::endl;
    std::wcout << L"  Allocation Granularity:     " << si.dwAllocationGranularity << L" bytes" << std::endl;
    std::wcout << L"  Processor Level:            " << si.wProcessorLevel << std::endl;
    std::wcout << L"  Processor Revision:         " << si.wProcessorRevision << std::endl;
}

void showMemoryStatus() {
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) {
        printError(L"GlobalMemoryStatusEx");
        return;
    }
    auto MB = [](DWORDLONG v) { return v / (1024 * 1024); };

    std::wcout << L"\n=== Virtual Memory Status (GlobalMemoryStatusEx) ===" << std::endl;
    std::wcout << L"  Memory Load:                " << ms.dwMemoryLoad << L"%" << std::endl;
    std::wcout << L"  Total Physical RAM:         " << MB(ms.ullTotalPhys) << L" MB" << std::endl;
    std::wcout << L"  Available Physical RAM:     " << MB(ms.ullAvailPhys) << L" MB" << std::endl;
    std::wcout << L"  Total Page File:            " << MB(ms.ullTotalPageFile) << L" MB" << std::endl;
    std::wcout << L"  Available Page File:        " << MB(ms.ullAvailPageFile) << L" MB" << std::endl;
    std::wcout << L"  Total Virtual (user):       " << MB(ms.ullTotalVirtual) << L" MB" << std::endl;
    std::wcout << L"  Available Virtual (user):   " << MB(ms.ullAvailVirtual) << L" MB" << std::endl;
    std::wcout << L"  Available Extended Virtual: " << MB(ms.ullAvailExtendedVirtual) << L" MB" << std::endl;
}

void queryMemoryRegion() {
    std::wcout << L"  Enter address (hex, e.g. 0x00400000): ";
    ULONG_PTR addr;
    std::wcin >> std::hex >> addr >> std::dec;

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T ret = VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi));
    if (ret == 0) {
        printError(L"VirtualQuery");
        return;
    }

    std::wcout << L"\n  --- VirtualQuery result ---" << std::endl;
    std::wcout << L"  Base Address:       0x" << std::hex << (ULONG_PTR)mbi.BaseAddress << std::dec << std::endl;
    std::wcout << L"  Allocation Base:    0x" << std::hex << (ULONG_PTR)mbi.AllocationBase << std::dec << std::endl;
    std::wcout << L"  Alloc Protect:      " << protectStr(mbi.AllocationProtect) << std::endl;
    std::wcout << L"  Region Size:        " << mbi.RegionSize << L" bytes" << std::endl;
    std::wcout << L"  State:              " << memStateStr(mbi.State) << std::endl;
    std::wcout << L"  Protect:            " << protectStr(mbi.Protect) << std::endl;
    std::wcout << L"  Type:               " << memTypeStr(mbi.Type) << std::endl;
}

static std::vector<LPVOID> g_allocations; // список выделенных регионов текущей сессии

void virtualAllocMenu() {
    int mode, allocType;
    std::wcout << L"\n  Address mode:  [1] Automatic  [2] Manual: ";
    std::wcin >> mode;

    LPVOID baseAddr = NULL;
    if (mode == 2) {
        std::wcout << L"  Enter desired base address (hex): ";
        ULONG_PTR a;
        std::wcin >> std::hex >> a >> std::dec;
        baseAddr = (LPVOID)a;
    }

    SIZE_T regionSize;
    std::wcout << L"  Enter region size (bytes): ";
    std::wcin >> regionSize;

    std::wcout << L"  Allocation type:" << std::endl;
    std::wcout << L"    [1] Reserve only  (MEM_RESERVE)" << std::endl;
    std::wcout << L"    [2] Commit only   (MEM_COMMIT)" << std::endl;
    std::wcout << L"    [3] Reserve+Commit simultaneously" << std::endl;
    std::wcout << L"  Choice: ";
    std::wcin >> allocType;

    DWORD flAllocType = 0;
    switch (allocType) {
    case 1: flAllocType = MEM_RESERVE;            break;
    case 2: flAllocType = MEM_COMMIT;             break;
    case 3: flAllocType = MEM_RESERVE | MEM_COMMIT; break;
    default:
        std::wcout << L"  Invalid choice." << std::endl;
        return;
    }

    LPVOID ptr = VirtualAlloc(baseAddr, regionSize, flAllocType, PAGE_READWRITE);
    if (!ptr) {
        printError(L"VirtualAlloc");
        return;
    }
    g_allocations.push_back(ptr);
    std::wcout << L"  Allocated at: 0x" << std::hex << (ULONG_PTR)ptr << std::dec << std::endl;
    std::wcout << L"  (saved to allocation list, index " << g_allocations.size() - 1 << L")" << std::endl;
}

void virtualFreeMenu() {
    if (g_allocations.empty()) {
        std::wcout << L"  No tracked allocations." << std::endl;
        return;
    }
    std::wcout << L"  Tracked allocations:" << std::endl;
    for (size_t i = 0; i < g_allocations.size(); ++i)
        std::wcout << L"    [" << i << L"] 0x" << std::hex << (ULONG_PTR)g_allocations[i] << std::dec << std::endl;
    std::wcout << L"  Enter index to free (-1 to cancel): ";
    int idx;
    std::wcin >> idx;
    if (idx < 0 || idx >= (int)g_allocations.size()) {
        std::wcout << L"  Cancelled." << std::endl;
        return;
    }
    if (VirtualFree(g_allocations[idx], 0, MEM_RELEASE)) {
        std::wcout << L"  Region freed." << std::endl;
        g_allocations.erase(g_allocations.begin() + idx);
    }
    else {
        printError(L"VirtualFree");
    }
}

void writeToMemory() {
    if (g_allocations.empty()) {
        std::wcout << L"  No tracked allocations to write into." << std::endl;
        return;
    }
    std::wcout << L"  Tracked allocations:" << std::endl;
    for (size_t i = 0; i < g_allocations.size(); ++i)
        std::wcout << L"    [" << i << L"] 0x" << std::hex << (ULONG_PTR)g_allocations[i] << std::dec << std::endl;
    std::wcout << L"  Enter allocation index: ";
    int idx;
    std::wcin >> idx;
    if (idx < 0 || idx >= (int)g_allocations.size()) {
        std::wcout << L"  Invalid index." << std::endl;
        return;
    }
    std::wcout << L"  Enter byte offset within region: ";
    SIZE_T offset;
    std::wcin >> offset;
    std::wcout << L"  Enter integer value to write (32-bit): ";
    DWORD value;
    std::wcin >> value;

    BYTE* target = (BYTE*)g_allocations[idx] + offset;
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(target, &mbi, sizeof(mbi));
    if (mbi.State != MEM_COMMIT) {
        if (!VirtualAlloc(target, sizeof(DWORD), MEM_COMMIT, PAGE_READWRITE)) {
            printError(L"commit page");
            return;
        }
    }
    if (SafeWrite((DWORD*)target, value)) {
        std::wcout << L"  Written 0x" << std::hex << value << std::dec
            << L" at address 0x" << std::hex << (ULONG_PTR)target << std::dec << std::endl;
        std::wcout << L"  Readback: 0x" << std::hex << *(DWORD*)target << std::dec << std::endl;
    }
    else {
        std::wcout << L"  [EXCEPTION] Access violation while writing." << std::endl;
    }
}


void setAndVerifyProtection() {
    if (g_allocations.empty()) {
        std::wcout << L"  No tracked allocations." << std::endl;
        return;
    }
    std::wcout << L"  Tracked allocations:" << std::endl;
    for (size_t i = 0; i < g_allocations.size(); ++i)
        std::wcout << L"    [" << i << L"] 0x" << std::hex << (ULONG_PTR)g_allocations[i] << std::dec << std::endl;
    std::wcout << L"  Enter allocation index: ";
    int idx;
    std::wcin >> idx;
    if (idx < 0 || idx >= (int)g_allocations.size()) {
        std::wcout << L"  Invalid index." << std::endl;
        return;
    }

    std::wcout << L"  Available protections:" << std::endl;
    std::wcout << L"    [1] PAGE_NOACCESS          (0x01)" << std::endl;
    std::wcout << L"    [2] PAGE_READONLY           (0x02)" << std::endl;
    std::wcout << L"    [3] PAGE_READWRITE          (0x04)" << std::endl;
    std::wcout << L"    [4] PAGE_EXECUTE            (0x10)" << std::endl;
    std::wcout << L"    [5] PAGE_EXECUTE_READ       (0x20)" << std::endl;
    std::wcout << L"    [6] PAGE_EXECUTE_READWRITE  (0x40)" << std::endl;
    std::wcout << L"  Choice: ";
    int pChoice;
    std::wcin >> pChoice;

    DWORD newProtect = 0;
    switch (pChoice) {
    case 1: newProtect = PAGE_NOACCESS;          break;
    case 2: newProtect = PAGE_READONLY;          break;
    case 3: newProtect = PAGE_READWRITE;         break;
    case 4: newProtect = PAGE_EXECUTE;           break;
    case 5: newProtect = PAGE_EXECUTE_READ;      break;
    case 6: newProtect = PAGE_EXECUTE_READWRITE; break;
    default:
        std::wcout << L"  Invalid choice." << std::endl;
        return;
    }

    SIZE_T regionSize;
    std::wcout << L"  Enter size of sub-region to protect (bytes): ";
    std::wcin >> regionSize;

    DWORD oldProtect = 0;
    if (!VirtualProtect(g_allocations[idx], regionSize, newProtect, &oldProtect)) {
        printError(L"VirtualProtect");
        return;
    }
    std::wcout << L"  Old protection: " << protectStr(oldProtect) << std::endl;
    std::wcout << L"  New protection: " << protectStr(newProtect) << std::endl;

    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(g_allocations[idx], &mbi, sizeof(mbi)); // проверяем защиту через VirtualQuery
    std::wcout << L"  Verified via VirtualQuery: " << protectStr(mbi.Protect) << std::endl;
}

int main() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    int choice;
    do {
        std::wcout << L"\n========================================" << std::endl;
        std::wcout << L"  Virtual Memory Management — Main Menu" << std::endl;
        std::wcout << L"========================================" << std::endl;
        std::wcout << L"  [1] System Information     (GetSystemInfo)" << std::endl;
        std::wcout << L"  [2] Memory Status          (GlobalMemoryStatusEx)" << std::endl;
        std::wcout << L"  [3] Query Region           (VirtualQuery)" << std::endl;
        std::wcout << L"  [4] Allocate Region        (VirtualAlloc)" << std::endl;
        std::wcout << L"  [5] Free Region            (VirtualFree)" << std::endl;
        std::wcout << L"  [6] Write to Memory" << std::endl;
        std::wcout << L"  [7] Set/Verify Protection  (VirtualProtect)" << std::endl;
        std::wcout << L"  [0] Exit" << std::endl;
        std::wcout << L"  Choice: ";
        std::wcin >> choice;

        switch (choice) {
        case 1: showSystemInfo();          break;
        case 2: showMemoryStatus();        break;
        case 3: queryMemoryRegion();       break;
        case 4: virtualAllocMenu();        break;
        case 5: virtualFreeMenu();         break;
        case 6: writeToMemory();           break;
        case 7: setAndVerifyProtection();  break;
        case 0: std::wcout << L"  Exiting..." << std::endl; break;
        default: std::wcout << L"  Invalid choice." << std::endl; break;
        }
    } while (choice != 0);

    for (LPVOID p : g_allocations) // освобождаем всё, что не было освобождено вручную
        VirtualFree(p, 0, MEM_RELEASE);

    return 0;
}
