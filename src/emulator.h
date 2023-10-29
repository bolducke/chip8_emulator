#include <cstdint>
#include <string.h>
#include <algorithm>    // std::copy
#include <stdlib.h>     /* srand, rand */
#include <cmath>

#include <fmt/core.h>

namespace chip8 {

    const unsigned int C8_REGISTER_SIZE = 16;
    const unsigned int C8_MEMORY_SIZE = 4096;
    const unsigned int C8_STACK_SIZE = 12;
    const unsigned int C8_KEYPAD_SIZE = 16;
    const unsigned int C8_DISPLAY_WIDTH = 64;
    const unsigned int C8_DISPLAY_HEIGHT = 32;

    const unsigned int C8_FONTSET_SIZE = 80;
    const unsigned int C8_FONT_SIZE = 5;
    const unsigned int C8_FONTSET_START_ADDRESS = 0x50;
    const unsigned int C8_START_ADDRESS = 0x200;

    const uint8_t C8_FONTSET[C8_FONTSET_SIZE] =
        {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };

    // {} inside struct -> value-initialization -> zero-initialization

    // Based on the specification of https://en.wikipedia.org/wiki/CHIP-8
    // The following specification is based on the SUPER-CHIP specification from 1991 
    // (but without the additional opcodes that provide extended functionality), as that is the most commonly encountered extension set today
    struct CHIP8EmulatorState {
        /////// Memory ///////
        // Register > 16 byte
        //  * Address from V0 to VF
        uint8_t V[C8_REGISTER_SIZE]{};
        
        // Main Memory > 4 KibiByte  (4096 bytes)
        // +---------------+= 0xFFF (4095) End of Chip-8 RAM
        // |               |
        // |               |
        // |               |
        // |               |
        // |               |
        // | 0x200 to 0xFFF|
        // |     Chip-8    |
        // | Program / Data|
        // |     Space     |
        // |               |
        // |               |
        // |               |
        // +- - - - - - - -+
        // |               |
        // |               | 
        // |               |
        // +---------------+= 0x200 (512) Start of most Chip-8 programs
        // | 0x000 to 0x1FF|
        // | Reserved for  | * Used for storing the font 
        // |  interpreter  |
        // +---------------+= 0x000 (0) Start of Chip-8 RAM
        uint8_t memory[C8_MEMORY_SIZE]{};

        //////// Instruction "Index" ///////
        //Program Counter
        // * Store the currently executing address
        uint16_t pc{};

        //Opcode
        // * Store the current instruction
        uint16_t opcode{};

        //Index Register
        // * Special Register for specific OpCode
        uint16_t I{};

        ////// Stack ///////
        // * Keep track the order of execution
        uint16_t stack[16]{};
        
        //Stack Pointer
        // * Proportional to the number of levels in the stack
        uint8_t sp{};

        ////// Timers ///////
        //Delay Timer
        // * Count down to 0 at 60hz 
        uint8_t delay_timer{};

        //Sound Timer
        // * Counter are in MS
        // * Count down to 0 at 60hz 
        uint8_t sound_timer{};

        ////// Keypad ///////
        uint8_t keypad[C8_KEYPAD_SIZE]{};

        ////// Graphics Display ///////
        // * 64 x 32 pixels
        uint8_t display[C8_DISPLAY_WIDTH * C8_DISPLAY_HEIGHT]{};
    };

    CHIP8EmulatorState create_chip8emulator();

    void reset_state(CHIP8EmulatorState& state);

    void load_rom_from_buffer(CHIP8EmulatorState& state, uint8_t* rom, int size);

    void emulate_cycle(CHIP8EmulatorState& state);

    void destroy_chip8emulator(CHIP8EmulatorState& state);
}