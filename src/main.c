#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>


#define STACK_SIZE 16

typedef uint8_t BYTE;
typedef uint16_t WORD;

typedef struct
{
    WORD stack[STACK_SIZE];
    BYTE stack_pointer;
}Stack;

typedef struct
{
    BYTE game_memory[0xFFF];
    BYTE game_registers[16];
    WORD i_register;
    WORD program_counter;
    Stack call_stack;
    BYTE screen_buffer[64*32]; 
}Chip8_CPU;



void reset_stack(Stack* stack)
{
    memset(stack->stack,0,sizeof(stack->stack));
    stack->stack_pointer = 0;
}

void cpu_reset(Chip8_CPU* cpu)
{
    memset(cpu->game_memory,0,sizeof(cpu->game_memory));
    memset(cpu->game_registers,0,sizeof(cpu->game_registers));
    memset(cpu->screen_buffer,0,sizeof(cpu->screen_buffer));
    reset_stack(&cpu->call_stack);
    cpu->i_register = 0;
    cpu->program_counter = 0x200;
}

WORD fetch_instruction(Chip8_CPU* cpu)
{
    WORD instruction = cpu->game_memory[cpu->program_counter++];
    instruction <<= 8;
    instruction |= cpu->game_memory[cpu->program_counter++];
    return instruction;
}


//OP-CODE Guide from https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
void decode_instruction(Chip8_CPU* cpu, WORD inst)
{
    switch (inst & 0xf000)
    {
    //Instrucciones 00E0 y 00EE
    case (0x0000):
        switch (inst & 0x000F)
        {
        //00E0: Clear Screen
        case (0x0000): memset(cpu->screen_buffer,0,sizeof(cpu->screen_buffer));; break;
        //00EE: Return from a subroutine
        case (0x000E): break;
        }
        break;

    //1NNN: Jump to address NNN
    case(0x1000): break;
    //2NNN: Execute subroutine starting at address NNN
    case(0x2000): break;
    case(0x3000): 
    default:
        break;
    }
}


int main(int argc, char* argv[])
{
    Chip8_CPU cpu = {};
    cpu_reset(&cpu);

    FILE* fd = fopen("test_opcode.ch8","rb");
    if(NULL == fd)
    {
        fprintf(stderr,"Error opening file\n");
        exit(1);
    }

    fread(&cpu.game_memory[0x200],sizeof(BYTE), 0xfff,fd);
    fclose(fd);

    while(cpu.program_counter < 0xfff)
    {
        printf("%04x\n", fetch_instruction(&cpu));
    }

    return 0;
}

