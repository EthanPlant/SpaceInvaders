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
        if (i8080.total_cycles >= (CLOCK_SPEED / FPS) / 2) 
        {
            if (i8080.last_interrupt != 0x0008) i8080.generate_interrupt(0x0008);
            else i8080.generate_interrupt(0x0010);
            i8080.total_cycles = 0;
        }
    }
}