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
    ASSERT((cpu->call_stack.n_elements < STACK_SIZE - 1), "[ERROR] Tried to push full stack at PC: 0x%04x\n", cpu->program_counter);
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

BYTE get_vy(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vY = (instruction & 0x00F0) >> 4;
    return cpu->game_registers[vY];
}

static inline void dump_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE max = (instruction & 0x0F00) >> 8;

    for (int x = 0; x <= max; x++)
    {
        cpu->game_memory[cpu->i_register + x] = cpu->game_registers[x];
    }
    if (cpu->target == CHIP8)
        cpu->i_register += max + 1;
}

static inline void load_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE max = (instruction & 0x0F00) >> 8;

    for (int x = 0; x <= max; x++)
    {
        cpu->game_registers[x] = cpu->game_memory[cpu->i_register + x];
    }
    if (cpu->target == CHIP8)
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

static inline void draw_sprite_CHIP8(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = (get_vx(cpu, instruction) & 63) * 2;  // 61 = 123/2 para mantener dentro de límites
    BYTE coordY = (get_vy(cpu, instruction) & 31) * 2;  // 31 = 63/2 para mantener dentro de límites
    BYTE height = (instruction & 0xF);
    BYTE row;

    cpu->game_registers[0xF] = 0;

    for (int yline = 0; yline < height && (coordY + (yline * 2)) < 64; yline++)
    {
        row = cpu->game_memory[cpu->i_register + yline];
        for (int xpixel = 0; xpixel < 8 && (coordX + (xpixel * 2)) < 128; xpixel++)
        {
            if ((row & (0x80 >> xpixel)) != 0)
            {
                // Calculamos las posiciones para los 4 píxeles (2x2)
                int pos1 = coordX + (xpixel * 2) + ((coordY + (yline * 2)) * 128);
                
                // Verificamos si alguno de los píxeles ya está encendido
                if (pos1 + 129 < (128 * 64))  // Aseguramos que estamos dentro del buffer
                {
                    if (cpu->screen_buffer[pos1] == 1)
                    {
                        cpu->game_registers[0xF] = cpu->screen_buffer[pos1];
                    }
                    
                    cpu->screen_buffer[pos1] ^= 1;
                    cpu->screen_buffer[pos1 + 1] ^= 1;
                    cpu->screen_buffer[pos1 + 128] ^= 1;
                    cpu->screen_buffer[pos1 + 129] ^= 1;
                }
            }
        }
    }
    cpu->dirty_flag = 1;
}

static inline void OP_00E0(Chip8_CPU *cpu)
{

    memset(cpu->screen_buffer, 0, sizeof(cpu->screen_buffer));
}

static inline void OP_00EE(Chip8_CPU *cpu)
{
    return_subroutine(cpu);
}

// Instrucción Jump
static inline void OP_1NNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->program_counter = (inst & 0x0fff);
}

// Call subroutine
static inline void OP_2NNN(Chip8_CPU *cpu, WORD inst)
{
    jump_subroutine(cpu, (inst & 0x0fff));
}

// Skip if equal
static inline void OP_3XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vx(cpu, inst);
    BYTE NN = inst & 0xFF;
    if (value == NN)
        cpu->program_counter += 2;
}

// Skip if not equal
static inline void OP_4XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vx(cpu, inst);
    BYTE NN = inst & 0xFF;
    if (value != NN)
        cpu->program_counter += 2;
}

// Skip if registers equal
static inline void OP_5XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    if (vx == vy)
        cpu->program_counter += 2;
}

// Set register
static inline void OP_6XNN(Chip8_CPU *cpu, WORD inst)
{
    set_vx(cpu, inst);
}

// Add value
static inline void OP_7XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE value = (inst & 0x00FF);
    set_vx_value(cpu, inst, (vx + value));
}

static inline void OP_8XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vy(cpu, inst);
    set_vx_value(cpu, inst, value);
}

/* 8XY1: Set VX to VX OR VY
    - CHIP 8: Resets VF register
*/
static inline void OP_8XY1(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx | vy));
    if (cpu->target == CHIP8)
        cpu->game_registers[0xF] = 0;
}

/* 8XY2: Set VX to VX AND VY
    - CHIP 8: Resets VF register
*/
static inline void OP_8XY2(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx & vy));
    if (cpu->target == CHIP8)
        cpu->game_registers[0xF] = 0;
}

/* 8XY3: Set VX to VX XOR VY
    - CHIP 8: Resets VF register
*/
static inline void OP_8XY3(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx ^ vy));
    if (cpu->target == CHIP8)
        cpu->game_registers[0xF] = 0;
}

static inline void OP_8XY4(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx + vy));
    cpu->game_registers[15] = 0;
    if (vx + vy > 255)
        cpu->game_registers[0xF] = 1;
}

static inline void OP_8XY5(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx - vy));
    cpu->game_registers[15] = 1;
    if (vy > vx)
        cpu->game_registers[0xF] = 0;
}

static inline void OP_8XY6(Chip8_CPU *cpu, WORD inst)
{
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy >> 1));
    cpu->game_registers[0xF] = (vy & 0x1);
}

static inline void OP_8XY7(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy - vx));
    cpu->game_registers[15] = 1;
    if (vx > vy)
        cpu->game_registers[0xF] = 0;
}

static inline void OP_8XYE(Chip8_CPU *cpu, WORD inst)
{
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy << 1));
    cpu->game_registers[0xF] = (vy & 0x80) >> 7;
}

// Skip if registers not equal
static inline void OP_9XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    if (vx != vy)
        cpu->program_counter += 2;
}

// Set index register
static inline void OP_ANNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->i_register = (inst & 0x0FFF);
}

// BNNN: Jump to address NNN + V0
static inline void OP_BNNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->program_counter = cpu->game_registers[0x0] + (inst & 0x0FFF);
}

// Random
static inline void OP_CXNN(Chip8_CPU *cpu, WORD inst)
{
    set_vx_value(cpu, inst, (rand() & (inst & 0xFF)));
}

/*DXXN: Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I. Set VF to 01 if any set pixels are changed to unset, and 00 otherwise.
    - CHIP 8: Only `lores` display. Clips sprites.
    - SCHIPC: `lores` & `hires` display.
    - XO-CHIP: lores` & `hires` display. Wraps all sprites.
*/
static inline void OP_DXXN(Chip8_CPU *cpu, WORD inst)
{

    // TODO:Cambiar esto a puntero a funcion almacenado en Chip8_CPU.
    switch (cpu->target)
    {
    case CHIP8:
        draw_sprite_CHIP8(cpu, inst);
        break;
    case SCHIPC:
        break;
    case XOCHIP:
        break;
    }
}

// Skip if key pressed
static inline void OP_EX9E(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    if (cpu->keys[vx])
        cpu->program_counter += 2;
}

// Skip if key not pressed
static inline void OP_EXA1(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    if (!cpu->keys[vx])
        cpu->program_counter += 2;
}

// Get delay timer
static inline void OP_FX07(Chip8_CPU *cpu, WORD inst)
{
    set_vx_value(cpu, inst, cpu->delay_timer);
}

// Wait for key press
static inline void OP_FX0A(Chip8_CPU *cpu, WORD inst)
{
    wait_key(cpu, inst);
}

// Set delay timer
static inline void OP_FX15(Chip8_CPU *cpu, WORD inst)
{
    cpu->delay_timer = get_vx(cpu, inst);
}

// Set sound timer
static inline void OP_FX18(Chip8_CPU *cpu, WORD inst)
{
    cpu->sound_timer = get_vx(cpu, inst);
}

// Add to index
static inline void OP_FX1E(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->i_register += vx;
}

// Set index to sprite
static inline void OP_FX29(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->i_register = 0x50 + (5 * vx); // Max 0x9f
}

// Store BCD
static inline void OP_FX33(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->game_memory[cpu->i_register] = vx / 100;
    cpu->game_memory[cpu->i_register + 1] = (vx / 10) % 10;
    cpu->game_memory[cpu->i_register + 2] = vx % 10;
}

/* FX55: Store the values of registers V0 to VX inclusive in memory starting at address I
    - CHIP 8: I is set to I + X + 1 after operation
    - SCHIPC & XO-CHIP: I stays the same.
*/
static inline void OP_FX55(Chip8_CPU *cpu, WORD inst)
{
    dump_vx(cpu, inst);
}

/* FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I
    - CHIP 8: I is set to I + X + 1 after operation
    - SCHIPC & XO-CHIP: I stays the same.
*/
static inline void OP_FX65(Chip8_CPU *cpu, WORD inst)
{
    load_vx(cpu, inst);
}

static inline void OP_NULL(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
}

#endif