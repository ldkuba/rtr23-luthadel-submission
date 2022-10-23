#pragma once

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

    static File                               open(const String& file_path);
    static Result<Vector<byte>, RuntimeError> read_file_bytes(
        const String& file_path
    );
    static Result<Vector<String>, RuntimeError> read_file_lines(
        const String& file_path
    );
};