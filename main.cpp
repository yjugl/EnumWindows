#include <windows.h>
#include <psapi.h>
#include <stdio.h>

constexpr auto kIndentIncrement = 2;
const auto kOutput = stderr;

void Indent(LPARAM indent)
{
    for (LPARAM i = 0; i < indent; ++i) {
        fputwc(L' ', kOutput);
    }
}

BOOL WINAPI PrintWindow(HWND window, LPARAM indent)
{
    wchar_t title[128]{ 0 };
    GetWindowTextW(window, title, sizeof title / sizeof *title);

    wchar_t className[128]{ 0 };
    GetClassNameW(window, className, sizeof className / sizeof *className);

    DWORD processId = 0;
    auto threadId = GetWindowThreadProcessId(window, &processId);

    wchar_t fileName[MAX_PATH]{ 0 };
    auto process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (process) {
        GetModuleFileNameExW(process, nullptr, fileName, sizeof fileName / sizeof *fileName);
    }
    Indent(indent);
    fwprintf(kOutput, L"Window \"%s\" of class \"%s\" from \"%s\" [pid %d tid %d]\n", title, className, fileName, processId, threadId);
    EnumChildWindows(
        window,
        PrintWindow,
        indent + kIndentIncrement
    );
    return TRUE;
}

BOOL CALLBACK PrintDesktop(LPWSTR desktopName, LPARAM indent)
{
    Indent(indent);
    fwprintf(kOutput, L"Desktop \"%s\"\n", desktopName);
    SetLastError(0);
    auto desktop = OpenDesktopW(desktopName, 0, FALSE, DESKTOP_READOBJECTS);
    if (!desktop) {
        Indent(indent + 2);
        fwprintf(kOutput, L"Failed to open Desktop (%d)\n", GetLastError());
    }
    else {
        auto previousDesktop = GetThreadDesktop(GetCurrentThreadId());
        SetThreadDesktop(desktop);
        EnumDesktopWindows(desktop, PrintWindow, indent + kIndentIncrement);
        SetThreadDesktop(previousDesktop);
    }
    return TRUE;
}

BOOL CALLBACK PrintWindowStation(LPWSTR windowStationName, LPARAM indent)
{
    Indent(indent);
    fwprintf(kOutput, L"Window Station \"%s\"\n", windowStationName);
    SetLastError(0);
    auto windowStation = OpenWindowStationW(windowStationName, FALSE, WINSTA_ENUMDESKTOPS);
    if (!windowStation) {
        Indent(indent + 2);
        fwprintf(kOutput, L"Failed to open Window Station (%d)\n", GetLastError());
    }
    else {
        auto previousWindowStation = GetProcessWindowStation();
        SetProcessWindowStation(windowStation);
        EnumDesktopsW(windowStation, PrintDesktop, indent + kIndentIncrement);
        SetProcessWindowStation(previousWindowStation);
    }
    return TRUE;
}

void PrintWindowStations(LPARAM indent)
{
    EnumWindowStationsW(PrintWindowStation, indent);
}

void PrintCurrentDesktop()
{
    wchar_t name[128]{ 0 };

    auto windowStation = GetProcessWindowStation();
    GetUserObjectInformationW(windowStation, UOI_NAME, reinterpret_cast<void*>(name), sizeof name, nullptr);
    fwprintf(kOutput, L"Current Window Station \"%s\"\n", name);

    name[0] = 0;
    auto desktop = GetThreadDesktop(GetCurrentThreadId());
    GetUserObjectInformationW(desktop, UOI_NAME, reinterpret_cast<void*>(name), sizeof name, nullptr);
    fwprintf(kOutput, L"Current Desktop \"%s\"\n", name);
}

int main(void)
{
    fwprintf(kOutput, L"############################################################\n");
    PrintCurrentDesktop();
    fwprintf(kOutput, L"############################################################\n");
    PrintWindowStations(0);
    fwprintf(kOutput, L"############################################################\n");
}
