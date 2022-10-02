#include "string.hpp"

#include <algorithm>
#include "logger.hpp"

template<typename T>
T parse_uint(const char* s, const uint32 n, const T max);
template<typename T>
T parse_int(const char* s, const uint32 n, const T max);

// Constructor & Destructor
String::String() {}
String::~String() {}

// /////////////////////// //
// STRING PUBLIC FUNCTIONS //
// /////////////////////// //

// Character transforms
void String::to_lower() {
    std::transform(this->cbegin(), this->cend(),
        this->begin(), // write to the same location
        [](uchar c) { return std::tolower(c); });
}
void String::to_upper() {
    std::transform(this->cbegin(), this->cend(),
        this->begin(), // write to the same location
        [](uchar c) { return std::toupper(c); });
}

// Trim methods
void String::trim_left() {
    auto is_space = [](uchar ch) { return !std::isspace(ch); };
    erase(begin(), std::find_if(begin(), end(), is_space));
}
void String::trim_right() {
    auto is_space = [](uchar ch) { return !std::isspace(ch); };
    erase(std::find_if(rbegin(), rend(), is_space).base(), end());
}
void String::trim() {
    trim_left();
    trim_right();
}

// Compare methods
int32 String::compare_ci(const String& other) const {
    String a = *this; a.to_lower();
    String b = other; b.to_lower();
    return a.compare(b);
}

// Split methods
std::vector<String> String::split(const String delimiter) const {
    std::size_t pos = 0;
    std::size_t next = 0;
    std::vector<String> fields;
    while (next != std::string::npos) {
        next = this->find_first_of(delimiter, pos);
        String field = next == std::string::npos ? this->substr(pos) : this->substr(pos, next - pos);
        fields.push_back(field);
        pos = next + 1;
    }
    return fields;
}
std::vector<String> String::split(const char delimiter) const {
    std::size_t pos = 0;
    std::size_t next = 0;
    std::vector<String> fields;
    while (next != std::string::npos) {
        next = this->find_first_of(delimiter, pos);
        String field = next == std::string::npos ? this->substr(pos) : this->substr(pos, next - pos);
        fields.push_back(field);
        pos = next + 1;
    }
    return fields;
}

// Parse methods
uint8 String::parse_as_uint8() {
    return parse_uint<uint8>(data(), length(), UINT8_MAX);
}
uint16 String::parse_as_uint16() {
    return parse_uint<uint16>(data(), length(), UINT16_MAX);
}
uint32 String::parse_as_uint32() {
    return parse_uint<uint32>(data(), length(), UINT32_MAX);
}
uint64 String::parse_as_uint64() {
    return parse_uint<uint64>(data(), length(), UINT64_MAX);
}
uint128 String::parse_as_uint128() {
    if (length() > 39)
        throw std::invalid_argument("String couldn't be parsed.");
    Logger::warning("Correct conversion to uint128 is not guaranteed, overflow may still occur.");
    return parse_uint<uint128>(data(), length(), UINT128_MAX);
}
int8 String::parse_as_int8() {
    return parse_int<uint8>(data(), length(), INT8_MAX);
}
int16 String::parse_as_int16() {
    return parse_int<uint16>(data(), length(), INT16_MAX);
}
int32 String::parse_as_int32() {
    return parse_int<uint32>(data(), length(), INT32_MAX);
}
int64 String::parse_as_int64() {
    return parse_int<uint64>(data(), length(), INT64_MAX);
}
int128 String::parse_as_int128() {
    return parse_int<uint128>(data(), length(), INT128_MAX);
}
float32 String::parse_as_float32() {
    size_t ending;
    float32 result = std::stof(*this, &ending);
    if (size() - ending != 0)
        throw std::invalid_argument("String couldn't be parsed.");
    return result;
}
float64 String::parse_as_float64() {
    size_t ending;
    float64 result = std::stod(*this, &ending);
    if (size() - ending != 0)
        throw std::invalid_argument("String couldn't be parsed.");
    return result;
}
float128 String::parse_as_float128() {
    size_t ending;
    float128 result = std::stold(*this, &ending);
    if (size() - ending != 0)
        throw std::invalid_argument("String couldn't be parsed.");
    return result;
}

// /////////////////////// //
// STRING HELPER FUNCTIONS //
// /////////////////////// //

template<typename T>
T parse_uint(const char* s, const uint32 n, const T max) {
    T result = 0;
    uint128 power = 1;
    for (int32 i = n - 1; i >= 0; i--) {
        uint32 digit = s[i] - '0';
        if (digit / 10 != 0)
            return -1;

        if (max - result < power * digit)
            throw std::invalid_argument("String couldn't be parsed.");

        result += power * digit;
        power *= 10;
    }
    return result;
}
template<typename T>
T parse_int(const char* s, const uint32 n, const T max) {
    if (s[0] == '-')
        return -parse_uint<T>(s + 1, n - 1, max - 1);
    return parse_uint<T>(s, n, max);
}