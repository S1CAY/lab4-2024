#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>

constexpr size_t BUFFER_SIZE = 8 * 1024; // 8 KB buffer

struct AsyncFileOperation {
    HANDLE hFile;
    OVERLAPPED overlapped;
    std::vector<char> buffer;
    DWORD bytesTransferred;
    HANDLE event;

    AsyncFileOperation(HANDLE fileHandle)
        : hFile(fileHandle),
          overlapped{},
          buffer(BUFFER_SIZE),
          bytesTransferred(0),
          event(CreateEvent(nullptr, TRUE, FALSE, nullptr)) {
        overlapped.hEvent = event;
    }

    ~AsyncFileOperation() {
        if (event) CloseHandle(event);
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    }
};

void PerformAsyncIO(const std::vector<std::wstring>& inputFiles, const std::vector<std::wstring>& outputFiles) {
    if (inputFiles.size() != outputFiles.size()) {
        std::cerr << "Input and output file counts must match.\n";
        return;
    }

    std::vector<AsyncFileOperation*> operations;
    for (size_t i = 0; i < inputFiles.size(); ++i) {
        HANDLE inputFile = CreateFileW(inputFiles[i].c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                       FILE_FLAG_OVERLAPPED, nullptr);
        if (inputFile == INVALID_HANDLE_VALUE) {
            std::wcerr << L"Failed to open input file: " << inputFiles[i] << L". Error: " << GetLastError() << "\n";
            continue;
        }


        HANDLE outputFile = CreateFileW(outputFiles[i].c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                        FILE_FLAG_OVERLAPPED, nullptr);
        if (outputFile == INVALID_HANDLE_VALUE) {
            std::wcerr << L"Failed to open output file: " << outputFiles[i] << L". Error: " << GetLastError() << "\n";
            CloseHandle(inputFile);
            continue;
        }

        AsyncFileOperation* operation = new AsyncFileOperation(inputFile);
        operations.push_back(operation);

        // Start an asynchronous read operation
        DWORD bytesRead;
        if (!ReadFile(inputFile, operation->buffer.data(), BUFFER_SIZE, &bytesRead, &operation->overlapped) &&
            GetLastError() != ERROR_IO_PENDING) {
            std::cerr << "Failed to initiate asynchronous read. Error: " << GetLastError() << "\n";
            delete operation;
            operations.pop_back();
        }
    }

    while (!operations.empty()) {
        std::vector<HANDLE> events;
        for (auto* op : operations) {
            events.push_back(op->event);
        }

        DWORD waitResult = WaitForMultipleObjects(events.size(), events.data(), FALSE, INFINITE);
        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + events.size()) {
            size_t completedIndex = waitResult - WAIT_OBJECT_0;
            AsyncFileOperation* completedOp = operations[completedIndex];

            DWORD bytesTransferred;
            if (GetOverlappedResult(completedOp->hFile, &completedOp->overlapped, &bytesTransferred, FALSE)) {
                if (bytesTransferred > 0) {
                    // Handle successful read operation
                    HANDLE outputFile = CreateFileW(outputFiles[completedIndex].c_str(), GENERIC_WRITE, 0, nullptr,
                                                    OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);

                    OVERLAPPED writeOverlapped{};
                    writeOverlapped.Offset = completedOp->overlapped.Offset;
                    writeOverlapped.hEvent = completedOp->event;

                    DWORD bytesWritten;
                    if (!WriteFile(outputFile, completedOp->buffer.data(), bytesTransferred, &bytesWritten, &writeOverlapped) &&
                        GetLastError() != ERROR_IO_PENDING) {
                        std::cerr << "Failed to write to output file. Error: " << GetLastError() << "\n";
                    }

                    // Prepare for the next read
                    completedOp->overlapped.Offset += bytesTransferred;
                    if (!ReadFile(completedOp->hFile, completedOp->buffer.data(), BUFFER_SIZE, nullptr, &completedOp->overlapped) &&
                        GetLastError() != ERROR_IO_PENDING) {
                        std::cerr << "Failed to initiate next asynchronous read. Error: " << GetLastError() << "\n";
                    }
                } else {
                    // EOF reached, clean up
                    delete completedOp;
                    operations.erase(operations.begin() + completedIndex);
                }
            } else {
                std::cerr << "Failed to complete asynchronous operation. Error: " << GetLastError() << "\n";
                delete completedOp;
                operations.erase(operations.begin() + completedIndex);
            }
        } else {
            std::cerr << "WaitForMultipleObjects failed. Error: " << GetLastError() << "\n";
            break;
        }
    }

    for (auto* op : operations) {
        delete op;
    }
}

int main() {
    std::vector<std::wstring> inputFiles = {L"file1.bin", L"file2.bin"};
    std::vector<std::wstring> outputFiles = {L"output1.bin", L"output2.bin"};

    PerformAsyncIO(inputFiles, outputFiles);

    return 0;
}
