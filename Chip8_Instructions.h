#ifndef CHIP8_INSTRUCTIONS_H
#define CHIP8_INSTRUCTIONS_H 1

#include "Chip8_CPU.h"

// OP-CODE Guide from https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set

static inline void return_subroutine(Chip8_CPU *cpu)
{
    ASSERT((cpu->call_stack.n_elements > 0), "[ERROR] Tried to pop empty stack at PC: 0x%04x\n", cpu->program_counter);
    cpu->program_counter = cpu->call_stack.stack[--cpu->call_stack.n_elements];
}

static inline void jump_subroutine(Chip8_CPU *cpu, WORD address)
{
    ASSERT((cpu->call_stack.n_elements < CHIP8_STACK_SIZE - 1), "[ERROR] Tried to push full stack at PC: 0x%04x\n", cpu->program_counter);
    cpu->call_stack.stack[cpu->call_stack.n_elements++] = cpu->program_counter;
    cpu->program_counter = address;
}

static inline void reset_stack(Stack *stack)
{
    memset(stack->stack, 0, sizeof(stack->stack));
    stack->n_elements = 0;
}

static inline BYTE get_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    return cpu->game_registers[vX];
}

static inline void set_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    cpu->game_registers[vX] = (instruction & 0xFF);
}

static inline void set_vx_value(Chip8_CPU *cpu, WORD instruction, BYTE value)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    cpu->game_registers[vX] = value;
}

static inline BYTE get_vy(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vY = (instruction & 0x00F0) >> 4;
    return cpu->game_registers[vY];
}

static inline void dump_vxy(Chip8_CPU *cpu, BYTE min, BYTE max)
{
    for (BYTE x = min; x <= max; x++)
    {
        cpu->game_memory[cpu->i_register + x] = cpu->game_registers[x];
    }

    if (cpu->target != SCHIPC)
        cpu->i_register += max + 1;
}

static inline void load_vxy(Chip8_CPU *cpu, BYTE min, BYTE max)
{
    for (BYTE x = min; x <= max; x++)
    {
        cpu->game_registers[x] = cpu->game_memory[cpu->i_register + x];
    }

    if (cpu->target != SCHIPC)
        cpu->i_register += max + 1;
}

static inline void wait_key(Chip8_CPU *cpu, WORD instruction)
{

    if (cpu->pressed_key != 16 && !cpu->keys[cpu->pressed_key])
    {
        set_vx_value(cpu, instruction, cpu->pressed_key);
        cpu->pressed_key = 16;
        return;
    }

    for (BYTE i = 0; i < 16; i++)
    {
        if (cpu->keys[i])
        {
            cpu->pressed_key = i;
            break;
        }
    }
    cpu->program_counter -= 2;
}

static inline void draw_sprite_lores_clipping(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = (get_vx(cpu, instruction) & 63) * 2;
    BYTE coordY = (get_vy(cpu, instruction) & 31) * 2;
    BYTE height = (instruction & 0xF);
    BYTE row;

    cpu->game_registers[0xF] = 0;

    for (BYTE yline = 0; yline < height && (coordY + (yline * 2)) < CHIP8_SCREEN_HEIGHT; yline++)
    {
        row = cpu->game_memory[cpu->i_register + yline];
        for (BYTE xpixel = 0; xpixel < 8 && (coordX + (xpixel * 2)) < CHIP8_SCREEN_WIDTH; xpixel++)
        {
            if (row & (0x80 >> xpixel))
            {
                WORD pos1 = coordX + (xpixel * 2) + ((coordY + (yline * 2)) * CHIP8_SCREEN_WIDTH);

                if (cpu->screen_plane1[pos1] == 1)
                {
                    cpu->game_registers[0xF] = 1;
                }

                cpu->screen_plane1[pos1] ^= 1;
                cpu->screen_plane1[pos1 + 1] ^= 1;
                cpu->screen_plane1[pos1 + CHIP8_SCREEN_WIDTH] ^= 1;
                cpu->screen_plane1[pos1 + CHIP8_SCREEN_WIDTH + 1] ^= 1;
            }
        }
    }
    cpu->dirty_flag = 1;
}

static inline void draw_sprite_lores_warping(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = (get_vx(cpu, instruction) & 63) * 2;
    BYTE coordY = (get_vy(cpu, instruction) & 31) * 2;
    BYTE height = (instruction & 0xF);
    BYTE both_planes = 0;
    WORD start_addr;
    BYTE row;

    cpu->game_registers[0xF] = 0;

    if (cpu->bitplane & 0x1)
    {
        for (BYTE yline = 0; yline < height; yline++)
        {
            row = cpu->game_memory[cpu->i_register + yline];
            for (BYTE xpixel = 0; xpixel < 8; xpixel++)
            {
                if (row & (0x80 >> xpixel))
                {
                    WORD pos1 = ((coordX + (xpixel * 2)) % CHIP8_SCREEN_WIDTH) + (((coordY + (yline * 2)) % CHIP8_SCREEN_HEIGHT) * CHIP8_SCREEN_WIDTH);

                    if (cpu->screen_plane1[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = 1;
                    }

                    cpu->screen_plane1[pos1] ^= 1;
                    cpu->screen_plane1[pos1 + 1] ^= 1;
                    cpu->screen_plane1[pos1 + CHIP8_SCREEN_WIDTH] ^= 1;
                    cpu->screen_plane1[pos1 + CHIP8_SCREEN_WIDTH + 1] ^= 1;
                }
            }
        }

        both_planes = 1;
    }

    start_addr = (both_planes) ? cpu->i_register + height : cpu->i_register;

    if (cpu->bitplane & 0x2)
    {
        for (BYTE yline = 0; yline < height; yline++)
        {
            row = cpu->game_memory[start_addr + yline];
            for (BYTE xpixel = 0; xpixel < 8; xpixel++)
            {
                if (row & (0x80 >> xpixel))
                {
                    WORD pos1 = ((coordX + (xpixel * 2)) % CHIP8_SCREEN_WIDTH) + (((coordY + (yline * 2)) % CHIP8_SCREEN_HEIGHT) * CHIP8_SCREEN_WIDTH);

                    if (cpu->screen_plane2[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = 1;
                    }

                    cpu->screen_plane2[pos1] ^= 1;
                    cpu->screen_plane2[pos1 + 1] ^= 1;
                    cpu->screen_plane2[pos1 + CHIP8_SCREEN_WIDTH] ^= 1;
                    cpu->screen_plane2[pos1 + CHIP8_SCREEN_WIDTH + 1] ^= 1;
                }
            }
        }
    }

    cpu->dirty_flag = 1;
}

// TODO: Warping Version
static inline void draw_sprite_big(Chip8_CPU *cpu, BYTE coordX, BYTE coordY)
{
    WORD row;
    BYTE both_planes = 0;
    WORD start_addr;
    cpu->game_registers[0xF] = 0;

    if (cpu->bitplane & 0x1)
    {
        for (BYTE yline = 0; yline < 16 && (coordY + yline) < CHIP8_SCREEN_HEIGHT; yline++)
        {
            row = (cpu->game_memory[cpu->i_register + yline * 2] << 8) | cpu->game_memory[cpu->i_register + yline * 2 + 1];
            for (BYTE xpixel = 0; xpixel < 16 && (coordX + xpixel) < CHIP8_SCREEN_WIDTH; xpixel++)
            {
                if (row & (0x8000 >> xpixel))
                {
                    WORD pos1 = coordX + xpixel + ((coordY + yline) * CHIP8_SCREEN_WIDTH);

                    if (cpu->screen_plane1[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = 1;
                    }

                    cpu->screen_plane1[pos1] ^= 1;
                }
            }
        }

        both_planes = 1;
    }

    start_addr = (both_planes) ? cpu->i_register + 16 * 2 : cpu->i_register;

    if (cpu->bitplane & 0x2)
    {
        for (BYTE yline = 0; yline < 16 && (coordY + yline) < CHIP8_SCREEN_HEIGHT; yline++)
        {
            row = (cpu->game_memory[start_addr + yline * 2] << 8) | cpu->game_memory[start_addr + yline * 2 + 1];
            for (BYTE xpixel = 0; xpixel < 16 && (coordX + xpixel) < CHIP8_SCREEN_WIDTH; xpixel++)
            {
                if (row & (0x8000 >> xpixel))
                {
                    WORD pos1 = coordX + xpixel + ((coordY + yline) * CHIP8_SCREEN_WIDTH);

                    if (cpu->screen_plane2[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = 1;
                    }

                    cpu->screen_plane2[pos1] ^= 1;
                }
            }
        }
    }

    cpu->dirty_flag = 1;
}

static inline void draw_sprite_hires_clipping(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = get_vx(cpu, instruction) & 127;
    BYTE coordY = get_vy(cpu, instruction) & 63;
    BYTE height = (instruction & 0xF);
    BYTE row;

    if (height == 0)
    {
        draw_sprite_big(cpu, coordX, coordY);
        return;
    }

    cpu->game_registers[0xF] = 0;

    for (BYTE yline = 0; yline < height && (coordY + yline) < CHIP8_SCREEN_HEIGHT; yline++)
    {
        row = cpu->game_memory[cpu->i_register + yline];
        for (BYTE xpixel = 0; xpixel < 8 && (coordX + xpixel) < CHIP8_SCREEN_WIDTH; xpixel++)
        {
            if ((row & (0x80 >> xpixel)) != 0)
            {
                WORD pos1 = coordX + xpixel + ((coordY + yline) * CHIP8_SCREEN_WIDTH);

                if (cpu->screen_plane1[pos1] == 1)
                {
                    cpu->game_registers[0xF] = 1;
                }

                cpu->screen_plane1[pos1] ^= 1;
            }
        }
    }
    cpu->dirty_flag = 1;
}

static inline void draw_sprite_hires_warping(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = get_vx(cpu, instruction) & 127;
    BYTE coordY = get_vy(cpu, instruction) & 63;
    BYTE height = (instruction & 0xF);
    BYTE both_planes = 0;
    WORD start_addr;
    BYTE row;

    if (height == 0)
    {
        draw_sprite_big(cpu, coordX, coordY);
        return;
    }

    cpu->game_registers[0xF] = 0;

    if (cpu->bitplane & 0x1)
    {
        for (BYTE yline = 0; yline < height; yline++)
        {
            row = cpu->game_memory[cpu->i_register + yline];
            for (BYTE xpixel = 0; xpixel < 8; xpixel++)
            {
                if ((row & (0x80 >> xpixel)) != 0)
                {
                    WORD pos1 = ((coordX + xpixel) % CHIP8_SCREEN_WIDTH) + (((coordY + yline) % CHIP8_SCREEN_HEIGHT) * CHIP8_SCREEN_WIDTH);

                    if (cpu->screen_plane1[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = 1;
                    }

                    cpu->screen_plane1[pos1] ^= 1;
                }
            }
        }
        both_planes = 1;
    }

    start_addr = (both_planes) ? cpu->i_register + height : cpu->i_register;

    if(cpu->bitplane & 0x2)
    {
        for (BYTE yline = 0; yline < height; yline++)
        {
            row = cpu->game_memory[start_addr + yline];
            for (BYTE xpixel = 0; xpixel < 8; xpixel++)
            {
                if ((row & (0x80 >> xpixel)) != 0)
                {
                    WORD pos1 = ((coordX + xpixel) % CHIP8_SCREEN_WIDTH) + (((coordY + yline) % CHIP8_SCREEN_HEIGHT) * CHIP8_SCREEN_WIDTH);

                    if (cpu->screen_plane2[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = 1;
                    }

                    cpu->screen_plane2[pos1] ^= 1;
                }
            }
        }
    }

    cpu->dirty_flag = 1;
}

/* 00CN: Scroll screen content down N pixel.
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Only selected bit planes are scrolled.
*/
static inline void OP_00CN(Chip8_CPU *cpu, WORD inst)
{
    BYTE amount = inst & 0x00F;

    if (cpu->mode == LORES)
        amount *= 2;

    if (cpu->target == CHIP8)
        ASSERT((0), "[ERROR] SUPER CHIP/XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: CHIP-8\n", inst, cpu->program_counter);

    if (cpu->bitplane & 1)
    {
        memmove(cpu->screen_plane1 + (amount * CHIP8_SCREEN_WIDTH), cpu->screen_plane1, (CHIP8_SCREEN_HEIGHT - amount) * CHIP8_SCREEN_WIDTH);
        memset(cpu->screen_plane1, 0, amount * CHIP8_SCREEN_WIDTH);
    }
    if (cpu->bitplane & 2)
    {
        memmove(cpu->screen_plane2 + (amount * CHIP8_SCREEN_WIDTH), cpu->screen_plane2, (CHIP8_SCREEN_HEIGHT - amount) * CHIP8_SCREEN_WIDTH);
        memset(cpu->screen_plane2, 0, amount * CHIP8_SCREEN_WIDTH);
    }

    cpu->dirty_flag = 1;
}

/* O0DN: Scroll screen content up N pixel.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Only selected bit planes are scrolled.
*/
static inline void OP_00DN(Chip8_CPU *cpu, WORD inst)
{
    BYTE amount = inst & 0x00F;

    if (cpu->target != XOCHIP)
        ASSERT((0), "[ERROR] XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: %s\n", inst, cpu->program_counter, (cpu->target == 0) ? "Chip-8" : "SUPER CHIP");

    if (cpu->mode == LORES)
        amount *= 2;

    if (cpu->bitplane & 1)
    {
        memmove(cpu->screen_plane1, cpu->screen_plane1 + (amount * CHIP8_SCREEN_WIDTH), (CHIP8_SCREEN_HEIGHT - amount) * CHIP8_SCREEN_WIDTH);
        memset(cpu->screen_plane1 + (amount * CHIP8_SCREEN_WIDTH), 0, amount * CHIP8_SCREEN_WIDTH);
    }
    if (cpu->bitplane & 2)
    {
        memmove(cpu->screen_plane2, cpu->screen_plane2 + (amount * CHIP8_SCREEN_WIDTH), (CHIP8_SCREEN_HEIGHT - amount) * CHIP8_SCREEN_WIDTH);
        memset(cpu->screen_plane2 + (amount * CHIP8_SCREEN_WIDTH), 0, amount * CHIP8_SCREEN_WIDTH);
    }

    cpu->dirty_flag = 1;
}

/* 00E0: Clears the screen.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Only selected bit planes are cleared.
*/
static inline void OP_00E0(Chip8_CPU *cpu)
{
    if (cpu->bitplane & 1)
        memset(cpu->screen_plane1, 0, sizeof(cpu->screen_plane1));
    if (cpu->bitplane & 2)
        memset(cpu->screen_plane2, 0, sizeof(cpu->screen_plane2));
}

// 00EE: Return from a subroutine.
static inline void OP_00EE(Chip8_CPU *cpu)
{
    return_subroutine(cpu);
}

/* O0FB: Scroll screen content right 4 pixel.
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Only selected bit planes are scrolled.
*/
static inline void OP_00FB(Chip8_CPU *cpu, WORD inst)
{
    BYTE amount = 4;

    if (cpu->target == CHIP8)
        ASSERT((0), "[ERROR] SUPER CHIP/XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: CHIP-8\n", inst, cpu->program_counter);

    if (cpu->mode == LORES)
        amount = 8;

    if (cpu->bitplane & 1)
    {
        for (BYTE y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
        {
            BYTE *row = cpu->screen_plane1 + (CHIP8_SCREEN_WIDTH * y);
            memmove(row + amount, row, CHIP8_SCREEN_WIDTH - amount);
            memset(row, 0, amount);
        }
    }
    if (cpu->bitplane & 2)
    {
        for (BYTE y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
        {
            BYTE *row = cpu->screen_plane2 + (CHIP8_SCREEN_WIDTH * y);
            memmove(row + amount, row, CHIP8_SCREEN_WIDTH - amount);
            memset(row, 0, amount);
        }
    }
    cpu->dirty_flag = 1;
}

/* O0FC: Scroll screen content left 4 pixels.
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Only selected bit planes are scrolled.
*/
static inline void OP_00FC(Chip8_CPU *cpu, WORD inst)
{
    BYTE amount = 4;

    if (cpu->target == CHIP8)
        ASSERT((0), "[ERROR] SUPER CHIP/XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: CHIP-8\n", inst, cpu->program_counter);

    if (cpu->mode == LORES)
        amount = 8;

    if (cpu->bitplane & 1)
    {
        for (BYTE y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
        {
            BYTE *row = cpu->screen_plane1 + (CHIP8_SCREEN_WIDTH * y);
            memmove(row, row + amount, CHIP8_SCREEN_WIDTH - amount);
            memset(row + amount, 0, amount);
        }
    }
    if (cpu->bitplane & 2)
    {
        for (BYTE y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
        {
            BYTE *row = cpu->screen_plane2 + (CHIP8_SCREEN_WIDTH * y);
            memmove(row, row + amount, CHIP8_SCREEN_WIDTH - amount);
            memset(row + amount, 0, amount);
        }
    }
    cpu->dirty_flag = 1;
}

/* O0FD: Exit interpreter.
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_00FD(Chip8_CPU *cpu, WORD inst)
{
    UNUSED(cpu);
    UNUSED(inst);
    exit(EXIT_SUCCESS); // TODO: otra manera de hacer esto?
}

/* O0FE: Switch to lores mode (64x32).
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_00FE(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target == CHIP8)
        ASSERT((0), "[ERROR] SUPER CHIP/XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: CHIP-8\n", inst, cpu->program_counter);

    cpu->mode = LORES;
    memset(cpu->screen_plane1, 0, sizeof(cpu->screen_plane1));
    memset(cpu->screen_plane2, 0, sizeof(cpu->screen_plane2));
}

/* O0FF: Switch to hires mode (128x64).
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_00FF(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target == CHIP8)
        ASSERT((0), "[ERROR] SUPER CHIP/XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: CHIP-8\n", inst, cpu->program_counter);

    cpu->mode = HIRES;

    memset(cpu->screen_plane1, 0, sizeof(cpu->screen_plane1));
    memset(cpu->screen_plane2, 0, sizeof(cpu->screen_plane2));
}

// 1NNN: Jump to address `NNN`.
static inline void OP_1NNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->program_counter = (inst & 0x0fff);
}

// 2NNN: Execute subroutine starting at address `NNN`.
static inline void OP_2NNN(Chip8_CPU *cpu, WORD inst)
{
    jump_subroutine(cpu, (inst & 0x0fff));
}

/* 3XNN: Skip the following instruction if the value of register ``VX equals `NN`.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Skips 4 bytes if next instruction is F000.
*/
static inline void OP_3XNN(Chip8_CPU *cpu, WORD inst)
{

    BYTE value = get_vx(cpu, inst);
    BYTE NN = inst & 0xFF;
    if (value == NN)
    {
        cpu->program_counter += 2;

        if (cpu->target == XOCHIP)
        {
            WORD next_inst = (cpu->game_memory[cpu->program_counter - 2] << 8) |
                             cpu->game_memory[cpu->program_counter - 1];

            if (next_inst == 0xF000)
            {
                cpu->program_counter += 2;
            }
        }
    }
}

/* 4XNN: Skip the following instruction if the value of register `VX` is not equal to `NN`.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Skips 4 bytes if next instruction is F000.
*/
static inline void OP_4XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vx(cpu, inst);
    BYTE NN = inst & 0xFF;

    if (value != NN)
    {
        cpu->program_counter += 2;

        if (cpu->target == XOCHIP)
        {
            WORD next_inst = (cpu->game_memory[cpu->program_counter - 2] << 8) |
                             cpu->game_memory[cpu->program_counter - 1];

            if (next_inst == 0xF000)
            {
                cpu->program_counter += 2;
            }
        }
    }
}

/* 5XY0: Skip the following instruction if the value of register `VX` is equal to the value of register `VY`.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Skips 4 bytes if next instruction is F000.
*/
static inline void OP_5XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);

    if (vx == vy)
    {
        cpu->program_counter += 2;

        if (cpu->target == XOCHIP)
        {
            WORD next_inst = (cpu->game_memory[cpu->program_counter - 2] << 8) |
                             cpu->game_memory[cpu->program_counter - 1];

            if (next_inst == 0xF000)
            {
                cpu->program_counter += 2;
            }
        }
    }
}

/* 5XY2: Write registers vX to vY to memory pointed to by I. I stays the same.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_5XY2(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target != XOCHIP)
        ASSERT((0), "[ERROR] XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: %s\n", inst, cpu->program_counter, (cpu->target == 0) ? "Chip-8" : "SUPER CHIP");

    BYTE min = get_vx(cpu, inst);
    BYTE max = get_vy(cpu, inst);

    dump_vxy(cpu, min, max);
}

/* 5XY3: Load registers vX to vY from memory pointed to by I. I stays the same.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_5XY3(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target != XOCHIP)
        ASSERT((0), "[ERROR] XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: %s\n", inst, cpu->program_counter, (cpu->target == 0) ? "Chip-8" : "SUPER CHIP");

    BYTE min = get_vx(cpu, inst);
    BYTE max = get_vy(cpu, inst);

    load_vxy(cpu, min, max);
}

// 6XNN: Store number NN in register VX.
static inline void OP_6XNN(Chip8_CPU *cpu, WORD inst)
{
    set_vx(cpu, inst);
}

// 7XNN: Add the value NN to register VX.
static inline void OP_7XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE value = (inst & 0x00FF);
    set_vx_value(cpu, inst, (vx + value));
}

// 8XY0: Store the value of register VY in register VX.
static inline void OP_8XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vy(cpu, inst);
    set_vx_value(cpu, inst, value);
}

/* 8XY1: Set VX to VX OR VY.
    - CHIP 8: Resets VF register.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_8XY1(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx | vy));
    if (cpu->target == CHIP8)
        cpu->game_registers[0xF] = 0;
}

/* 8XY2: Set VX to VX AND VY.
    - CHIP 8: Resets VF register.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_8XY2(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx & vy));
    if (cpu->target == CHIP8)
        cpu->game_registers[0xF] = 0;
}

/* 8XY3: Set VX to VX XOR VY.
    - CHIP 8: Resets VF register.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_8XY3(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx ^ vy));
    if (cpu->target == CHIP8)
        cpu->game_registers[0xF] = 0;
}

// 8XY4: Add the value of register VY to register VX. Set VF to 01 if a carry occurs. Set VF to 00 if a carry does not occur.
static inline void OP_8XY4(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx + vy));
    cpu->game_registers[15] = 0;
    if (vx + vy > 255)
        cpu->game_registers[0xF] = 1;
}

// 8XY5: Subtract the value of register VY from register VX. Set VF to 00 if a borrow occurs. Set VF to 01 if a borrow does not occur.
static inline void OP_8XY5(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx - vy));
    cpu->game_registers[15] = 1;
    if (vy > vx)
        cpu->game_registers[0xF] = 0;
}

/* 8XY6: Store the value of register VY shifted right one bit in register VX. Set register VF to the least significant bit prior to the shift. VY is unchanged.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Shifts vX and ignores vY.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_8XY6(Chip8_CPU *cpu, WORD inst)
{
    BYTE reg;
    if (cpu->target == SCHIPC)
        reg = get_vx(cpu, inst);
    else
        reg = get_vy(cpu, inst);

    set_vx_value(cpu, inst, (reg >> 1));
    cpu->game_registers[0xF] = (reg & 0x1);
}

// 8XY7: Set register VX to the value of VY minus VX. Set VF to 00 if a borrow occurs. Set VF to 01 if a borrow does not occur.
static inline void OP_8XY7(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy - vx));
    cpu->game_registers[15] = 1;
    if (vx > vy)
        cpu->game_registers[0xF] = 0;
}

/* 8XYE: Store the value of register VY shifted left one bit in register VX. Set register VF to the most significant bit prior to the shift. VY is unchanged.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Shifts vX and ignores vY.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_8XYE(Chip8_CPU *cpu, WORD inst)
{
    BYTE reg;
    if (cpu->target == SCHIPC)
        reg = get_vx(cpu, inst);
    else
        reg = get_vy(cpu, inst);

    set_vx_value(cpu, inst, (reg << 1));
    cpu->game_registers[0xF] = (reg & 0x80) >> 7;
}

// 9XY0: Skip the following instruction if the value of register VX is not equal to the value of register VY.
static inline void OP_9XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    if (vx != vy)
        cpu->program_counter += 2;
}

// ANNN: Store memory address NNN in register I.
static inline void OP_ANNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->i_register = (inst & 0x0FFF);
}

/* BNNN: Jump to address NNN + V0
    - CHIP 8: Normal behaviour.
    - SCHIPC: Doesn't use `V0`, but `VX` instead. X is the highest nibble of `NNN`.
    - XO-CHIP: Doesn't use `V0`, but `VX` instead. X is the highest nibble of `NNN`.
*/
static inline void OP_BNNN(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target == SCHIPC)
        cpu->program_counter = get_vx(cpu, inst) + (inst & 0x0FFF);
    else
        cpu->program_counter = cpu->game_registers[0x0] + (inst & 0x0FFF);
}

// CXNN: Set VX to a random number with a mask of NN.
static inline void OP_CXNN(Chip8_CPU *cpu, WORD inst)
{
    set_vx_value(cpu, inst, (rand() & (inst & 0xFF)));
}

/*DXYN: Draw a 8xN sprite at position VX, VY with N bytes of sprite data starting at the address stored in I. Set VF to 01 if any set pixels are changed to unset, and 00 otherwise.
    - CHIP 8: Only `lores` display. Clips sprites.
    - SCHIPC: `lores` & `hires` display.
    - XO-CHIP: lores` & `hires` display. Wraps all sprites. Only draws on selected bitplane.
*/
static inline void OP_DXYN(Chip8_CPU *cpu, WORD inst)
{

    // TODO:Cambiar esto a puntero a funcion almacenado en Chip8_CPU.
    switch (cpu->target)
    {
    case CHIP8:
        draw_sprite_lores_clipping(cpu, inst);
        break;
    case SCHIPC:
        (cpu->mode == LORES) ? draw_sprite_lores_clipping(cpu, inst) : draw_sprite_hires_clipping(cpu, inst);
        break;
    case XOCHIP:
        (cpu->mode == LORES) ? draw_sprite_lores_warping(cpu, inst) : draw_sprite_hires_warping(cpu, inst);
        break;
    }
}

/* EX9E: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Skips 4 bytes if next instruction is F000.
*/
static inline void OP_EX9E(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);

    if (cpu->keys[vx])
    {
        cpu->program_counter += 2;

        if (cpu->target == XOCHIP)
        {
            WORD next_inst = (cpu->game_memory[cpu->program_counter - 2] << 8) |
                             cpu->game_memory[cpu->program_counter - 1];

            if (next_inst == 0xF000)
            {
                cpu->program_counter += 2;
            }
        }
    }
}

/* EX9E: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed.
    - CHIP 8: Normal behaviour.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Skips 4 bytes if next instruction is F000.
*/
static inline void OP_EXA1(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);

    if (!cpu->keys[vx])
    {
        cpu->program_counter += 2;

        if (cpu->target == XOCHIP)
        {
            WORD next_inst = (cpu->game_memory[cpu->program_counter - 2] << 8) |
                             cpu->game_memory[cpu->program_counter - 1];

            if (next_inst == 0xF000)
            {
                cpu->program_counter += 2;
            }
        }
    }
}

/* F000: Assign next 16 bit word to I, and set PC behind it, this is a four byte instruction.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_F000(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target != XOCHIP)
        ASSERT((0), "[ERROR] XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: %s\n", inst, cpu->program_counter, (cpu->target == 0) ? "Chip-8" : "SUPER CHIP");

    cpu->i_register = (cpu->game_memory[cpu->program_counter] << 8) | cpu->game_memory[cpu->program_counter + 1];

    cpu->program_counter += 2;
}

/* FN01: Select bit planes to draw on when drawing with DXY0/DXYN.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_FN01(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target != XOCHIP)
        ASSERT((0), "[ERROR] XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: %s\n", inst, cpu->program_counter, (cpu->target == 0) ? "Chip-8" : "SUPER CHIP");

    BYTE plane = (inst & 0x0F00) >> 8;

    if (plane < 1 || plane > 3)
    {
        ASSERT((0), "[ERROR] Invalid bitplane: %d. \"0x%04x\" at PC: 0x%04x\n", plane, inst, cpu->program_counter);
    }

    cpu->bitplane = plane;
}

/* F002: Load 16 bytes audio pattern pointed to by I into audio pattern buffer.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_F002(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter); // TODO
}

// FX07: Store the current value of the delay timer in register VX.
static inline void OP_FX07(Chip8_CPU *cpu, WORD inst)
{
    set_vx_value(cpu, inst, cpu->delay_timer);
}

// FX0A: Wait for a keypress and store the result in register VX.
static inline void OP_FX0A(Chip8_CPU *cpu, WORD inst)
{
    wait_key(cpu, inst);
}

// FX15: Set the delay timer to the value of register VX.
static inline void OP_FX15(Chip8_CPU *cpu, WORD inst)
{
    cpu->delay_timer = get_vx(cpu, inst);
}

// FX18: Set the sound timer to the value of register VX.
static inline void OP_FX18(Chip8_CPU *cpu, WORD inst)
{
    cpu->sound_timer = get_vx(cpu, inst);
}

// FX1E: Add the value stored in register VX to register I.
static inline void OP_FX1E(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->i_register += vx;
}

// FX29: Set I to the memory address of the sprite data corresponding to the 5-lines high hexadecimal digit stored in register VX.
static inline void OP_FX29(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->i_register = SMALL_FONT_ADDRESS + (5 * vx); // Max 0x9f
}

/* FX30: Set I to the memory address of the sprite data corresponding to the 10-lines high hexadecimal digit stored in register VX.
    - CHIP 8: Unimplemented.
    - SCHIPC: Normal behaviour.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_FX30(Chip8_CPU *cpu, WORD inst)
{
    if (cpu->target == CHIP8)
        ASSERT((0), "[ERROR] SUPER CHIP/XO-CHIP instruction \"0x%04x\" at PC: 0x%04x. Current target: CHIP-8\n", inst, cpu->program_counter);

    BYTE vx = get_vx(cpu, inst);

    cpu->i_register = BIG_FONT_ADDRESS + (10 * vx);
}

// FX33: Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2.
static inline void OP_FX33(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->game_memory[cpu->i_register] = vx / 100;
    cpu->game_memory[cpu->i_register + 1] = (vx / 10) % 10;
    cpu->game_memory[cpu->i_register + 2] = vx % 10;
}

/* FX3A: Set audio pitch for a audio pattern playback rate of 4000*2^((vX-64)/48)Hz.
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_FX3A(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
}

/* FX55: Store the values of registers V0 to VX inclusive in memory starting at address I
    - CHIP 8: I is set to I + X + 1 after operation
    - SCHIPC: I stays the same.
    - XO-CHIP: I stays the same.
*/
static inline void OP_FX55(Chip8_CPU *cpu, WORD inst)
{
    BYTE max = (inst & 0x0F00) >> 8;
    dump_vxy(cpu, 0, max);
}

/* FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I
    - CHIP 8: I is set to I + X + 1 after operation
    - SCHIPC: I stays the same.
    - XO-CHIP: I stays the same.
*/
static inline void OP_FX65(Chip8_CPU *cpu, WORD inst)
{
    BYTE max = (inst & 0x0F00) >> 8;
    load_vxy(cpu, 0, max);
}

/* FX75: Store the content of the registers v0 to vX into flags storage (outside of the addressable ram).
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_FX75(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
}

/* FX85: Store the content of the registers v0 to vX into flags storage (outside of the addressable ram).
    - CHIP 8: Unimplemented.
    - SCHIPC: Unimplemented.
    - XO-CHIP: Normal behaviour.
*/
static inline void OP_FX85(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
}

static inline void OP_NULL(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
}

#endif