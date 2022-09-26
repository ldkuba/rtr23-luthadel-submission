#include "string.hpp"

#include <algorithm>

String::String() {}
String::~String() {}

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

int32 String::compare_ci(const String& other) const {
    String a = *this; a.to_lower();
    String b = other; b.to_lower();
    return a.compare(b);
}