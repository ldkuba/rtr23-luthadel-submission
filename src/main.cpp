#include <iostream>
#include <GLFW/glfw3.h>

#include "event.hpp"
#include "platform.hpp"
#include "logger.hpp"

int main(int, char**) {

    Platform p = Platform();

    Logger::fatal("Some FATAL ERROR! idk LOL.");
    Logger::error("Some error idk LOL.");
    Logger::warning("Some warning idk LOL.");
    Logger::log("Some text idk LOL.");
    Logger::debug("Some debug idk LOL.");


    std::string a = " AAAAAAAAAAA ";
    std::string b = " BBBBBBBBBBB ";

    std::cout << (a + b) << std::endl;

    return 0;
}