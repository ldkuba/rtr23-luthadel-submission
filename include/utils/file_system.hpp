#pragma once

#include <vector>
#include <fstream>

#include "string.hpp"
#include "result.hpp"
#include "error_types.hpp"

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
    static Result<std::vector<byte>, RuntimeError> read_file_bytes(
        const String& file_path
    );
    static Result<std::vector<String>, RuntimeError> read_file_lines(
        const String& file_path
    );
};