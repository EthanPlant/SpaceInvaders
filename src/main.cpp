#include <iostream>
#include <chrono>
#include <thread>

#include "i8080.hpp"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: invaders <ROM>" << std::endl;
        return 6;
    }

    I8080 i8080 = I8080();

    // Attempt to laod ROM
    load:
        i8080.load_rom(argv[1]);

    // Emulation loop
    while (true)
    {
        i8080.run_opcode();
        std::this_thread::sleep_for(std::chrono::nanoseconds((1 / CLOCK_SPEED) * 1000000000) * i8080.cycles); // Sleep to slow emulation time
    }
}