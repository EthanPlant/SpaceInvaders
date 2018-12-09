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

    if (rom_size < 65536)
    {
        for (int i = 0; i < rom_size; ++i)
        {
           memory[i] = (uint8_t) buffer[i]; // Load into memory
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
            break;
        case 0x01:
            std::cout << "LXI B, d16" << std::endl;
            regs.b = memory[pc + 1];
            regs.c = memory[pc];
            pc += 2;
            break;
        case 0x05:
            std::cout << "DCR B" << std::endl;
            --regs.b;
            flags.s = (0x80 == (regs.b & 0x80));
            flags.z = regs.b == 0;
            flags.p = parity(regs.b, 8);
            break;
        case 0x06:
            std::cout << "MVI B, d8" << std::endl;
            regs.b = memory[pc];
            pc++;
            break;
        case 0x09:
            std::cout << "DAD B" << std::endl;
            {
                uint16_t hl = regs.h << 8 | regs.l;
                uint16_t bc = regs.b << 8 | regs.c;
                hl += bc;
                regs.h = hl >> 8;
                regs.l = hl;
                flags.c = (hl & 0xFFFF0000) != 0;
            }
            break;
        case 0x0D:
            std::cout << "DCR C" << std::endl;
            --regs.c;
            flags.s = (0x80 == (regs.c & 0x80));
            flags.z = regs.c == 0;
            flags.p = parity(regs.c, 8);
            break;
        case 0x0E:
            std::cout << "MVI C, d8" << std::endl;
            regs.c = memory[pc];
            pc++;
            break;
        case 0x0F:
            std::cout << "RRC" << std::endl;
            {
                uint8_t x = regs.a;
                regs.a = ((x & 1) << 7) | (x >> 1);
                flags.c = (x & 1) == 1;
            }
            break;
        case 0x11:
            std::cout << "LXI D, 16" << std::endl;
            regs.d = memory[pc + 1];
            regs.e = memory[pc];
            pc += 2;
            break;
        case 0x13:
            std::cout << "INX D" << std::endl;
            {
                uint16_t de = (regs.d << 8) | regs.e;
                ++de;
                regs.d = de >> 8;
                regs.e = de;
            }
            break;
        case 0x19:
            std::cout << "DAD D" << std::endl;
            {
                uint16_t hl = regs.h << 8 | regs.l;
                uint16_t de = regs.d << 8 | regs.e;
                hl += de;
                regs.h = hl >> 8;
                regs.l = hl;
                flags.c = (hl & 0xFFFF0000) != 0;
            }
            break;
        case 0x1A:
            std::cout << "LDAX D" << std::endl;
            regs.a = memory[(regs.d << 8) | regs.e];
            break;
        case 0x21:
            std::cout << "LXI H, d16" << std::endl;
            regs.h = memory[pc + 1];
            regs.l = memory[pc];
            pc += 2;
            break;
        case 0x23:
            std::cout << "INX H" << std::endl;
            {
                uint16_t hl = (regs.h << 8) | regs.l;
                ++hl;
                regs.h = hl >> 8;
                regs.l = hl;
            }
            break;
        case 0x26:
            std::cout << "MVI H, d8" << std::endl;
            regs.h = memory[pc];
            pc++;
            break;
        case 0x29:
            std::cout << "DAD H" << std::endl;
            {
                uint16_t hl = (regs.h << 8) | regs.l;
                hl += hl;
                regs.h = hl >> 8;
                regs.l = hl;
                flags.c = (hl & 0xFFFF0000) != 0;
            }
            break;
        case 0x31:
            std::cout << "LXI SP, d16" << std::endl;
            sp = (memory[pc + 1] << 8) | memory[pc];
            pc+= 2;
            break;
        case 0x32:
            std::cout << "STA a16" << std::endl;
            memory[(memory[(pc + 1)] << 8) | memory[pc]] = regs.a;
            pc += 2;
            break;
        case 0x36:
            std::cout << "MVI M, d8" << std::endl;
            memory[(regs.h) << 8 | regs.l] = memory[pc];
            pc++;
            break;
        case 0x3A:
            std::cout << "LDA a16" << std::endl; 
            regs.a = memory[(memory[(pc + 1)] << 8) | memory[pc]];
            pc += 2;
            break;
        case 0x3E:
            std::cout << "MVI A, d8" << std::endl;
            regs.a = memory[pc];
            pc++;
            break;
        case 0x56:
            std::cout << "MOV D, M" << std::endl;
            regs.d = memory[(regs.h << 8) | regs.l];
            break;
        case 0x5E:
            std::cout << "MOV E, M" << std::endl;
            regs.e = memory[(regs.h << 8) | regs.l];
            break;
        case 0x66:
            std::cout << "MOV H, M" << std::endl;
            regs.h = memory[(regs.h << 8) | regs.l];
            break;
        case 0x6F:
            std::cout << "MOV L, A" << std::endl;
            regs.l = regs.a;
            break;
        case 0x77:
            std::cout << "MOV M, A" << std::endl;
            memory[(regs.h << 8) | regs.l] = regs.a;
            break;
        case 0x7A:
            std::cout << "MOV A, D" << std::endl;
            regs.a = regs.d;
            break;
        case 0x7B:
            std::cout << "MOV A, E" << std::endl;
            regs.a = regs.e;
            break;
        case 0x7C:
            std::cout << "MOV A, H" << std::endl;
            regs.a = regs.h;
            break;
        case 0x7E:
            std::cout << "MOV A, M" << std::endl;
            regs.a = memory[(regs.h << 8) | regs.l];
            break;
        case 0xA7:
            std::cout << "ANA A" << std::endl;
            regs.a &= regs.a;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            break;
        case 0xAF:
            std::cout << "XRA A" << std::endl;
            regs.a ^= regs.a;
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
        case 0xC1:
            std::cout << "POP B" << std::endl;
            regs.b = memory[sp + 1];
            regs.c = memory[sp];
            sp += 2;
            break;
        case 0xC2:
            std::cout << "JNZ a16" << std::endl;
            if (!flags.z) pc = (memory[pc + 1] << 8) | memory[pc]; 
            else pc += 2;
            break;
        case 0xC3:
            std::cout << "JMP a16" << std::endl;
            pc = (memory[pc + 1] << 8) | memory[pc];
            break;
        case 0xC5:
            std::cout << "PUSH B" << std::endl;
            memory[sp - 1] = regs.b;
            memory[sp - 2] = regs.c;
            sp -= 2;
            break;
        case 0xC6:
            std::cout << "ADI d8" << std::endl;
            {
                uint16_t ans = (uint16_t) regs.a + (uint16_t) memory[pc];
                flags.z = (ans & 0xFF) == 0;
                flags.s = (ans & 0x80) != 0x80;
                flags.c = ans > 0xFF;
                flags.p = parity((ans & 0xFF), 8);
                regs.a = (uint8_t) ans;
            }
            pc++;
            break;
        case 0xC9:
            std::cout << "RET" << std::endl;
            pc = (memory[sp + 1] << 8) | memory[sp];
            sp += 2;
            break;
        case 0xCD:
            std::cout << "CALL a16" << std::endl;
            {
                uint16_t ret = pc + 2;
                memory[sp - 1] = (ret >> 8);
                memory[sp - 2] = ret;
                sp -= 2;
                pc = (memory[pc + 1] << 8) | memory[pc];
            }
            break;
        case 0xD1:
            std::cout << "POP D" << std::endl;
            regs.d = memory[sp + 1];
            regs.e = memory[sp];
            sp += 2;
            break;
        case 0xD3:
            // Special instruction for IO to do later
            std::cout << "OUT d8" << std::endl;
            pc++;
            break;
        case 0xD5:
            std::cout << "PUSH D" << std::endl;
            memory[sp - 1] = regs.d;
            memory[sp - 2] = regs.e;
            sp -= 2;
            break;
        case 0xE1:
            std::cout << "POP H" << std::endl;
            regs.h = memory[sp + 1];
            regs.l = memory[sp];
            sp += 2;
            break;
          case 0xE5:
            std::cout << "PUSH H" << std::endl;
            memory[sp - 1] = regs.h;
            memory[sp - 2] = regs.l;
            sp -= 2;
            break;
        case 0xE6:
            std::cout << "ANI d8" << std::endl;
            regs.a &= memory[pc];
            flags.c = 0;
            flags.z = regs.a == 0;
            flags.s = (regs.a & 0x80) == 0x80;
            flags.p = parity(regs.a, 8);
            pc++;
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
            sp += 2;
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
            break;
        case 0xFB:
            // Special instruction for interupts to do later
            std::cout << "EI" << std::endl;
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
            pc++;
            break;
        default: 
            std::cerr << "Unimplimented Instruction" << std::endl;
            exit(5);
            break;
    }
}