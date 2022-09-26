#pragma once

#include <string>
#include "defines.hpp"

class String : public std::string {
public:
    using std::string::string;
    String();
    String(const std::string& __str) : std::string(__str) {}
    ~String();

    void to_lower();
    void to_upper();

    int32 compare_ci(const String& other) const;

};