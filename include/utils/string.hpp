#pragma once

#include <string>

#include "defines.hpp"
#include "result.hpp"
#include "error_types.hpp"
#include "vector.hpp"

// Additional to_string conversions
namespace std {
string to_string(const uint128& in);
string to_string(const int128& in);
} // namespace std

class String : public std::string {
  public:
    using std::string::string;
    String() noexcept;
    String(const std::string& __str) noexcept : std::string(__str) {}
    ~String() noexcept;

    // String builder
    /**
     * @brief Concatenates argument list into one string string object.
     * Non-string objects will automaticaly be converted to their string form
     * via std::to_string where applicable.
     * @returns Concatenated string
     */
    template<typename... Args>
    static String build(Args... message) noexcept {
        String result = "";
        (add_to_string(result, message), ...);
        return result;
    }

    // Transform
    /// @brief Transform all string characters to lowercase (inplace)
    void to_lower() noexcept;
    /// @brief Transform all string characters to uppercase (inplace)
    void to_upper() noexcept;

    // Trim
    /// @brief Removes all white-space characters from left side (Inplace)
    void trim_left() noexcept;
    /// @brief Removes all white-space characters from right side (Inplace)
    void trim_right() noexcept;
    /// @brief Removes all white-space characters from both sides (Inplace)
    void trim() noexcept;

    // Comparison
    /**
     *  @brief  Compare two strings; case insensitive.
     *  @param other  String to compare against.
     *  @return  Integer < 0, 0, or > 0.
     *
     * Returns an integer < 0 if this string is ordered before @a other, 0 if
     * their values are equivalent, or > 0 if this string is ordered after
     * @a other. Determines the effective length rlen of the strings to compare
     * as the smallest of size() and str.size(). The function then compares the
     * two strings by calling traits::compare(data(), str.data(),rlen). If the
     * result of the comparison is nonzero returns it, otherwise the shorter one
     * is ordered first.
     */
    int32 compare_ci(const String& other) const;

    // Split
    /**
     * @brief Splits string into substrings
     * @param delimiter Token to split by
     * @return Vector of strings
     */
    Vector<String> split(const String delimiter) const;
    /**
     * @brief Splits string into substrings
     * @param delimiter Token to split by
     * @return Vector of strings
     */
    Vector<String> split(const char delimiter) const;

    // Parsing
    /**
     * @brief Parses string as uint8
     * @return uint8
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<uint8, InvalidArgument>    parse_as_uint8();
    /**
     * @brief Parses string as uint16
     * @return uint16
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<uint16, InvalidArgument>   parse_as_uint16();
    /**
     * @brief Parses string as uint32
     * @return uint32
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<uint32, InvalidArgument>   parse_as_uint32();
    /**
     * @brief Parses string as uint64
     * @return uint64
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<uint64, InvalidArgument>   parse_as_uint64();
    /**
     * @brief Parses string as uint128
     * @return uint128
     * @throws InvalidArgument error If parse is impossible
     */
    Result<uint128, InvalidArgument>  parse_as_uint128();
    /**
     * @brief Parses string as int8
     * @return int8
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<int8, InvalidArgument>     parse_as_int8();
    /**
     * @brief Parses string as int16
     * @return int16
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<int16, InvalidArgument>    parse_as_int16();
    /**
     * @brief Parses string as int32
     * @return int32
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<int32, InvalidArgument>    parse_as_int32();
    /**
     * @brief Parses string as int64
     * @return int64
     * @throws InvalidArgument error If parse is impossible. Parse is also
     * deemed impossible if overflow is detected.
     */
    Result<int64, InvalidArgument>    parse_as_int64();
    /**
     * @brief Parses string as int128
     * @return int128
     * @throws InvalidArgument error If parse is impossible
     */
    Result<int128, InvalidArgument>   parse_as_int128();
    /**
     * @brief Parses string as float32
     * @return float32
     * @throws InvalidArgument error If parse is impossible
     */
    Result<float32, InvalidArgument>  parse_as_float32();
    /**
     * @brief Parses string as float64
     * @return float64
     * @throws InvalidArgument error If parse is impossible
     */
    Result<float64, InvalidArgument>  parse_as_float64();
    /**
     * @brief Parses string as float128
     * @return float128
     * @throws InvalidArgument error If parse is impossible
     */
    Result<float128, InvalidArgument> parse_as_float128();

    // NEW
    void* operator new(size_t size) {
        return ::operator new(size, MemoryTag::String);
    }

  private:
    // String builder
    template<typename T>
    static void add_to_string(String& out_string, T component) noexcept {
        out_string += std::to_string(component);
    }
};

namespace std {
template<>
struct hash<String> {
    size_t operator()(String const& str) const { return hash<string>()(str); }
};
} // namespace std

template<>
void String::add_to_string<char*>(String& out_string, char* component) noexcept;
template<>
void String::add_to_string<const char*>(
    String& out_string, const char* component
) noexcept;
template<>
void String::add_to_string<std::string>(
    String& out_string, std::string component
) noexcept;
template<>
void String::add_to_string<String>(
    String& out_string, String component
) noexcept;