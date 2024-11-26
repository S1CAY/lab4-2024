#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

void ReadLargeFile_C(const std::string& inputFile, const std::string& outputFile) {
    FILE* input = fopen(inputFile.c_str(), "rb");
    if (!input) {
        std::cerr << "Failed to open input file.\n";
        return;
    }

    FILE* output = fopen(outputFile.c_str(), "wb");
    if (!output) {
        std::cerr << "Failed to open output file.\n";
        fclose(input);
        return;
    }

    constexpr size_t bufferSize = 8 * 1024; // 8 KB buffer
    std::vector<char> buffer(bufferSize);

    auto start = std::chrono::high_resolution_clock::now();

    size_t bytesRead;
    while ((bytesRead = fread(buffer.data(), 1, bufferSize, input)) > 0) {
        fwrite(buffer.data(), 1, bytesRead, output);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken (C library): "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";

    fclose(input);
    fclose(output);
}

int main() {
    std::string inputFile = "large_input.bin";
    std::string outputFile = "large_output.bin";
    ReadLargeFile_C(inputFile, outputFile);
    return 0;
}
