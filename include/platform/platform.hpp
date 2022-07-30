#pragma once

#include "defines.hpp"
#include <iostream>

class Platform {
private:
public:
    Platform();
    ~Platform();

    float64 get_absolute_time();
    void sleep(uint64 ms);

    class Console {
    private:
    public:
        Console();
        ~Console();

        static void write(std::string message, int kind = 0);
        static std::string read();
    };

};
