#pragma once

#include <string>
#include <vector>
#include "defines.hpp"

class String : public std::string {
public:
    using std::string::string;
    String();
    String(const std::string& __str) : std::string(__str) {}
    ~String();

    void to_lower();
    void to_upper();

    void trim_left();
    void trim_right();
    void trim();

    int32 compare_ci(const String& other) const;
    std::vector<String> split(const String delimiter) const;
    std::vector<String> split(const char delimiter) const;

    // Parsing
    uint8 parse_as_uint8();
    uint16 parse_as_uint16();
    uint32 parse_as_uint32();
    uint64 parse_as_uint64();
    uint128 parse_as_uint128();
    int8 parse_as_int8();
    int16 parse_as_int16();
    int32 parse_as_int32();
    int64 parse_as_int64();
    int128 parse_as_int128();
    float32 parse_as_float32();
    float64 parse_as_float64();
    float128 parse_as_float128();
};

namespace std {
    template<> struct hash<String> {
        size_t operator()(String const& str) const {
            return hash<string>()(str);
        }
    };
}