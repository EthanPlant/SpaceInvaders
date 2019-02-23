#pragma once

#include <cstdint>
#include <iostream>

// Uncomment this if using the cpudiag rom
// #define CPUDIAG

#define CLOCK_SPEED 2000000
#define FPS 1/60

class I8080
{
    public:
        int cycles;
        int total_cycles = 0;
        int last_interrupt;

        void load_rom(const char* filename);
        void run_opcode();
        void generate_interrupt(uint interrupt);

    private:
        uint8_t memory[65536]; // 64 K of memory

        // Registers
        struct regs
        {
            uint8_t a;
            uint8_t b;
            uint8_t c;
            uint8_t d;
            uint8_t e;
            uint8_t h;
            uint8_t l;
        } regs;

        // Flags
        struct flags
        {
            uint8_t s; // Sign flag
            uint8_t z; // Zero flag
            uint8_t p; // Parity flag
            uint8_t c; // Carry flag
            uint8_t ac; //Auxiliary carry
        } flags;

        uint16_t sp; // Stack pointer
        uint16_t pc; // Program counter
        uint8_t opcode;

        void init();
        bool parity(int x, int size);
};
