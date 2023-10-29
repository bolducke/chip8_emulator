#include "emulator.h"

namespace chip8 {

    CHIP8EmulatorState create_chip8emulator() {
        CHIP8EmulatorState state{};
        reset_state(state);

        //Load the font char starting from 0x50
        for(unsigned int i = 0; i < C8_FONTSET_SIZE; i+=1) {
            state.memory[C8_FONTSET_START_ADDRESS + i] = C8_FONTSET[i];
        }   

        return state;
    }

    void reset_state(CHIP8EmulatorState& state) {
        memset(state.V, 0, sizeof(state.V));
        memset(&state.memory[C8_START_ADDRESS], 0, (C8_MEMORY_SIZE - C8_START_ADDRESS)*sizeof(state.memory[0]));
        state.pc = C8_START_ADDRESS;
        state.opcode = 0;
        state.I = 0;

        state.sp = 0;
        memset(state.stack, 0, sizeof(state.stack));

        state.delay_timer = 0;
        state.sound_timer = 0;

        memset(state.display, 0, sizeof(state.display));
    }

    void load_rom_from_buffer(CHIP8EmulatorState& state, uint8_t* rom, int size) {
        reset_state(state);
        std::copy(rom, rom + size, state.memory + C8_START_ADDRESS);
    }

    void destroy_chip8emulator(CHIP8EmulatorState& state) {
        // Nothing to do here
    }
    ///////////////////////
    // OPCODE
    ///////////////////////

    // Clear the display
    void OP_00E0(CHIP8EmulatorState& state) {
        memset(state.display, 0, sizeof(state.display));
    }

    // Return from a subroutine
    void OP_00EE(CHIP8EmulatorState& state) {
        state.sp -= 1;
        state.pc = state.stack[state.sp];
    }

    // Jump to address NNN
    void OP_1NNN(CHIP8EmulatorState& state) {
        uint16_t NNN = (state.opcode & 0x0FFFu);
        state.pc = NNN;
    }

    // Call subroutine at NNN
    void OP_2NNN(CHIP8EmulatorState& state) {
        uint16_t NNN = (state.opcode & 0x0FFFu);

        state.stack[state.sp] = state.pc;
        state.sp += 1;
        state.pc = NNN;
    }

    // Skip the following instruction if the value of register VX equals NN
    void OP_3XKK(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t KK = (state.opcode & 0x00FFu);

        if (state.V[X] == KK) {
            state.pc += 2;
        }
    }

    // Skip the following instruction if the value of register VX is not equal to NN
    void OP_4XKK(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t KK = (state.opcode & 0x00FFu);

        if (state.V[X] != KK) {
            state.pc += 2;
        }
    }   

    // Skip the following instruction if the value of register VX is equal to the value of register VY
    void OP_5XY0(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        if (state.V[X] == state.V[Y]) {
            state.pc += 2;
        }
    }

    // Store number KK in register VX
    void OP_6XKK(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t KK = (state.opcode & 0x00FFu);

        state.V[X] = KK;
    }

    void OP_7XKK(CHIP8EmulatorState& state) {
        // Add the value KK to register VX
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t KK = state.opcode & 0x00FFu;

        state.V[X] +=  KK;
    }

    // Store the value of register VY in register VX
    void OP_8XY0(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        state.V[X] = state.V[Y];
    }

    // Set VX to VX OR VY
    void OP_8XY1(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        state.V[X] = state.V[X] | state.V[Y];
    }

    // Set VX to VX AND VY
    void OP_8XY2(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        state.V[X] = state.V[X] & state.V[Y];
    }

    void OP_8XY3(CHIP8EmulatorState& state) {
        // Set VX to VX XOR VY
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        state.V[X] = state.V[X] ^ state.V[Y];
    }

    // Vx = Vx + Vy
    // Set VF to 01 if a carry occurs
    // Set VF to 00 if a carry does not occur
    void OP_8XY4(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        uint16_t sum = state.V[X] + state.V[Y];
        uint8_t carry = sum > 0x00FFu;
        
        state.V[0x0F] = carry;
        state.V[X] = sum;
    }

    // Vx = Vx - Vy. 
    // VF is set to 0 when there's a borrow, and 1 when there is not.
    void OP_8XY5(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        uint16_t sub = state.V[X] - state.V[Y];
        uint8_t noborrow = state.V[X] > state.V[Y];
        
        state.V[0x0F] = noborrow;
        state.V[X] = sub;
    }

    // Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
    void OP_8XY6(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        //uint16_t Y = (state.opcode & 0x00F0u) >> 4;

        uint8_t lsb = (state.V[X] & 0b00000001u);
        state.V[0xF] = lsb;
        state.V[X] = state.V[X] >> 1;    
    }

    // Vx = Vy - Vx. 
    // VF is set to 0 when there's a borrow, and 1 when there is not.
    void OP_8XY7(CHIP8EmulatorState& state) {
        //TODO
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        uint16_t sub = state.V[Y] - state.V[X];
        uint8_t noborrow = state.V[Y] > state.V[X];
        
        state.V[0x0F] = noborrow;
        state.V[X] = sub;
    }

    // Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
    void OP_8XYE(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        //uint16_t Y = (state.opcode & 0x00F0u) >> 4;

        uint8_t msb = (state.V[X] & 0x80u) >> 7;
        state.V[0xF] = msb;
        state.V[X] = state.V[X] << 1;  
    }

    // Skip the following instruction if the value of register VX is not equal to the value of register VY
    void OP_9XY0(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;

        if (state.V[X] != state.V[Y]) {
            state.pc += 2;
        }
    }

    // Store memory address NNN in register I
    void OP_ANNN(CHIP8EmulatorState& state) {
        uint16_t NNN = state.opcode & 0x0FFFu;
        state.I = NNN;
    }

    // Jump to address NNN + V0
    void OP_BNNN(CHIP8EmulatorState& state) {
        uint16_t NNN = state.opcode & 0x0FFFu;
        state.pc = state.V[0] + NNN;
    }

    // Set VX to a random number with a mask of NN
    void OP_CXKK(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t NN = (state.opcode & 0x00FFu);

        state.V[X] = (rand() % 0xFFu) & NN;
    }

    // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. 
    // Each row of 8 pixels is read as bit-coded starting from memory location I;
    // I value does not change after the execution of this instruction.
    // As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that does not happen
    void OP_DXYN(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        uint8_t Y = (state.opcode & 0x00F0u) >> 4;
        uint8_t N = (state.opcode & 0x000Fu);

        uint8_t Vx = state.V[X];
        uint8_t Vy = state.V[Y];

        state.V[0xF] = 0;

        for(int irow = 0; irow < N; irow++) {
            uint8_t sprite = state.memory[state.I + irow];

            for(int icol = 0; icol < 8; icol++) {
                uint8_t sprite_pixel = ((sprite >> (7-icol) ) & 0b1);

                unsigned int x_pos = (Vx+icol) % C8_DISPLAY_WIDTH;
                unsigned int y_pos = (Vy+irow) % C8_DISPLAY_HEIGHT;

                uint8_t& curr_display = state.display[x_pos + C8_DISPLAY_WIDTH*y_pos];

                state.V[0xF] =  state.V[0xF] || (sprite_pixel & curr_display);
                curr_display = curr_display ^ sprite_pixel;
            }
        }
    }

    // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
    void OP_EX9E(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        if (state.keypad[state.V[X]]) {
            state.pc += 2;
        }
    }

    // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
    void OP_EXA1(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        if (!state.keypad[state.V[X]]) {
            state.pc += 2;
        }
    }

    // Store the current value of the delay timer in register VX
    void OP_FX07(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        state.V[X] = state.delay_timer;
    }

    // Wait for a keypress and store the result in register VX
    void OP_FX0A(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        bool pressed = false;
        for(uint8_t i = 0; i <= 0x0Fu; i++) {
            if (state.keypad[i]) {
                state.V[X] = i;
                pressed = true;
            }
        }
        if (!pressed){
            state.pc -= 2;
        }
    }

    // Set the delay timer to the value of register VX
    void OP_FX15(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;
        state.delay_timer = state.V[X];
    }

    // Set the sound timer to the value of register VX
    void OP_FX18(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        state.sound_timer = state.V[X];
    }

    // Add the value stored in register VX to register I
    void OP_FX1E(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        state.I = state.I + state.V[X];
    }

    // Sets I to the location of the sprite for the character in VX.
    void OP_FX29(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        state.I = C8_FONTSET_START_ADDRESS + C8_FONT_SIZE * state.V[X];
    }

    // Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I+1, and I+2
    void OP_FX33(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8;

        uint8_t rem = state.V[X];
        
        for (int i = 2; i >= 0; i -= 1) {
            state.memory[state.I+i] = rem % 10;
            rem = rem / 10;
        }
    }

    // Stores from V0 to VX (including VX) in memory, starting at address I. 
    // The offset from I is increased by 1 for each value written, but I itself is left unmodified
    void OP_FX55(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u ) >> 8; 
        memcpy(&state.memory[state.I], state.V, sizeof(uint8_t) * (X+1));
    }

    // Fills from V0 to VX (including VX) in memory, starting at address I. 
    // The offset from I is increased by 1 for each value written, but I itself is left unmodified
    void OP_FX65(CHIP8EmulatorState& state) {
        uint8_t X = (state.opcode & 0x0F00u) >> 8; 
        memcpy(state.V, &state.memory[state.I], sizeof(uint8_t) * (X+1));
    }

    void OP_NULL(CHIP8EmulatorState& state) {
        fmt::println("Wrong OPCODE Call: {}", state.opcode);
    }

    typedef void (*Chip8Func)(CHIP8EmulatorState& state);
    void TB_0TTT(CHIP8EmulatorState& state) {
        const static Chip8Func table0[0xE + 1] = {&OP_00E0, &OP_NULL, &OP_NULL, &OP_NULL, // 0x00-0x03
                                                  &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x04-0x07
                                                  &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x08-0x0B
                                                  &OP_NULL, &OP_NULL, &OP_00EE};          // 0x0C-0x0E

        uint8_t inst_type = (state.opcode & 0x000F);
        (*table0[inst_type])(state);
    }

    void TB_8TTT(CHIP8EmulatorState& state) {
        const static Chip8Func table8[0xE + 1] = {&OP_8XY0, &OP_8XY1, &OP_8XY2, &OP_8XY3, // 0x00-0x03
                                                  &OP_8XY4, &OP_8XY5, &OP_8XY6, &OP_8XY7, // 0x04-0x07
                                                  &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x08-0x0B
                                                  &OP_NULL, &OP_NULL, &OP_8XYE};          // 0x0C-0x0E

        uint8_t inst_type = (state.opcode & 0x000F);
        (*table8[inst_type])(state);
    }
    
    void TB_ETTT(CHIP8EmulatorState& state) {
        const static Chip8Func tableE[0xE + 1] = {&OP_NULL, &OP_EXA1, &OP_NULL, &OP_NULL,
                                                  &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL,
                                                  &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL,
                                                  &OP_NULL, &OP_NULL, &OP_EX9E};
        
        uint8_t inst_type = (state.opcode & 0x000F);
        (*tableE[inst_type])(state);
    }

    void TB_FTTT(CHIP8EmulatorState& state) {
        const static Chip8Func tableF[0x65 + 1] = { &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x00-0x03
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_FX07, // 0x04-0x07
                                                    &OP_NULL, &OP_NULL, &OP_FX0A, &OP_NULL, // 0x08-0x0B
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x0C-0x0F
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x10-0x13 
                                                    &OP_NULL, &OP_FX15, &OP_NULL, &OP_NULL, // 0x14-0x17
                                                    &OP_FX18, &OP_NULL, &OP_NULL, &OP_NULL, // 0x18-0x1B
                                                    &OP_NULL, &OP_NULL, &OP_FX1E, &OP_NULL, // 0x1C-0x1F
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x20-0x23
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x24-0x27
                                                    &OP_NULL, &OP_FX29, &OP_NULL, &OP_NULL, // 0x28-0x2B
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x2C-0x2F
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_FX33, // 0x30-0x33
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x34-0x37
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x38-0x3B
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x3C-0x3F
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x40-0x43
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x44-0x47
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x48-0x4B
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x4C-0x4F
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x50-0x53
                                                    &OP_NULL, &OP_FX55, &OP_NULL, &OP_NULL, // 0x54-0x57
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x58-0x5B
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x5C-0x5F
                                                    &OP_NULL, &OP_NULL, &OP_NULL, &OP_NULL, // 0x60-0x63
                                                    &OP_NULL, &OP_FX65};                    // 0x64-0x65

        uint8_t inst_type = (state.opcode & 0x00FF);
        (*tableF[inst_type])(state);
    }

    void emulate_cycle(CHIP8EmulatorState& state) {
        const static Chip8Func table[0xF + 1] = { &TB_0TTT, &OP_1NNN, &OP_2NNN, &OP_3XKK,
                                                  &OP_4XKK, &OP_5XY0, &OP_6XKK, &OP_7XKK,
                                                  &TB_8TTT, &OP_9XY0, &OP_ANNN, &OP_BNNN,
                                                  &OP_CXKK, &OP_DXYN, &TB_ETTT, &TB_FTTT};
        
        // Fetch the OPCode
        state.opcode = (state.memory[state.pc] << 8) | state.memory[state.pc+1];

        // Increase the program counter
        state.pc += 2;

        // Call the function
        uint8_t inst_type = (state.opcode & 0xF000u) >> 12;
        (*table[inst_type])(state);

        if (state.delay_timer > 0) {
            state.delay_timer -= 1;
        }

        if (state.sound_timer > 0) {
            state.sound_timer -= 1;
        }
    }
}