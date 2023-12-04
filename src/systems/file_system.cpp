#include "systems/file_system.hpp"

#include <filesystem>

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
FileSystem::FileSystem() {}
FileSystem::~FileSystem() {}

// ////////////////////////// //
// FILE SYSTEM PUBLIC METHODS //
// ////////////////////////// //

bool FileSystem::exists(const String& file_path) {
    return std::filesystem::exists({ std::string(file_path) });
}

// Open file
Result<std::unique_ptr<File>, RuntimeError> FileSystem::open(
    const String& file_path, OpenMode mode
) {
    auto file =
        std::make_unique<File>(file_path, std::ios::in | std::ios::out | mode);
    if (!file->is_open()) return Failure("Failed to open file:" + file_path);
    return file;
}
Result<std::unique_ptr<FileIn>, RuntimeError> FileSystem::open_input(
    const String& file_path, OpenMode mode
) {
    auto file = std::make_unique<FileIn>(file_path, std::ios::in | mode);
    if (!file->is_open()) return Failure("Failed to open file:" + file_path);
    return file;
}
Result<std::unique_ptr<FileOut>, RuntimeError> FileSystem::open_output(
    const String& file_path, OpenMode mode
) {
    if (!exists(file_path))
        return Failure(
            "Failed to open file:" + file_path + ". This file doesn't exist."
        );
    auto file = std::make_unique<FileOut>(file_path, std::ios::out | mode);
    if (!file->is_open()) return Failure("Failed to open file:" + file_path);
    return file;
}

// Create file
Result<std::unique_ptr<FileOut>, RuntimeError> FileSystem::create(
    const String& file_path, OpenMode mode
) {
    std::filesystem::path path { std::string(file_path) };

    // Check existence
    if (std::filesystem::exists(path))
        return Failure(
            "Failed to create file:" + file_path + ". This file already exist."
        );

    // Create required directories
    std::filesystem::create_directories(path.parent_path());

    // Create & open file
    auto file = std::make_unique<FileOut>(file_path, std::ios::out | mode);
    if (!file->is_open()) return Failure("Failed to create file:" + file_path);
    return file;
}

Result<std::unique_ptr<FileOut>, RuntimeError> FileSystem::create_or_open(
    const String& file_path, OpenMode mode
) {
    std::filesystem::path path { std::string(file_path) };

    // Check existence
    if (!std::filesystem::exists(path))
        // Create required directories
        std::filesystem::create_directories(path.parent_path());

    // Create & open file
    auto file = std::make_unique<FileOut>(file_path, std::ios::out | mode);
    if (!file->is_open()) return Failure("Failed to create file:" + file_path);
    return file;
}

// Read whole file
Result<Vector<byte>, RuntimeError> FileSystem::read_bytes(
    const String& file_path
) {
    std::ifstream file { file_path,
                         std::ios::ate | std::ios::binary | std::ios::in };

    if (!file.is_open()) return Failure("Failed to open file: " + file_path);

    size_t       file_size = static_cast<size_t>(file.tellg());
    Vector<byte> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

Result<Vector<String>, RuntimeError> FileSystem::read_lines(
    const String& file_path
) {
    std::ifstream file { file_path, std::ios::in };

    if (!file.is_open()) return Failure("Failed to open file: " + file_path);

    Vector<String> lines;
    String         line;
    while (std::getline(file, line))
        lines.push_back(line);

    file.close();

    return lines;
}

Result<nlohmann::json, RuntimeError> FileSystem::read_json(
    const String& file_path
) {
    std::ifstream file { file_path, std::ios::in };
    if (!file.is_open()) return Failure("Failed to open file: " + file_path);

    return nlohmann::json::parse(file);
}

} // namespace ENGINE_NAMESPACE