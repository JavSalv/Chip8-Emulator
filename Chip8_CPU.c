#include "Chip8_CPU.h"
#include "Chip8_Instructions.h"

void (*I_8XYN[16])(Chip8_CPU *, WORD);
void (*I_base[16])(Chip8_CPU *, WORD);

void aux_0XXX(Chip8_CPU *cpu, WORD inst)
{
    switch (inst & 0x00FF)
    {
    case (0x00E0): return OP_00E0(cpu, inst);
    case (0x00EE): return OP_00EE(cpu, inst);
    default: return OP_NULL(cpu, inst);
    }
}

void aux_8XYN(Chip8_CPU *cpu, WORD inst)
{
    I_8XYN[(inst & 0x000F)](cpu, inst);
}

void aux_EXYN(Chip8_CPU *cpu, WORD inst)
{   
    switch (inst & 0x00FF)
    {
    case (0x009E): return OP_EX9E(cpu, inst);
    case (0x00A1): return OP_EXA1(cpu, inst);
    default: return OP_NULL(cpu, inst);
    }
}
void aux_FXNN(Chip8_CPU *cpu, WORD inst)
{
    switch (inst & 0x00FF)
    {
    case 0x07: return OP_FX07(cpu, inst);
    case 0x0A: return OP_FX0A(cpu, inst);
    case 0x15: return OP_FX15(cpu, inst);
    case 0x18: return OP_FX18(cpu, inst);
    case 0x1E: return OP_FX1E(cpu, inst);
    case 0x29: return OP_FX29(cpu, inst);
    case 0x33: return OP_FX33(cpu, inst);
    case 0x55: return OP_FX55(cpu, inst);
    case 0x65: return OP_FX65(cpu, inst);
    default: return OP_NULL(cpu, inst);
    }
}

void (*I_base[16])(Chip8_CPU *, WORD) =
    {
        aux_0XXX, OP_1NNN, OP_2NNN, OP_3XNN, OP_4XNN, OP_5XY0, OP_6XNN, OP_7XNN,
        aux_8XYN, OP_9XY0, OP_ANNN, OP_BNNN, OP_CXNN, OP_DXXN, aux_EXYN, aux_FXNN};

void (*I_8XYN[16])(Chip8_CPU *, WORD) =
    {
        OP_8XY0, OP_8XY1, OP_8XY2, OP_8XY3, OP_8XY4, OP_8XY5, OP_8XY6, OP_8XY7,
        OP_NULL, OP_NULL, OP_NULL, OP_NULL, OP_NULL, OP_NULL, OP_8XYE, OP_NULL};



void exec_instruction(Chip8_CPU *cpu)
{   
    WORD instruction = cpu->game_memory[cpu->program_counter++];
    instruction <<= 8;
    instruction |= cpu->game_memory[cpu->program_counter++];    

    I_base[(instruction & 0xF000) >> 12](cpu, instruction);
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

void init_cpu(Chip8_CPU *cpu, FILE *stream)
{
    cpu_reset(cpu);
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
