#pragma once

#include <fstream>

#include "string.hpp"
#include "result.hpp"
#include "error_types.hpp"
#include "platform/platform.hpp"

#include <nlohmann/json.hpp>

namespace ENGINE_NAMESPACE {

/**
 * @brief IO File
 */
class File : public std::fstream {
  public:
    using std::fstream::fstream;
};
/**
 * @brief Input file
 */
class FileIn : public std::ifstream {
  public:
    using std::ifstream::ifstream;

    String read(const uint64 size) {
        char* buffer = new (MemoryTag::Temp) char(size);
        auto  n      = std::ifstream::readsome(buffer, size);
        if (n) return String(std::string(buffer, n));
        return "";
    }
};
/**
 * @brief Output file
 */
class FileOut : public std::ofstream {
  public:
    using std::ofstream::ofstream;

    template<typename... Args>
    void write(const Args&... data) {
        auto all_data = String::build(data...);
        std::ofstream::write(all_data.data(), all_data.size());
    }
    template<typename... Args>
    void write_ln(const Args&... data) {
        write(data..., "\n");
    }
};

class FileSystem {
  public:
    /// @brief File open mode flags
    typedef std::_Ios_Openmode OpenMode;

    /// Seek to end before each write.
    static const OpenMode app = OpenMode::_S_app;

    /// Open and seek to end immediately after opening.
    static const OpenMode ate = OpenMode::_S_ate;

    /// Perform input and output in binary mode (as opposed to text mode).
    /// This is probably not what you think it is; see
    /// https://gcc.gnu.org/onlinedocs/libstdc++/manual/fstreams.html#std.io.filestreams.binary
    static const OpenMode binary = OpenMode::_S_bin;

    /// Truncate an existing stream when opening.  Default for @c ofstream.
    static const OpenMode trunc = OpenMode::_S_trunc;

    /**
     * @brief Checks whether a file on a given path exists
     *
     * @param file_path File path, separated by dashes ('/')
     * @return true If file exists
     * @return false Otherwise
     */
    static bool exists(const String& file_path);

    /**
     * @brief Opens file for input and output. Will fails if file doesn't exist.
     *
     * @param file_path File path, separated by dashes ('/')
     * @param mode Active file open modes
     * @return File If successful
     * @throw RuntimeError Otherwise
     */
    static Result<std::unique_ptr<File>, RuntimeError> open(
        const String& file_path, OpenMode mode = {}
    );
    /**
     * @brief Opens file for input. Will fails if file doesn't exist.
     *
     * @param file_path File path, separated by dashes ('/')
     * @param mode Active file open modes
     * @return File If successful
     * @throw RuntimeError Otherwise
     */
    static Result<std::unique_ptr<FileIn>, RuntimeError> open_input(
        const String& file_path, OpenMode mode = {}
    );
    /**
     * @brief Opens file for output. Will fails if file doesn't exist.
     *
     * @param file_path File path, separated by dashes ('/')
     * @param mode Active file open modes
     * @return File If successful
     * @throw RuntimeError Otherwise
     */
    static Result<std::unique_ptr<FileOut>, RuntimeError> open_output(
        const String& file_path, OpenMode mode = {}
    );

    /**
     * @brief Create and open a file. Will fail if file already exists. All
     * required nonexistant directories will also be created.
     *
     * @param file_path File path, separated by dashes ('/')
     * @param mode Active file open modes
     * @return File If successful
     * @throw RuntimeError Otherwise
     */
    static Result<std::unique_ptr<FileOut>, RuntimeError> create(
        const String& file_path, OpenMode mode = {}
    );

    /**
     * @brief Create and open a file. Just opens already existing files. All
     * required nonexistant directories will also be created.
     *
     * @param file_path File path, separated by dashes ('/')
     * @param mode Active file open modes
     * @return File If successful
     * @throw RuntimeError Otherwise
     */
    static Result<std::unique_ptr<FileOut>, RuntimeError> create_or_open(
        const String& file_path, OpenMode mode = {}
    );

    /**
     * @brief Opens and fully reads a binary file
     *
     * @param file_path File path, separated by dashes ('/')
     * @return All file bytes as Vector<byte> if successful
     * @throw RuntimeError otherwise
     */
    static Result<Vector<byte>, RuntimeError> read_bytes(const String& file_path
    );
    /**
     * @brief Opens and fully reads a text file.
     *
     * @param file_path File path, separated by dashes ('/')
     * @return All file lines as Vector<String> if successful
     * @throw RuntimeError otherwise
     */
    static Result<Vector<String>, RuntimeError> read_lines(
        const String& file_path
    );

    /**
     * @brief Opens and fully reads a json file.
     *
     * @param file_path File path, separated by dashes ('/')
     * @return json object
     * @throw RuntimeError otherwise
     */
    static Result<nlohmann::json, RuntimeError> read_json(
        const String& file_path
    );

  private:
    FileSystem();
    ~FileSystem();
};

} // namespace ENGINE_NAMESPACE