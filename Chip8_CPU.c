#include "Chip8_CPU.h"
#include "Chip8_Instructions.h"


void aux_0XXX(Chip8_CPU *cpu, WORD inst)
{
    switch (inst & 0x00FF)
    {
    case (0x00E0): OP_00E0(cpu); break;
    case (0x00EE): OP_00EE(cpu); break;
    default: OP_NULL(cpu, inst); break;
    }
}

void aux_8XYN(Chip8_CPU *cpu, WORD inst)
{
    switch ( inst & 0x000F)
    {
    case 0x0: OP_8XY0(cpu,inst); break;
    case 0x1: OP_8XY1(cpu,inst); break;
    case 0x2: OP_8XY2(cpu,inst); break;
    case 0x3: OP_8XY3(cpu,inst); break;
    case 0x4: OP_8XY4(cpu,inst); break;
    case 0x5: OP_8XY5(cpu,inst); break;
    case 0x6: OP_8XY6(cpu,inst); break;
    case 0x7: OP_8XY7(cpu,inst); break;
    case 0xe: OP_8XYE(cpu,inst); break;
    default: OP_NULL(cpu,inst); break;
    }
    
}

void aux_EXYN(Chip8_CPU *cpu, WORD inst)
{   
    switch (inst & 0x00FF)
    {
    case (0x009E): OP_EX9E(cpu, inst); break;
    case (0x00A1): OP_EXA1(cpu, inst); break;
    default: OP_NULL(cpu, inst); break;
    }
}

void aux_FXNN(Chip8_CPU *cpu, WORD inst)
{
    switch (inst & 0x00FF)
    {
    case 0x07: OP_FX07(cpu, inst); break;
    case 0x0A: OP_FX0A(cpu, inst); break;
    case 0x15: OP_FX15(cpu, inst); break;
    case 0x18: OP_FX18(cpu, inst); break;
    case 0x1E: OP_FX1E(cpu, inst); break;
    case 0x29: OP_FX29(cpu, inst); break;
    case 0x33: OP_FX33(cpu, inst); break;
    case 0x55: OP_FX55(cpu, inst); break;
    case 0x65: OP_FX65(cpu, inst); break;
    default: OP_NULL(cpu, inst); break;
    }
}

void exec_instruction(Chip8_CPU *cpu)
{   
    WORD instruction = cpu->game_memory[cpu->program_counter++];
    instruction <<= 8;
    instruction |= cpu->game_memory[cpu->program_counter++];    
    
    switch ((instruction & 0xF000) >> 12)
    {
    case 0x0: aux_0XXX(cpu,instruction); break;
    case 0x1: OP_1NNN(cpu,instruction); break;
    case 0x2: OP_2NNN(cpu,instruction); break;
    case 0x3: OP_3XNN(cpu,instruction); break;
    case 0x4: OP_4XNN(cpu,instruction); break;
    case 0x5: OP_5XY0(cpu,instruction); break;
    case 0x6: OP_6XNN(cpu,instruction); break;
    case 0x7: OP_7XNN(cpu,instruction); break;
    case 0x8: aux_8XYN(cpu,instruction); break;
    case 0x9: OP_9XY0(cpu,instruction); break;
    case 0xa: OP_ANNN(cpu,instruction); break;
    case 0xb: OP_BNNN(cpu,instruction); break;
    case 0xc: OP_CXNN(cpu,instruction); break;
    case 0xd: OP_DXYN(cpu,instruction); break;
    case 0xe: aux_EXYN(cpu,instruction); break;
    case 0xf: aux_FXNN(cpu,instruction); break;
    default: OP_NULL(cpu,instruction); break;
    }
}

void cpu_reset(Chip8_CPU *cpu)
{
    memset(cpu->game_memory, 0, sizeof(cpu->game_memory));
    memset(cpu->game_registers, 0, sizeof(cpu->game_registers));
    memset(cpu->screen_buffer, 0, sizeof(cpu->screen_buffer));
    memcpy(&cpu->game_memory[0x050], &digits, sizeof(digits));
    reset_stack(&cpu->call_stack);
    cpu->i_register = 0;
    cpu->program_counter = 0x200;
    cpu->pressed_key = 16;
}

void init_cpu(Chip8_CPU *cpu, FILE *stream, Target_Platform target)
{
    cpu_reset(cpu);
    cpu->target = target;
    cpu->mode = LORES;
    fread(&cpu->game_memory[0x200], sizeof(BYTE), 0xfff, stream);
}

void run_instructions(Chip8_CPU *cpu, int n_instructions)
{
    for (int i = 0; i < n_instructions; i++)
    {
        //printf("0x%04x  0x%0x4\n",cpu->game_memory[cpu->program_counter-2],cpu->program_counter-2);
        exec_instruction(cpu);
    }
}

void update_timers(Chip8_CPU *cpu)
{
    if (cpu->delay_timer > 0)
    {
        cpu->delay_timer--;
    }
    if (cpu->sound_timer > 0)
    {
        printf("BEEP\n");
        cpu->sound_timer--;
    }
}
