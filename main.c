#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STACK_SIZE 16

#define ASSERT(_bool, ...) do{if (!(_bool)){ fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE);}}while(0);

typedef uint8_t BYTE;
typedef uint16_t WORD;

typedef struct
{
    WORD stack[STACK_SIZE];
    BYTE n_elements;
} Stack;

typedef struct
{
    BYTE game_memory[0xFFF];
    BYTE game_registers[16];
    WORD i_register;
    WORD program_counter;
    Stack call_stack;
    BYTE screen_buffer[64 * 32];
} Chip8_CPU;

void return_subroutine(Chip8_CPU* cpu)
{
    ASSERT((cpu->call_stack.n_elements > 0),"[ERROR] Tried to pop empty stack at PC: 0x%04x\n",cpu->program_counter);
    cpu->program_counter =  cpu->call_stack.stack[--cpu->call_stack.n_elements];
}

void jump_subroutine(Chip8_CPU* cpu, WORD address)
{
    ASSERT((cpu->call_stack.n_elements < STACK_SIZE-1),"[ERROR] Tried to push full stack at PC: 0x%04x\n",cpu->program_counter);
    cpu->call_stack.stack[cpu->call_stack.n_elements++] = cpu->program_counter;
    cpu->program_counter = address;
}

void reset_stack(Stack *stack)
{
    memset(stack->stack, 0, sizeof(stack->stack));
    stack->n_elements = 0;
}

void cpu_reset(Chip8_CPU *cpu)
{
    memset(cpu->game_memory, 0, sizeof(cpu->game_memory));
    memset(cpu->game_registers, 0, sizeof(cpu->game_registers));
    memset(cpu->screen_buffer, 0, sizeof(cpu->screen_buffer));
    reset_stack(&cpu->call_stack);
    cpu->i_register = 0;
    cpu->program_counter = 0x200;
}

WORD fetch_instruction(Chip8_CPU *cpu)
{
    WORD instruction = cpu->game_memory[cpu->program_counter++];
    instruction <<= 8;
    instruction |= cpu->game_memory[cpu->program_counter++];
    return instruction;
}

BYTE get_vx_value(Chip8_CPU* cpu, WORD instruction)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    ASSERT((vX<=15 && vX >= 0), "[ERROR] Tried to acces data register V%u at PC: 0x%04x\n",vX,cpu->program_counter);
    return cpu->game_registers[vX];
}

BYTE get_vy_value(Chip8_CPU* cpu, WORD instruction)
{
    BYTE vY = (instruction & 0x00F0) >> 4;
    ASSERT((vY<=15 && vY >= 0), "[ERROR] Tried to acces data register V%u at PC: 0x%04x\n",vY,cpu->program_counter);
    return cpu->game_registers[vY];
}

// OP-CODE Guide from https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
void decode_instruction(Chip8_CPU *cpu, WORD inst)
{
    switch (inst & 0xF000)
    {
    // Instrucciones 00E0 y 00EE
    case (0x0000):
        switch (inst & 0x000F)
        {
        // 00E0: Clear Screen
        case (0x0000):
            memset(cpu->screen_buffer, 0, sizeof(cpu->screen_buffer));
            break;
        // 00EE: Return from a subroutine
        case (0x000E):
            return_subroutine(cpu);
            break;
        }

    // 1NNN: Jump to address NNN
    case (0x1000):
        cpu->program_counter = (inst & 0x0fff);
        break;
    // 2NNN: Execute subroutine starting at address NNN
    case (0x2000):
        jump_subroutine(cpu, (inst & 0x0fff) );
        break;
    // 3XNN: Skip the following instruction if the value of register VX equals NN
    case (0x3000):
        BYTE value = get_vx_value(cpu, inst);
        BYTE NN = inst & 0xFF;
        if(value == NN) cpu->program_counter+=2; 
        break;
    // 4XNN: Skip the following instruction if the value of register VX is not equal to NN
    case (0X4000):
        BYTE value = get_vx_value(cpu, inst);
        BYTE NN = inst & 0xFF;
        if(value != NN) cpu->program_counter+=2;
        break;
    // 5XY0: Skip the following instruction if the value of register VX is equal to the value of register VY
    case (0X5000):
        break;
    // 6XNN: Store number NN in register VX
    case (0x6000):
        break;
    // 7XNN: Add the value NN to register VX
    case (0x7000):
        break;

    // Instrucciones 8XXX
    case (0x8000):
        switch (inst & 0xF00F)
        {
        // 8XY0: Store the value of register VY in register VX
        case (0x8000):
            break;
        // 8XY1: Set VX to VX OR VY
        case (0x8001):
            break;
        // 8XY2: Set VX to VX AND VY
        case (0x8002):
            break;
        // 8XY3: Set VX to VX XOR VY
        case (0x8003):
            break;
        // 8XY4: Add the value of register VY to register VX. Set VF to 01 if a carry occurs. Set VF to 00 if a carry does not occur
        case (0x8004):
            break;
        // 8XY5: Subtract the value of register VY from register VX. Set VF to 00 if a borrow occurs. Set VF to 01 if a borrow does not occur
        case (0x8005):
            break;
        // 8XY6: Store the value of register VY shifted right one bit in register VX. Set register VF to the least significant bit prior to the shift. VY is unchanged
        case (0x8006):
            break;
        // 8XY7: Set register VX to the value of VY minus VX .Set VF to 00 if a borrow occurs. Set VF to 01 if a borrow does not occur.
        case (0x8007):
            break;
        // 8XYE: Store the value of register VY shifted left one bit in register VX. Set register VF to the most significant bit prior to the shift. VY is unchanged.
        case (0x800E):
            break;
        }

    // 9XY0: Skip the following instruction if the value of register VX is not equal to the value of register VY
    case (0x9000):
        break;
    // ANNN: Store memory address NNN in register I
    case (0xA000):
        break;
    // BNNN: Jump to address NNN + V0
    case (0xB000):
        break;
    // CXNN: Set VX to a random number with a mask of NN
    case (0xC000):
        break;
    // DXYN: Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I. Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
    case (0xD000):
        break;

    case (0xE000):
        switch (inst & 0xF0FF)
        {
        // EX9E: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed.
        case (0xE09E):
            break;
        // EXA1: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
        case (0xE0A1):
            break;
        }

    case (0xF000):
        switch (inst & 0xF0FF)
        {
        // FX07: Store the current value of the delay timer in register VX
        case (0xF007):
            break;
        // FX0A: Wait for a keypress and store the result in register VX
        case (0xF00A):
            break;
        // FX15: Set the delay timer to the value of register VX
        case (0xF015):
            break;
        // FX18: Set the sound timer to the value of register VX
        case (0xF018):
            break;
        // FX1E: Add the value stored in register VX to register I
        case (0xF01E):
            break;
        // FX29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
        case (0xF029):
            break;
        // FX33: Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
        case (0xF033):
            break;
        // FX55: Store the values of registers V0 to VX inclusive in memory starting at address I. I is set to I + X + 1 after operation.
        case (0xF055):
            break;
        // FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation.
        case (0xF065):
            break;
        }
    default:
        fprintf(stderr, "[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    Chip8_CPU cpu = {};
    cpu_reset(&cpu);
    const char *filename = "test_opcode.ch8";

    FILE *fd = fopen(filename, "rb");
    if (NULL == fd)
    {
        fprintf(stderr, "[ERROR] \"%s\" No such file or directory.\n", filename);
        exit(1);
    }

    fread(&cpu.game_memory[0x200], sizeof(BYTE), 0xfff, fd);
    fclose(fd);
    WORD inst;

    while (cpu.program_counter < 0xfff)
    {
        inst = fetch_instruction(&cpu);
        printf("0x%04x\n", inst);
        decode_instruction(&cpu, inst);
    }

    return 0;
}
