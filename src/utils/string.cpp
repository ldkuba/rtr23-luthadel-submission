#include "string.hpp"

#include <algorithm>
#include "logger.hpp"
#include "property.hpp"

namespace ENGINE_NAMESPACE {

template<typename T>
Result<T, InvalidArgument> parse_uint(
    const char* s, const uint32 n, const T max
);
template<typename T>
Result<T, InvalidArgument> parse_int(
    const char* s, const uint32 n, const T max
);

// Constructor & Destructor
String::String() noexcept {}
String::~String() noexcept {}

// /////////////////////// //
// STRING PUBLIC FUNCTIONS //
// /////////////////////// //

#define copy_method(inplace_method)                                            \
    String copy { *this };                                                     \
    copy.inplace_method();                                                     \
    return copy

// Character transforms
void String::to_lower() noexcept {
    std::transform(
        this->cbegin(),
        this->cend(),
        this->begin(), // write to the same location
        [](uchar c) { return std::tolower(c); }
    );
}
void String::to_upper() noexcept {
    std::transform(
        this->cbegin(),
        this->cend(),
        this->begin(), // write to the same location
        [](uchar c) { return std::toupper(c); }
    );
}

String String::lower_c() const noexcept { copy_method(to_lower); }
String String::upper_c() const noexcept { copy_method(to_upper); }

// Trim methods
void String::trim_left() noexcept {
    auto is_space = [](uchar ch) { return !std::isspace(ch); };
    erase(begin(), std::find_if(begin(), end(), is_space));
}
void String::trim_right() noexcept {
    auto is_space = [](uchar ch) { return !std::isspace(ch); };
    erase(std::find_if(rbegin(), rend(), is_space).base(), end());
}
void String::trim() noexcept {
    trim_left();
    trim_right();
}

String String::trimmed_left_c() const noexcept { copy_method(trim_left); }
String String::trimmed_right_c() const noexcept { copy_method(trim_right); }
String String::trimmed_c() const noexcept { copy_method(trim); }

// Compare methods
int32 String::compare_ci(const String& other) const {
    String a = *this;
    a.to_lower();
    String b = other;
    b.to_lower();
    return a.compare(b);
}

// Split methods
Vector<String> String::split(const String delimiter) const {
    std::size_t    pos  = 0;
    std::size_t    next = 0;
    Vector<String> fields;
    while (next != std::string::npos) {
        next         = this->find_first_of(delimiter, pos);
        String field = next == std::string::npos
                           ? this->substr(pos)
                           : this->substr(pos, next - pos);
        fields.push_back(field);
        pos = next + 1;
    }
    return fields;
}
Vector<String> String::split(const char delimiter) const {
    std::size_t    pos  = 0;
    std::size_t    next = 0;
    Vector<String> fields;
    while (next != std::string::npos) {
        next         = this->find_first_of(delimiter, pos);
        String field = next == std::string::npos
                           ? this->substr(pos)
                           : this->substr(pos, next - pos);
        fields.push_back(field);
        pos = next + 1;
    }
    return fields;
}

// Parse methods
Result<uint8, InvalidArgument> String::parse_as_uint8() {
    return parse_uint<uint8>(data(), length(), UINT8_MAX);
}
Result<uint16, InvalidArgument> String::parse_as_uint16() {
    return parse_uint<uint16>(data(), length(), UINT16_MAX);
}
Result<uint32, InvalidArgument> String::parse_as_uint32() {
    return parse_uint<uint32>(data(), length(), UINT32_MAX);
}
Result<uint64, InvalidArgument> String::parse_as_uint64() {
    return parse_uint<uint64>(data(), length(), UINT64_MAX);
}
Result<uint128, InvalidArgument> String::parse_as_uint128() {
    if (length() > 39) return Failure("String couldn't be parsed.");
    Logger::warning("Correct conversion to uint128 is not guaranteed, overflow "
                    "may still occur.");
    return parse_uint<uint128>(data(), length(), UINT128_MAX);
}
Result<int8, InvalidArgument> String::parse_as_int8() {
    return parse_int<int8>(data(), length(), INT8_MAX);
}
Result<int16, InvalidArgument> String::parse_as_int16() {
    return parse_int<int16>(data(), length(), INT16_MAX);
}
Result<int32, InvalidArgument> String::parse_as_int32() {
    return parse_int<int32>(data(), length(), INT32_MAX);
}
Result<int64, InvalidArgument> String::parse_as_int64() {
    return parse_int<int64>(data(), length(), INT64_MAX);
}
Result<int128, InvalidArgument> String::parse_as_int128() {
    return parse_int<int128>(data(), length(), INT128_MAX);
}
Result<float32, InvalidArgument> String::parse_as_float32() {
    size_t  ending;
    float32 result = std::stof(*this, &ending);
    if (size() - ending != 0) return Failure("String couldn't be parsed.");
    return result;
}
Result<float64, InvalidArgument> String::parse_as_float64() {
    size_t  ending;
    float64 result = std::stod(*this, &ending);
    if (size() - ending != 0) return Failure("String couldn't be parsed.");
    return result;
}
Result<float128, InvalidArgument> String::parse_as_float128() {
    size_t   ending;
    float128 result = std::stold(*this, &ending);
    if (size() - ending != 0) return Failure("String couldn't be parsed.");
    return result;
}

// /////////////////////// //
// STRING HELPER FUNCTIONS //
// /////////////////////// //

template<typename T>
Result<T, InvalidArgument> parse_uint(
    const char* s, const uint32 n, const T max
) {
    T       result = 0;
    uint128 power  = 1;
    for (int32 i = n - 1; i >= 0; i--) {
        uint32 digit = s[i] - '0';
        if (digit / 10 != 0) return -1;

        if (max - result < power * digit)
            return Failure("String couldn't be parsed.");

        result += power * digit;
        power *= 10;
    }
    return result;
}
template<typename T>
T change_sign(T x) {
    return -x;
}

template<typename T>
Result<T, InvalidArgument> parse_int(
    const char* s, const uint32 n, const T max
) {
    if (s[0] == '-') {
        return parse_uint<T>(s + 1, n - 1, max - 1).map([](T a) -> T {
            return -a;
        });
    }

    return parse_uint<T>(s, n, max);
}

// String builder specializations
template<>
void String::add_to_string<char>(
    String& out_string, const char& component
) noexcept {
    out_string += component;
}
template<>
void String::add_to_string<char>(
    String& out_string, char* const component
) noexcept {
    out_string += String(component);
}
template<>
void String::add_to_string<const char>(
    String& out_string, const char* const component
) noexcept {
    out_string += String(component);
}
template<>
void String::add_to_string<std::string>(
    String& out_string, const std::string& component
) noexcept {
    out_string += component;
}
template<>
void String::add_to_string<String>(
    String& out_string, const String& component
) noexcept {
    out_string += component;
}

} // namespace ENGINE_NAMESPACE

using namespace ENGINE_NAMESPACE;

// Additional to_string conversions
namespace std {
string to_string(const uint128& in) {
    uint8   digit = in % 10;
    uint128 num   = in / 10;
    string  res   = to_string(digit);
    while (num != 0) {
        digit = num % 10;
        num   = num / 10;
        res   = to_string(digit) + res;
    }
    return res;
}
string to_string(const int128& in) {
    if (in > 0) return to_string((uint128) in);
    return "-" + to_string((uint128) -in);
}
template<typename T>
string to_string(const Property<T>& in) {
    return std::to_string(in());
}
template<>
string to_string<String>(const Property<String>& in) {
    return in();
}
string to_string(const Vector<char>& in) {
    return string((char*) in.data(), in.size());
}
string to_string(const Vector<unsigned char>& in) {
    return string((char*) in.data(), in.size());
}
} // namespace std