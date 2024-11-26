#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

void ReadLargeFile_Windows(const std::wstring& inputFile, const std::wstring& outputFile) {
    HANDLE hInput = CreateFileW(inputFile.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hInput == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open input file. Error: " << GetLastError() << "\n";
        return;
    }

    HANDLE hOutput = CreateFileW(outputFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hOutput == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open output file. Error: " << GetLastError() << "\n";
        CloseHandle(hInput);
        return;
    }

    constexpr DWORD bufferSize = 8 * 1024; // 8 KB buffer
    std::vector<char> buffer(bufferSize);

    DWORD bytesRead, bytesWritten;
    auto start = std::chrono::high_resolution_clock::now();

    while (ReadFile(hInput, buffer.data(), bufferSize, &bytesRead, nullptr) && bytesRead > 0) {
        if (!WriteFile(hOutput, buffer.data(), bytesRead, &bytesWritten, nullptr)) {
            std::cerr << "Failed to write to output file. Error: " << GetLastError() << "\n";
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken (Windows API): "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";

    CloseHandle(hInput);
    CloseHandle(hOutput);
}

int main() {
    std::wstring inputFile = L"large_input.bin";
    std::wstring outputFile = L"large_output.bin";
    ReadLargeFile_Windows(inputFile, outputFile);
    return 0;
}
