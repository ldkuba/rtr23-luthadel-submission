#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "defines.hpp"

class FileSystem {
private:

public:
    FileSystem();
    ~FileSystem();

    static std::vector<byte> read_file_bytes(const std::string& file_path);
};
