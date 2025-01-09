#ifndef CHIP8_INSTRUCTIONS_H
#define CHIP8_INSTRUCTIONS_H 1

#include "Chip8_CPU.h"

// OP-CODE Guide from https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set

void return_subroutine(Chip8_CPU *cpu)
{
    ASSERT((cpu->call_stack.n_elements > 0), "[ERROR] Tried to pop empty stack at PC: 0x%04x\n", cpu->program_counter);
    cpu->program_counter = cpu->call_stack.stack[--cpu->call_stack.n_elements];
}

void jump_subroutine(Chip8_CPU *cpu, WORD address)
{
    ASSERT((cpu->call_stack.n_elements < STACK_SIZE - 1), "[ERROR] Tried to push full stack at PC: 0x%04x\n", cpu->program_counter);
    cpu->call_stack.stack[cpu->call_stack.n_elements++] = cpu->program_counter;
    cpu->program_counter = address;
}

void reset_stack(Stack *stack)
{
    memset(stack->stack, 0, sizeof(stack->stack));
    stack->n_elements = 0;
}

BYTE get_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    return cpu->game_registers[vX];
}

void set_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    cpu->game_registers[vX] = (instruction & 0xFF);
}

void set_vx_value(Chip8_CPU *cpu, WORD instruction, BYTE value)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    cpu->game_registers[vX] = value;
}

BYTE get_vy(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vY = (instruction & 0x00F0) >> 4;
    return cpu->game_registers[vY];
}

void dump_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE max = (instruction & 0x0F00) >> 8;

    for (int x = 0; x <= max; x++)
    {
        cpu->game_memory[cpu->i_register + x] = cpu->game_registers[x];
    }
    cpu->i_register += max + 1;
}

void load_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE max = (instruction & 0x0F00) >> 8;

    for (int x = 0; x <= max; x++)
    {
        cpu->game_registers[x] = cpu->game_memory[cpu->i_register + x];
    }
    cpu->i_register += max + 1;
}

void wait_key(Chip8_CPU *cpu, WORD instruction)
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

void draw_sprite(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = get_vx(cpu, instruction) & 63;
    BYTE coordY = get_vy(cpu, instruction) & 31;
    cpu->game_registers[0xF] = 0;
    BYTE height = instruction & 0xF;
    BYTE pixel;

    for (int yline = 0; yline < height && (coordY + yline) < 32; yline++)
    {
        pixel = cpu->game_memory[cpu->i_register + yline];
        for (int xline = 0; xline < 8 && (coordX + xline) < 64; xline++)
        {
            if ((pixel & (0x80 >> xline)) != 0)
            {
                int pos = coordX + xline + ((coordY + yline) * 64);
                if (pos < 2048)
                {
                    if (cpu->screen_buffer[pos] == 1)
                        cpu->game_registers[0xF] = 1;
                    cpu->screen_buffer[pos] ^= 1;
                }
            }
        }
    }
}

void OP_00E0(Chip8_CPU *cpu)
{
    memset(cpu->screen_buffer, 0, sizeof(cpu->screen_buffer));
}

void OP_00EE(Chip8_CPU *cpu)
{
    return_subroutine(cpu);
}

// Instrucción Jump
void OP_1NNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->program_counter = (inst & 0x0fff);
}

// Call subroutine
void OP_2NNN(Chip8_CPU *cpu, WORD inst)
{
    jump_subroutine(cpu, (inst & 0x0fff));
}

// Skip if equal
void OP_3XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vx(cpu, inst);
    BYTE NN = inst & 0xFF;
    if (value == NN)
        cpu->program_counter += 2;
}

// Skip if not equal
void OP_4XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vx(cpu, inst);
    BYTE NN = inst & 0xFF;
    if (value != NN)
        cpu->program_counter += 2;
}

// Skip if registers equal
void OP_5XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    if (vx == vy)
        cpu->program_counter += 2;
}

// Set register
void OP_6XNN(Chip8_CPU *cpu, WORD inst)
{
    set_vx(cpu, inst);
}

// Add value
void OP_7XNN(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE value = (inst & 0x00FF);
    set_vx_value(cpu, inst, (vx + value));
}

// Instrucciones 8XXX
void OP_8XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE value = get_vy(cpu, inst);
    set_vx_value(cpu, inst, value);
}

void OP_8XY1(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx | vy));
}

void OP_8XY2(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx & vy));
}

void OP_8XY3(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx ^ vy));
}

void OP_8XY4(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx + vy));
    cpu->game_registers[15] = 0;
    if (vx + vy > 255)
        cpu->game_registers[0xF] = 1;
}

void OP_8XY5(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vx - vy));
    cpu->game_registers[15] = 1;
    if (vy > vx)
        cpu->game_registers[0xF] = 0;
}

void OP_8XY6(Chip8_CPU *cpu, WORD inst)
{
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy >> 1));
    cpu->game_registers[0xF] = (vy & 0x1);
}

void OP_8XY7(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy - vx));
    cpu->game_registers[15] = 1;
    if (vx > vy)
        cpu->game_registers[0xF] = 0;
}

void OP_8XYE(Chip8_CPU *cpu, WORD inst)
{
    BYTE vy = get_vy(cpu, inst);
    set_vx_value(cpu, inst, (vy << 1));
    cpu->game_registers[0xF] = (vy & 0x80) >> 7;
}

// Skip if registers not equal
void OP_9XY0(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    BYTE vy = get_vy(cpu, inst);
    if (vx != vy)
        cpu->program_counter += 2;
}

// Set index register
void OP_ANNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->i_register = (inst & 0x0FFF);
}

// Jump with offset
void OP_BNNN(Chip8_CPU *cpu, WORD inst)
{
    cpu->program_counter = (get_vx(cpu, inst) + (inst & 0x0FFF));
}

// Random
void OP_CXNN(Chip8_CPU *cpu, WORD inst)
{
    set_vx_value(cpu, inst, (rand() & (inst & 0xFF)));
}

// Draw sprite
void OP_DXXN(Chip8_CPU *cpu, WORD inst)
{
    draw_sprite(cpu, inst);
}

// Skip if key pressed
void OP_EX9E(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    if (cpu->keys[vx])
        cpu->program_counter += 2;
}

// Skip if key not pressed
void OP_EXA1(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    if (!cpu->keys[vx])
        cpu->program_counter += 2;
}

// Get delay timer
void OP_FX07(Chip8_CPU *cpu, WORD inst)
{
    set_vx_value(cpu, inst, cpu->delay_timer);
}

// Wait for key press
void OP_FX0A(Chip8_CPU *cpu, WORD inst)
{
    wait_key(cpu, inst);
}

// Set delay timer
void OP_FX15(Chip8_CPU *cpu, WORD inst)
{
    cpu->delay_timer = get_vx(cpu, inst);
}

// Set sound timer
void OP_FX18(Chip8_CPU *cpu, WORD inst)
{
    cpu->sound_timer = get_vx(cpu, inst);
}

// Add to index
void OP_FX1E(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->i_register += vx;
}

// Set index to sprite
void OP_FX29(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->i_register = 0x50 + (5 * vx); // Max 0x9f
}

// Store BCD
void OP_FX33(Chip8_CPU *cpu, WORD inst)
{
    BYTE vx = get_vx(cpu, inst);
    cpu->game_memory[cpu->i_register] = vx / 100;
    cpu->game_memory[cpu->i_register + 1] = (vx / 10) % 10;
    cpu->game_memory[cpu->i_register + 2] = vx % 10;
}

// Store registers
void OP_FX55(Chip8_CPU *cpu, WORD inst)
{
    dump_vx(cpu, inst);
}

// Load registers
void OP_FX65(Chip8_CPU *cpu, WORD inst)
{
    load_vx(cpu, inst);
}

void OP_NULL(Chip8_CPU *cpu, WORD inst)
{
    ASSERT((0), "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
}

#endif