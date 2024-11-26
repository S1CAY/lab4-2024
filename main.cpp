#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <sddl.h>

void PrintFileTime(const FILETIME& ft, const std::string& description) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);

    std::cout << description << ": "
              << st.wDay << "/" << st.wMonth << "/" << st.wYear << " "
              << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "\n";
}

std::string GetFileAttributesString(DWORD attributes) {
    std::string result;
    if (attributes & FILE_ATTRIBUTE_READONLY) result += "Read-Only ";
    if (attributes & FILE_ATTRIBUTE_HIDDEN) result += "Hidden ";
    if (attributes & FILE_ATTRIBUTE_SYSTEM) result += "System ";
    if (attributes & FILE_ATTRIBUTE_ARCHIVE) result += "Archive ";
    if (attributes & FILE_ATTRIBUTE_TEMPORARY) result += "Temporary ";
    if (attributes & FILE_ATTRIBUTE_COMPRESSED) result += "Compressed ";
    if (attributes & FILE_ATTRIBUTE_ENCRYPTED) result += "Encrypted ";
    return result.empty() ? "None" : result;
}

void GetOwnerInfo(const std::wstring& filePath) {
    PSID ownerSid = nullptr;
    PSECURITY_DESCRIPTOR securityDescriptor = nullptr;

    if (GetNamedSecurityInfoW(filePath.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                              &ownerSid, nullptr, nullptr, nullptr, &securityDescriptor) == ERROR_SUCCESS) {
        LPWSTR ownerName = nullptr;
        if (ConvertSidToStringSidW(ownerSid, &ownerName)) {
            std::wcout << L"Owner SID: " << ownerName << L"\n";
            LocalFree(ownerName);
        }
    } else {
        std::cerr << "Failed to get owner information.\n";
    }

    if (securityDescriptor) {
        LocalFree(securityDescriptor);
    }
}

void GetFileInfo(const std::wstring& filePath) {
    HANDLE hFile = CreateFileW(
        filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Could not open file. Error: " << GetLastError() << "\n";
        return;
    }
    // File size
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(hFile, &fileSize)) {
        std::cout << "File size: " << fileSize.QuadPart << " bytes\n";
    } else {
        std::cerr << "Failed to get file size.\n";
    }

    // File times
    FILETIME creationTime, accessTime, writeTime;
    if (GetFileTime(hFile, &creationTime, &accessTime, &writeTime)) {
        PrintFileTime(creationTime, "Creation time");
        PrintFileTime(accessTime, "Last access time");
        PrintFileTime(writeTime, "Last modification time");
    } else {
        std::cerr << "Failed to get file times.\n";
    }

    CloseHandle(hFile);

    // File attributes
    DWORD attributes = GetFileAttributesW(filePath.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        std::cout << "File attributes: " << GetFileAttributesString(attributes) << "\n";
    } else {
        std::cerr << "Failed to get file attributes.\n";
    }

    // Owner info
    GetOwnerInfo(filePath);
}

int main() {
    std::wcout << L"Enter the full path to the file: ";
    std::wstring filePath;
    std::getline(std::wcin, filePath);

    GetFileInfo(filePath);

    return 0;
}
