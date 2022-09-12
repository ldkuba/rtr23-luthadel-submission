#include "file_system.hpp"

FileSystem::FileSystem() {}
FileSystem::~FileSystem() {}

std::vector<byte> FileSystem::read_file_bytes(const std::string& file_path) {
    std::ifstream file{ file_path, std::ios::ate | std::ios::binary };

    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + file_path);

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<byte> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}