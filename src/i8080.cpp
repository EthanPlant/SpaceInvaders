#include "i8080.hpp"

void I8080::init()
{
    // Reset values
    sp = 0;
    pc = 0;
    opcode = 0;

    // Clear registers
    regs.a = 0;
    regs.b = 0;
    regs.c = 0;
    regs.d = 0;
    regs.e = 0;
    regs.h = 0;
    regs.l = 0;

    // Clear flags
    flags.s = 0;
    flags.z = 0;
    flags.p = 0;
    flags.c = 0;
    flags.ac = 0;
    
    // Clear memory
    for (int i = 0; i < 65536; ++i)
    {
        memory[i] = 0;
    }
}

void I8080::load_rom(const char* filename)
{
    init();

    std::cout << "Opening ROM: " << filename << std::endl;

    FILE* rom = fopen(filename, "rb");
    if (rom == NULL)
    {
        std::cerr << "Couldn't open ROM" << std::endl;
        exit(1);
    }

    // Get file size
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    // Allocate memory to store ROM
    char* buffer = (char*) malloc(sizeof(char) * rom_size);
    if (buffer == NULL)
    {
        std::cerr << "Failed to allocate memory for rom" << std::endl;
        exit(2);
    }

    // Copy ROM into buffer
    size_t result = fread(buffer, sizeof(char), (size_t) rom_size, rom);
    if (result != rom_size)
    {
        std::cerr << "Failed to read ROM" << std::endl;
        exit(3);
    }

    if (rom_size < 65536 - 0x100)
    {
        for (int i = 0; i < rom_size; ++i)
        {
            #ifdef CPUDIAG
                memory[i + 0x100] = (uint8_t) buffer[i]; // Load into memory
            #else
                memory[i] = (uint8_t) buffer[i];
            #endif
        }
    }
    else
    {
        std::cerr << "ROM too large to fit in memory" << std::endl;
        exit(4);
    }

    // Clean up
    fclose(rom);
    free(buffer);

    #ifdef CPUDIAG
        pc = 100; // Set PC to the first instruction in the ROM
    
        // Set an exit point at the end of the test to avoid an infinite loop
        memory[0] = 0x27;

        // Fix the stack pointer  to the correct value
        memory[368] = 0x7;

        //  Skip DAA test as we don't need it
        memory[0x59c] = 0xc3; // JMP
        memory[0x59d] = 0xc2;
        memory[0x59e] = 0x05;
    #endif

    std::cout << "Loaded ROM successfully!" << std::endl;
}

bool I8080::parity(int x, int size)
{
    int i;
    int p = 0;
    x = (x & ((1 << size) - 1));
    for (i = 0; i < size; ++i)
    {
        if (x & 0x1) p++;
        x >>= 1;
    }
    return (p & 0x1) == 0;
}

void I8080::run_opcode()
{
    // Fetch opcode
    opcode = memory[pc];
    // Log out CPU trace
    std::cout << "================================================" << std::endl;
    std::cout << "PC: " << std::hex << pc << " | OP: " << std::hex << (unsigned int) opcode << std::endl;
    std::cout << "SP: " << std::hex << sp << std::endl;
    std::cout << "A: " << std::hex << (unsigned int) regs.a << std::endl;
    std::cout << "B: " << std::hex << (unsigned int) regs.b << std::endl;
    std::cout << "C: " << std::hex << (unsigned int) regs.c << std::endl;
    std::cout << "D: " << std::hex << (unsigned int) regs.d << std::endl;
    std::cout << "E: " << std::hex << (unsigned int) regs.e << std::endl;
    std::cout << "H: " << std::hex << (unsigned int) regs.h << std::endl;
    std::cout << "L: " << std::hex << (unsigned int) regs.l << std::endl;
    std::cout << "S: " << (unsigned int) flags.s << std::endl;
    std::cout << "Z: " << (unsigned int) flags.z << std::endl;
    std::cout << "P: " << (unsigned int) flags.p << std::endl;
    std::cout << "C: " << (unsigned int) flags.c << std::endl;
    std::cout << "AC: " << (unsigned int) flags.ac << std::endl;

    pc++; // Increment pc to next instruction

    switch (opcode)
    {
        case 0x00:
            std::cout << "NOP" << std::endl;
            cycles = 4;
            break;
        case 0x01:
            std::cout << "LXI B, d16" << std::endl;
            regs.b = memory[pc + 1];
            regs.c = memory[pc];
            cycles = 10;
            pc += 2;
            break;
        case 0x02:
            std::cout << "STAX B" << std::endl;
            memory[(regs.b << 8) | regs.c] = regs.a;
            cycles = 7;
            break;
        case 0x03:
            std::cout << "INX B" << std::endl;
            {
                uint16_t bc = (regs.b << 8) | regs.c;
                ++bc;
                regs.b = bc >> 8;
                regs.c = bc;
            }
            cycles = 5;
            break;
        case 0x04:
            std::cout << "INR B" << std::endl;
            ++regs.b;
            flags.s = (regs.b & 0x80) == 0x80;
            flags.z = regs.b == 0;
            flags.p = parity(regs.b, 8);
            cycles = 5;
            break;
        case 0x05:
            std::cout << "DCR B" << std::endl;
            --regs.b;
            flags.s = (0x80 == (regs.b & 0x80));
            flags.z = regs.b == 0;
            flags.p = parity(regs.b, 8);
            cycles = 5;
            break;
        case 0x06:
            std::cout << "MVI B, d8" << std::endl;
            regs.b = memory[pc];
            cycles = 7;
            pc++;
            break;
        case 0x07:
            std::cout << "RLC" << std::endl;
            flags.c = (regs.a >> 7);
            regs.a <<= 1;
            regs.a += flags.c;
            cycles = 4;
            break;
        case 0x09:
            std::cout << "DAD B" << std::endl;
            {
                uint16_t hl = regs.h << 8 | regs.l;
                uint16_t bc = regs.b << 8 | regs.c;
                hl += bc;
                regs.h = hl >> 8;
                regs.l = hl;
                flags.c = (hl & 0x0000FFFF) != 0;
            }
            cycles = 10;
            break;
        case 0x0A:
            std::cout << "LDAX B" << std::endl;
            regs.a = memory[(regs.b << 8) | regs.c];
            cycles = 7;
            break;
        case 0x0B:
            std::cout << "DCX B" << std::endl;
            {
                uint16_t bc = (regs.b << 8) | regs.c;
                --bc;
                regs.b = bc >> 8;
                regs.c = bc;
            }
            cycles = 5;
            break;
        case 0x0C:
            std::cout << "INR C" << std::endl;
            ++regs.c;
            flags.s = (regs.c & 0x80) == 0x80;
            flags.z = regs.c == 0;
            flags.p = parity(regs.c, 8);
            cycles = 5;
            break;
        case 0x0D:
            std::cout << "DCR C" << std::endl;
            --regs.c;
            flags.s = (0x80 == (regs.c & 0x80));
            flags.z = regs.c == 0;
            flags.p = parity(regs.c, 8);
            cycles = 5;
            break;
        case 0x0E:
            std::cout << "MVI C, d8" << std::endl;
            regs.c = memory[pc];
            pc++;
            cycles = 7;
            break;
        case 0x0F:
            std::cout << "RRC" << std::endl;
            {
                uint8_t x = regs.a;
                regs.a = ((x & 1) << 7) | (x >> 1);
                flags.c = (x & 1) == 1;
            }
            cycles = 4;
            break;
        case 0x11:
            std::cout << "LXI D, 16" << std::endl;
            regs.d = memory[pc + 1];
            regs.e = memory[pc];
            cycles = 10;
            pc += 2;
            break;
        case 0x12:
            std::cout << "STAX D" << std::endl;
            memory[(regs.d << 8) | regs.e] = regs.a;
            cycles = 7;
            break;
        case 0x13:
            std::cout << "INX D" << std::endl;
            {
                uint16_t de = (regs.d << 8) | regs.e;
                ++de;
                regs.d = de >> 8;
                regs.e = de;
            }
            cycles = 5;
            break;
        case 0x14:
            std::cout << "INR D" << std::endl;
            ++regs.d;
            flags.s = (regs.d & 0x80) == 0x80;
            flags.z = regs.d == 0;
            flags.p = parity(regs.d, 8);
            cycles = 5;
            break;
        case 0x15:
            std::cout << "DCR D" << std::endl;
            --regs.d;
            flags.s = (0x80 == (regs.d & 0x80));
            flags.z = regs.d == 0;
            flags.p = parity(regs.d, 8);
            cycles = 5;
            break;
        case 0x16:
            std::cout << "MVI D, d8" << std::endl;
            regs.d = memory[pc];
            pc++;
            cycles = 7;
            break;
        case 0x17:
            std::cout << "RAL" << std::endl;
            {
                uint8_t x = (regs.a >> 7);
                regs.a = (regs.a << 1) + flags.c;
                flags.c = x;
            }
            cycles = 4;
            break;
        case 0x19:
            std::cout << "DAD D" << std::endl;
            {
                uint16_t hl = regs.h << 8 | regs.l;
                uint16_t de = regs.d << 8 | regs.e;
                hl += de;
                regs.h = hl >> 8;
                regs.l = hl;
                flags.c = (hl & 0x0000FFFF) != 0;
            }
            cycles = 10;
            break;
        case 0x1A:
            std::cout << "LDAX D" << std::endl;
            regs.a = memory[(regs.d << 8) | regs.e];
            cycles = 7;
            break;
        case 0x1B:
            std::cout << "DCX D" << std::endl;
            {
                uint16_t de = (regs.d << 8) | regs.e;
                --de;
                regs.d = de >> 8;
                regs.e = de;
            }
            cycles = 5;
            break;
        case 0x1C:
            std::cout << "INR E" << std::endl;
            ++regs.e;
            flags.s = (regs.e & 0x80) == 0x80;
            flags.z = regs.e == 0;
            flags.p = parity(regs.e, 8);
            cycles = 5;
            break;
        case 0x1D:
            std::cout << "DCR E" << std::endl;
            --regs.e;
            flags.s = (0x80 == (regs.e & 0x80));
            flags.z = regs.e == 0;
            flags.p = parity(regs.e, 8);
            cycles = 5;
            break;
        case 0x1E:
            std::cout << "MVI E, d8" << std::endl;
            regs.e = memory[pc];
            pc++;
            cycles = 7;
            break;
        case 0x1F:
            std::cout << "RAR" << std::endl;
            {
                uint8_t x = (regs.a & 0b00000001);
                regs.a = (regs.a >> 1) + flags.c;
                flags.c = x;
            }
            cycles = 4;
            break;
        case 0x21:
            std::cout << "LXI H, d16" << std::endl;
            regs.h = memory[pc + 1];
            regs.l = memory[pc];
            pc += 2;
            cycles = 10;
            break;
        case 0x22:
            std::cout << "SHLD a16" << std::endl;
            memory[(memory[pc + 1] << 8) | memory[pc]] = regs.l;
            memory[(memory[pc + 1] << 8) | memory[pc] + 1] = regs.h;
            pc += 2;
            cycles = 16;
            break;
        case 0x23:
            std::cout << "INX H" << std::endl;
            {
                uint16_t hl = (regs.h << 8) | regs.l;
                ++hl;
                regs.h = hl >> 8;
                regs.l = hl;
            }
            cycles = 5;
            break;
        case 0x24:
            std::cout << "INR H" << std::endl;
            ++regs.h;
            flags.s = (regs.h & 0x80) == 0x80;
            flags.z = regs.h == 0;
            flags.p = parity(regs.h, 8);
            cycles = 5;
            break;
        case 0x25:
            std::cout << "DCR H" << std::endl;
            --regs.h;
            flags.s = (0x80 == (regs.h & 0x80));
            flags.z = regs.h == 0;
            flags.p = parity(regs.h, 8);
            cycles = 5;
            break;
        case 0x26:
            std::cout << "MVI H, d8" << std::endl;
            regs.h = memory[pc];
            pc++;
            cycles = 7;
            break;
        case 0x27:
            // Normally this would be DAA however Space Invaders never uses it
            // So instead we'll use it as a simple way to exit the ROM for cpudiag
            std::cout << "EXIT" << std::endl;
            exit(0);
            break;
        case 0x29:
            std::cout << "DAD H" << std::endl;
            {
                uint16_t hl = (regs.h << 8) | regs.l;
                hl += hl;
                regs.h = hl >> 8;
                regs.l = hl;
                flags.c = (hl & 0x0000FFFF) != 0;
            }
            cycles = 10;
            break;
        case 0x2A:
            std::cout << "LHLD a16" << std::endl;
            regs.l = memory[(memory[pc + 1] << 8) | memory[pc]];
            regs.h = memory[(memory[pc + 1] << 8) | memory[pc] + 1];
            pc += 2;
            cycles = 16;
            break;
        case 0x2B:
            std::cout << "DCX H" << std::endl;
            {
                uint16_t hl = (regs.h << 8) | regs.l;
                --hl;
                regs.h = hl >> 8;
                regs.l = hl;
            }
            cycles = 5;
            break;
        case 0x2C:
            std::cout << "INR L" << std::endl;
            ++regs.l;
            flags.s = (regs.l & 0x80) == 0x80;
            flags.z = regs.l == 0;
            flags.p = parity(regs.l, 8);
            cycles = 5;
            break;
        case 0x2D:
            std::cout << "DCR L" << std::endl;
            --regs.l;
            flags.s = (0x80 == (regs.l & 0x80));
            flags.z = regs.l == 0;
            flags.p = parity(regs.l , 8);
            cycles = 5;
            break;
        case 0x2E:
            std::cout << "MVI L, d8" << std::endl;
            regs.l = memory[pc];
            pc++;
            cycles = 7;
            break;
        case 0x2F:
            std::cout << "CMA" << std::endl;
            regs.a = ~regs.a;
            cycles = 4;
            break;
        case 0x31:
            std::cout << "LXI SP, d16" << std::endl;
            sp = (memory[pc + 1] << 8) | memory[pc];
            pc+= 2;
            cycles = 10;
            break;
        case 0x32:
            std::cout << "STA a16" << std::endl;
            memory[(memory[(pc + 1)] << 8) | memory[pc]] = regs.a;
            pc += 2;
            cycles = 13;
            break;
        case 0x33:
            std::cout << "INX SP" << std::endl;
            ++sp;
            cycles = 5;
            break;
        case 0x34:
            std::cout << "INR M" << std::endl;
            ++memory[(regs.h << 8) | regs.l];
            flags.s = (memory[(regs.h << 8) | regs.l] & 0x80) == 0x80;
            flags.z = memory[(regs.h << 8) | regs.l] == 0;
            flags.p = parity(memory[(regs.h << 8) | regs.l], 8);
            cycles = 10;
            break;
        case 0x35:
            std::cout << "DCR M" << std::endl;
            --memory[(regs.h << 8) | regs.l];;
            flags.s = (0x80 == (memory[(regs.h << 8) | regs.l] & 0x80));
            flags.z = memory[(regs.h << 8) | regs.l] == 0;
            flags.p = parity(memory[(regs.h << 8) | regs.l], 8);
            cycles = 10;
            break;
        case 0x36:
            std::cout << "MVI M, d8" << std::endl;
            memory[(regs.h) << 8 | regs.l] = memory[pc];
            pc++;
            cycles = 10;
            break;
        case 0x37:
            std::cout << "STC" << std::endl;
            flags.c = 1;
            cycles = 4;
            break;
        case 0x39:
            std::cout << "DAD SP" << std::endl;
            {
                uint16_t hl = (regs.h << 8) | regs.l;
                hl += sp;
                flags.c = (hl & 0x0000FFFF) != 0;
            }
            cycles = 10;
            break;
        case 0x3A:
            std::cout << "LDA a16" << std::endl; 
            regs.a = memory[(memory[(pc + 1)] << 8) | memory[pc]];
            pc += 2;
            cycles = 13;
            break;
        case 0x3B:
            std::cout << "DCX SP" << std::endl;
            --sp;
            cycles = 5;
            break;
        case 0x3C:
            std::cout << "INR A" << std::endl;
            ++regs.a;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.z = regs.a == 0;
            flags.p = parity(regs.a, 8);
            cycles = 5;
            break;
        case 0x3D:
            std::cout << "DCR A" << std::endl;
            --regs.a;
            flags.s = (0x80 == (regs.a & 0x80));
            flags.z = regs.a == 0;
            flags.p = parity(regs.a, 8);
            cycles = 5;
            break;
        case 0x3E:
            std::cout << "MVI A, d8" << std::endl;
            std::cout << std::hex << (uint16_t) memory[pc] << std::endl;
            regs.a = memory[pc];
            pc++;
            cycles = 7;
            break;
        case 0x3F:
            std::cout << "CMC" << std::endl;
            flags.c = !flags.c;
            cycles = 4;
        case 0x41:
            std::cout << "MOV B, C" << std::endl;
            regs.b = regs.c;
            cycles = 5;
            break;
        case 0x42:
            std::cout << "MOV B, D" << std::endl;
            regs.b = regs.d;
            cycles = 5;
            break;
        case 0x43:
            std::cout << "MOV B, E" << std::endl;
            regs.b = regs.e;
            cycles = 5;
            break;
        case 0x44:
            std::cout << "MOV B, H" << std::endl;
            regs.b = regs.h;
            cycles = 5;
            break;
        case 0x45:
            std::cout << "MOV B, L" << std::endl;
            regs.b = regs.l;
            cycles = 5;
            break;
        case 0x46:
            std::cout << "MOV B, M" << std::endl;
            regs.b = memory[(regs.h << 8) | regs.l];
            cycles = 7;
            break;
        case 0x47:
            std::cout << "MOV B, A" << std::endl;
            regs.b = regs.a;
            cycles = 5;
            break;
        case 0x48:
            std::cout << "MOV C, B" << std::endl;
            regs.c = regs.b;
            cycles = 5;
            break;
        case 0x4A:
            std::cout << "MOV C, D" << std::endl;
            regs.c = regs.d;
            cycles = 5;
            break;
        case 0x4B:
            std::cout << "MOV C, E" << std::endl;
            regs.c = regs.e;
            cycles = 5;
            break;
        case 0x4C:
            std::cout << "MOV C, H" << std::endl;
            regs.c = regs.h;
            cycles = 5;
            break;
        case 0x4D:
            std::cout << "MOV C, L" << std::endl;
            regs.c = regs.l;
            cycles = 5;
            break;
        case 0x4F:
            std::cout << "MOV C, A" << std::endl;
            regs.c = regs.a;
            cycles = 5;
            break;
        case 0x50:
            std::cout << "MOV D, B" << std::endl;
            regs.d = regs.b;
            cycles = 5;
            break;
        case 0x51:
            std::cout << "MOV D, C" << std::endl;
            regs.d = regs.c;
            cycles = 5;
            break;
        case 0x53:
            std::cout << "MOV D, E" << std::endl;
            regs.d = regs.e;
            cycles = 5;
            break;
        case 0x54:
            std::cout << "MOV D, H" << std::endl;
            regs.d = regs.h;
            cycles = 5;
            break;
        case 0x55:
            std::cout << "MOV D, L" << std::endl;
            regs.d = regs.l;
            cycles = 5;
            break;
        case 0x56: 
            std::cout << "MOV D, M" << std::endl;
            regs.d = memory[(regs.h << 8) | regs.l];
            cycles = 7;
            break;
        case 0x57:
            std::cout << "MOV D, A" << std::endl;
            regs.d = regs.a;
            cycles - 5;
            break;
        case 0x58:
            std::cout << "MOV E, B" << std::endl;
            regs.e = regs.b;
            cycles = 5;
            break;
        case 0x59:
            std::cout << "MOV E, C" << std::endl;
            regs.e = regs.c;
            cycles = 5;
            break;
        case 0x5A:
            std::cout << "MOV E, D" << std::endl;
            regs.e = regs.d;
            cycles = 5;
            break;
        case 0x5C:
            std::cout << "MOV E, H" << std::endl;
            regs.e = regs.h;
            cycles = 5;
            break;
        case 0x5D:
            std::cout << "MOV E, L" << std::endl;
            regs.e = regs.l;
            cycles = 5;
            break;
        case 0x5E:
            std::cout << "MOV E, M" << std::endl;
            regs.e = memory[(regs.h << 8) | regs.l];
            cycles = 7;
            break;
        case 0x5F:
            std::cout << "MOV E, A" << std::endl;
            regs.e = regs.a;
            cycles = 5;
            break;
        case 0x60:
            std::cout << "MOV H, B" << std::endl;
            regs.h = regs.b;
            cycles = 5;
            break;
        case 0x61:
            std::cout << "MOV H, C" << std::endl;
            regs.h = regs.c;
            cycles = 5;
            break;
        case 0x62:
            std::cout << "MOV H, D" << std::endl;
            regs.h = regs.d;
            cycles = 5;
            break;
        case 0x63:
            std::cout << "MOV H, E" << std::endl;
            regs.h = regs.e;
            cycles = 5;
            break;
        case 0x65: 
            std::cout << "MOV H, L" << std::endl;
            regs.h = regs.l;
            cycles = 5;
            break;
        case 0x66:
            std::cout << "MOV H, M" << std::endl;
            regs.h = memory[(regs.h << 8) | regs.l];
            cycles = 7;
            break;
        case 0x67:
            std::cout << "MOV H, A" << std::endl;
            regs.h = regs.a;
            cycles = 5;
            break;
        case 0x68:
            std::cout << "MOV L, B" << std::endl;
            regs.l = regs.b;
            cycles = 5;
            break;
        case 0x69:
            std::cout << "MOV L, C" << std::endl;
            regs.l = regs.c;
            cycles = 5;
            break;
        case 0x6A:
            std::cout << "MOV L, D" << std::endl;
            regs.l = regs.d;
            cycles = 5;
            break;
        case 0x6B:
            std::cout << "MOV L, E" << std::endl;
            regs.l = regs.e;
            cycles = 5;
            break;
        case 0x6C:
            std::cout << "MOV L, H" << std::endl;
            regs.l = regs.h;
            cycles = 5;
            break;
        case 0x6E:
            std::cout << "MOV L, M" << std::endl;
            regs.l = memory[(regs.h << 8) | regs.l];
            cycles = 7;
            break;
        case 0x6F:
            std::cout << "MOV L, A" << std::endl;
            regs.l = regs.a;
            cycles = 5;
            break;
        case 0x70:
            std::cout << "MOV M, B" << std::endl;
            memory[(regs.h << 8 | regs.l)] = regs.b;
            cycles = 7;
            break;
        case 0x72:
            std::cout << "MOV M, D" << std::endl;
            memory[(regs.h << 8) | regs.l] = regs.d;
            cycles = 7;
            break;
        case 0x73:
            std::cout << "MOV M, E" << std::endl;
            memory[(regs.h << 8) | regs.l] = regs.e;
            cycles = 7;
            break;
        case 0x74:
            std::cout << "MOV M, H" << std::endl;
            memory[(regs.h << 8) | regs.l] = regs.h;
            cycles = 7;
            break;
        case 0x75:
            std::cout << "MOV M, L" << std::endl;
            memory[(regs.h << 8) | regs.l] = regs.l;
            cycles = 7;
            break;
        case 0x77:
            std::cout << "MOV M, A" << std::endl;
            memory[(regs.h << 8) | regs.l] = regs.a;
            cycles = 7;
            break;
        case 0x78:
            std::cout << "MOV A, B" << std::endl;
            regs.a = regs.b;
            cycles = 5;
            break;
        case 0x79:
            std::cout << "MOV A, C" << std::endl;
            regs.a = regs.c;
            cycles = 5;
            break;
        case 0x7A:
            std::cout << "MOV A, D" << std::endl;
            regs.a = regs.d;
            cycles = 5;
            break;
        case 0x7B:
            std::cout << "MOV A, E" << std::endl;
            regs.a = regs.e;
            cycles = 5;
            break;
        case 0x7C:
            std::cout << "MOV A, H" << std::endl;
            regs.a = regs.h;
            cycles = 5;
            break;
        case 0x7D:
            std::cout << "MOV A, L" << std::endl;
            regs.a = regs.l;
            cycles = 5;
            break;
        case 0x7E:
            std::cout << "MOV A, M" << std::endl;
            regs.a = memory[(regs.h << 8) | regs.l];
            cycles = 7;
            break;
        case 0x80:
            std::cout << "ADD B" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.b;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x81:
            std::cout << "ADD C" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x82:
            std::cout << "ADD D" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.d;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x83:
            std::cout << "ADD E" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.e;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x84:
            std::cout << "ADD H" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.h;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x85:
            std::cout << "ADD L" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.l;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x86:
            std::cout << "ADD M" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) memory[(regs.h << 8) | regs.l];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            break;
        case 0x87:
            std::cout << "ADD A" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.a;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x88:
            std::cout << "ADC B" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.b + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x89:
            std::cout << "ADC C" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.c + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x8A:
            std::cout << "ADC D" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.d + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x8B:
            std::cout << "ADC E" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.e + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x8C:
            std::cout << "ADC H" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.h + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x8D:
            std::cout << "ADC L" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.l + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x8E:
            std::cout << "ADC M" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) memory[(regs.h << 8) | regs.l] + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            break;
        case 0x8F:
            std::cout << "ADC A" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) regs.a + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x90:
            std::cout << "SUB B" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.b;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < regs.b;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x91:
            std::cout << "SUB C" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < regs.c;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x92:
            std::cout << "SUB D" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.d;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < regs.d;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x93:
            std::cout << "SUB E" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.e;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < regs.e;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x94:
            std::cout << "SUB H" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.h;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < regs.h;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x95:
            std::cout << "SUB L" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.l;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < regs.l;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x96:
            std::cout << "SUB M" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) memory[(regs.h << 8) | regs.l];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < memory[(regs.h << 8) | regs.l];
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            break;
        case 0x97:
            std::cout << "SUB A" << std::endl;
            regs.a = 0;
            flags.z = 1;
            flags.s = 0;
            flags.c = 0;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0x98:
            std::cout << "SBB B" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.b - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.b + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x99:
            std::cout << "SBB C" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.c - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.c + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x9A:
            std::cout << "SBB D" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.d - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.d + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x9B:
            std::cout << "SBB E" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.e - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.e + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x9C:
            std::cout << "SBB H" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.h - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.h + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x9D:
            std::cout << "SBB L" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.l - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.l + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0x9E:
            std::cout << "SBB M" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) memory[(regs.h << 8) | regs.l] - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (memory[(regs.h << 8) | regs.l] + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            break;
        case 0x9F:
            std::cout << "SBB H" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) regs.a - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (0x80 == (ans & 0x80));
                flags.c = regs.a < (regs.a + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 4;
            break;
        case 0xA1:
            std::cout << "ANA C" << std::endl;
            regs.a &= regs.c;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA2:
            std::cout << "ANA D" << std::endl;
            regs.a &= regs.d;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA3:
            std::cout << "ANA E" << std::endl;
            regs.a &= regs.e;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA4:
            std::cout << "ANA H" << std::endl;
            regs.a &= regs.h;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA5:
            std::cout << "ANA L" << std::endl;
            regs.a &= regs.l;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA6:
            std::cout << "ANA M" << std::endl;
            regs.a &= memory[(regs.h << 8) | regs.l];
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 7;
            break;
        case 0xA7:
            std::cout << "ANA A" << std::endl;
            regs.a &= regs.a;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA8:
            std::cout << "XRA B" << std::endl;
            regs.a ^= regs.b;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xA9:
            std::cout << "XRA C" << std::endl;
            regs.a ^= regs.c;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xAA:
            std::cout << "XRA D" << std::endl;
            regs.a ^= regs.d;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xAB:
            std::cout << "XRA E" << std::endl;
            regs.a ^= regs.e;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xAC:
            std::cout << "XRA H" << std::endl;
            regs.a ^= regs.h;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xAD:
            std::cout << "XRA L" << std::endl;
            regs.a ^= regs.l;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xAE:
            std::cout << "XRA M" << std::endl;
            regs.a ^= memory[(regs.h << 8) | regs.l];
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 7;
            break;
        case 0xAF:
            std::cout << "XRA A" << std::endl;
            regs.a ^= regs.a;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB0:
            std::cout << "ORA B" << std::endl;
            regs.a |= regs.b;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB1:
            std::cout << "ORA C" << std::endl;
            regs.a |= regs.c;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB2:
            std::cout << "ORA D" << std::endl;
            regs.a |= regs.d;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB3:
            std::cout << "ORA E" << std::endl;
            regs.a |= regs.e;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB4:
            std::cout << "ORA H" << std::endl;
            regs.a |= regs.h;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB5:
            std::cout << "ORA L" << std::endl;
            regs.a |= regs.l;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB6:
            std::cout << "ORA M" << std::endl;
            regs.a |= memory[(regs.h << 8) | regs.l];
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 7;
            break;
        case 0xB7:
            std::cout << "ORA A" << std::endl;
            regs.a |= regs.a;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 4;
            break;
        case 0xB8:
            std::cout << "CMP B" << std::endl;
            {
                uint8_t ans = regs.a - regs.b;
                flags.z = ans == 0;
                flags.c = regs.a < regs.b;
            }
            cycles = 4;
            break;
        case 0xB9:
            std::cout << "CMP C" << std::endl;
            {
                uint8_t ans = regs.a - regs.c;
                flags.z = ans == 0;
                flags.c = regs.a < regs.c;
            }
            cycles = 4;
            break;
        case 0xBA:
            std::cout << "CMP D" << std::endl;
            {
                uint8_t ans = regs.a - regs.d;
                flags.z = ans == 0;
                flags.c = regs.a < regs.d;
            }
            cycles = 4;
            break;
        case 0xBB:
            std::cout << "CMP E" << std::endl;
            {
                uint8_t ans = regs.a - regs.e;
                flags.z = ans == 0;
                flags.c = regs.a < regs.d;
            }
            cycles = 4;
            break;
        case 0xBC:
            std::cout << "CMP H" << std::endl;
            {
                uint8_t ans = regs.a - regs.h;
                flags.z = ans == 0;
                flags.c = regs.a < regs.d;
            }
            cycles = 4;
            break;
        case 0xBD:
            std::cout << "CMP L" << std::endl;
            {
                uint8_t ans = regs.a - regs.l;
                flags.z = ans == 0;
                flags.c = regs.a < regs.l;
            }
            cycles = 4;
            break;
        case 0xBE:
            std::cout << "CMP M" << std::endl;
            {
                uint8_t ans = regs.a - memory[(regs.h << 8) | regs.l];
                flags.z = ans == 0;
                flags.c = regs.a < memory[(regs.a << 8) | regs.l];
            }
            cycles = 7;
            break;
        case 0xC0:
            std::cout << "RNZ" << std::endl;
            if (!flags.z)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            }
            else cycles = 5;
            break;
        case 0xC1:
            std::cout << "POP B" << std::endl;
            regs.b = memory[sp + 1];
            regs.c = memory[sp];
            sp += 2;
            cycles = 10;
            break;
        case 0xC2:
            std::cout << "JNZ a16" << std::endl;
            if (!flags.z) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xC3:
            std::cout << "JMP a16" << std::endl;
            pc = (memory[pc + 1] << 8) | memory[pc];
            cycles = 10;
            break;
        case 0xC4:
            std::cout << "CNZ a16" << std::endl;
            if (!flags.z)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xC5:
            std::cout << "PUSH B" << std::endl;
            memory[sp - 1] = regs.b;
            memory[sp - 2] = regs.c;
            sp -= 2;
            cycles = 11;
            break;
        case 0xC6:
            std::cout << "ADI d8"<< std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) memory[pc];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) == 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            pc++;
            break;
        case 0xC8:
            std::cout << "RZ" << std::endl;
            if (flags.z)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xC9:
            std::cout << "RET" << std::endl;
            pc = (memory[sp + 1] << 8) | memory[sp];
            sp += 2;
            cycles = 10;
            break;
        case 0xCA:
            std::cout << "JZ a16" << std::endl;
            if (flags.z) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xCC:
            std::cout << "CZ a16" << std::endl;
            if (flags.z)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else cycles = 11;
            {
                pc += 2;
            }
            break;
        case 0xCD:
            std::cout << "CALL a16" << std::endl;
            #ifdef CPUDIAG
                if (((memory[pc + 1] << 8) | memory[pc]) == 5)
                {
                    if (regs.c == 9)
                    {
                        uint16_t offset = (regs.d << 8) | regs.e;
                        uint8_t* str = &memory[offset + 3];
                        while (*str != '$')
                            std::cout << *str++;
                        std::cout << std::endl;
                    }
                    else if (regs.c == 2)
                    {
                        std::cout << "Print char routine called" << std::endl;
                    }
                }
                else if (((memory[pc + 1] << 8) | memory[pc]) == 0)
                {
                    exit(0);
                }
                else
            #endif
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
            }
            cycles = 17;
            break;
        case 0xCE:
            std::cout << "ACI d8" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) memory[pc] + flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) == 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            pc++;
            break;
        case 0xD0:
            std::cout << "RNC" << std::endl;
            if (!flags.c)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xD1:
            std::cout << "POP D" << std::endl;
            regs.d = memory[sp + 1];
            regs.e = memory[sp];
            sp += 2;
            cycles = 10;
            break;
        case 0xD2:
            std::cout << "JNC a16" << std::endl;
            if (!flags.c) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xD3:
            // Special instruction for IO to do later
            std::cout << "OUT d8" << std::endl;
            cycles = 10;
            pc++;
            break;
        case 0xD4:
            std::cout << "CNC a16" << std::endl;
            if (!flags.c)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xD5:
            std::cout << "PUSH D" << std::endl;
            memory[sp - 1] = regs.d;
            memory[sp - 2] = regs.e;
            sp -= 2;
            cycles = 11;
            break;
        case 0xD6:
            std::cout << "SUI d8" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) memory[pc];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) == 0x80;
                flags.c = regs.a < memory[pc];
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            pc++;
            break;
        case 0xD8:
            std::cout << "RC" << std::endl;
            if (flags.c)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xDA:
            std::cout << "JC a16" << std::endl;
            if (flags.c) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xDC:
            std::cout << "CC a16" << std::endl;
            if (flags.c)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xDE:
            std::cout << "SBI d8" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a - (uint16_t) memory[pc] - flags.c;
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) == 0x80;
                flags.c = regs.a < (memory[pc] + flags.c);
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            pc++;
            break;
        case 0xE0:
            std::cout << "RPO" << std::endl;
            if (!flags.p)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xE1:
            std::cout << "POP H" << std::endl;
            regs.h = memory[sp + 1];
            regs.l = memory[sp];
            sp += 2;
            cycles = 10;
            break;
        case 0xE2:
            std::cout << "JPO a16" << std::endl;
            if (!flags.p) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xE3:
            std::cout << "XTHL" << std::endl;
            {
                uint16_t stack = (memory[sp + 1] << 8) | memory[sp];
                memory[sp] = regs.l;
                memory[sp + 1] = regs.h;
                regs.h = stack >> 8;
                regs.l = stack;
            }
            cycles = 18;
            break;
        case 0xE4:
            std::cout << "CPO a16" << std::endl;
            if (!flags.p)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xE5:
            std::cout << "PUSH H" << std::endl;
            memory[sp - 1] = regs.h;
            memory[sp - 2] = regs.l;
            sp -= 2;
            cycles = 11;
            break;
        case 0xE6:
            std::cout << "ANI d8" << std::endl;
            regs.a &= memory[pc];
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            cycles = 7;
            pc++;
            break;
        case 0xE8:
            std::cout << "RPE" << std::endl;
            if (flags.p)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xE9:
            std::cout << "PCHL" << std::endl;
            pc = ((regs.h << 8) | regs.l);
            cycles = 5;
            break;
        case 0xEA:
            std::cout << "JPE a16" << std::endl;
            if (flags.p) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xEB:
            std::cout << "XCHG" << std::endl;
            {
                uint8_t save1 = regs.d;
                uint8_t save2 = regs.e;
                regs.d = regs.h;
                regs.e = regs.l;
                regs.h = save1;
                regs.l = save2;
            }
            cycles = 5;
            break;
        case 0xEC:
            std::cout << "CPE a16" << std::endl;
            if (flags.p)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xEE:
            std::cout << "XRA d8" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a ^ (uint16_t) memory[pc];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) == 0x80;
                flags.c = 0;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            pc++;
            break;
        case 0xF0:
            std::cout << "RP" << std::endl;
            if (!flags.s)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xF1:
            std::cout << "POP PSW" << std::endl;
            regs.a = memory[sp + 1];
            {
                uint8_t psw = memory[sp];
                flags.z = (psw & 0x01) == 0x01;
                flags.s = (psw & 0x02) == 0x02;
                flags.p = (psw & 0x04) == 0x04;
                flags.c = (psw & 0x08) == 0x05;
                flags.ac = (psw & 0x10) == 0x10;
            }
            cycles = 10;
            sp += 2;
            break;
        case 0xF2:
            std::cout << "JP a16" << std::endl;
            if (!flags.s) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            cycles = 10;
            break;
        case 0xF4:
            std::cout << "CP a16" << std::endl;
            if (!flags.s)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xF5:
            std::cout << "PUSH PSW" << std::endl;
            memory[sp - 1] = regs.a;
            {
                uint8_t psw = (
                    flags.z |
                    flags.s << 1 |
                    flags.p << 2 |
                    flags.c << 3 |
                    flags.ac << 4);
                memory[sp - 2] = psw;
                sp -= 2;
            }
            cycles = 11;
            break;
        case 0xF6:
            std::cout << "ORI d8" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a | (uint16_t) memory[pc];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) == 0x80;
                flags.c = 0;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            cycles = 7;
            pc++;
            break;
        case 0xF8:
            std::cout << "RM" << std::endl;
            if (flags.s)
            {
                pc = (memory[sp + 1] << 8) | memory[sp];
                sp += 2;
                cycles = 11;
            } else cycles = 5;
            break;
        case 0xF9:
            std::cout << "SPHL" << std::endl;
            sp = (regs.h << 8) | regs.l;
            cycles = 5;
            break;
        case 0xFA:
            std::cout << "JM a16" << std::endl;
            if (flags.s) pc = (memory[pc + 1] << 8) | memory[pc];
            else pc += 2;
            cycles = 10;
            break;
        case 0xFB:
            // Special instruction for interupts to do later
            std::cout << "EI" << std::endl;
            cycles = 4;
            break;
        case 0xFC:
            std::cout << "CM a16" << std::endl;
            if (flags.s)
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
                cycles = 17;
            }
            else
            {
                pc += 2;
                cycles = 11;
            }
            break;
        case 0xFE:
            std::cout << "CPI d8" << std::endl;
            {
                uint8_t x = regs.a - memory[pc];
                flags.s = ((x & 0x80) == 0x80);
                flags.z = x == 0;
                flags.p = parity(x, 8);
                flags.c = (regs.a < memory[pc]);
            }
            cycles = 7;
            pc++;
            break;
        default: 
            std::cerr << "Unimplimented Instruction" << std::endl;
            exit(5);
            break;
    }
    total_cycles += cycles;
}

void I8080::generate_interrupt(uint interrupt)
{
    // Push PC to the stack
    memory[sp - 1] = pc >> 8;
    memory[sp - 2] = pc;
    sp -= 2;

    // Generate the interrupt
    pc = (interrupt & 0xFFFF);

    // Save the previous interrupt
    last_interrupt = interrupt;
}