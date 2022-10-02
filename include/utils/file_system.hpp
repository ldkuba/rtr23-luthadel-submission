#pragma once

#include <vector>
#include <fstream>

#include "string.hpp"

class FileSystemException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class File {
public:
    File();
    ~File();
};
class BinaryFile {
public:
    BinaryFile();
    ~BinaryFile();
};

class FileSystem {
private:

public:
    FileSystem();
    ~FileSystem();

    static File open(const String& file_path);
    static std::vector<byte> read_file_bytes(const String& file_path);
    static std::vector<String> read_file_lines(const String& file_path);
};