#include "Chip8_CPU.h"

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
    ASSERT((vX <= 15 && vX >= 0), "[ERROR] Tried to acces data register V%u at PC: 0x%04x\n", vX, cpu->program_counter);
    return cpu->game_registers[vX];
}

void set_vx(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    ASSERT((vX <= 15 && vX >= 0), "[ERROR] Tried to acces data register V%u at PC: 0x%04x\n", vX, cpu->program_counter);
    cpu->game_registers[vX] = (instruction & 0xFF);
}

void set_vx_value(Chip8_CPU *cpu, WORD instruction, BYTE value)
{
    BYTE vX = (instruction & 0x0F00) >> 8;
    ASSERT((vX <= 15 && vX >= 0), "[ERROR] Tried to acces data register V%u at PC: 0x%04x\n", vX, cpu->program_counter);
    cpu->game_registers[vX] = value;
}

BYTE get_vy(Chip8_CPU *cpu, WORD instruction)
{
    BYTE vY = (instruction & 0x00F0) >> 4;
    ASSERT((vY <= 15 && vY >= 0), "[ERROR] Tried to acces data register V%u at PC: 0x%04x\n", vY, cpu->program_counter);
    return cpu->game_registers[vY];
}

void dump_vx(Chip8_CPU* cpu, WORD instruction)
{
    BYTE max = (instruction & 0x0F00) >> 8;

    for(int x = 0; x<= max; x++)
    {
        cpu->game_memory[cpu->i_register+x] = cpu->game_registers[x];
    }
    cpu->i_register+= max+1;
}

void load_vx(Chip8_CPU* cpu, WORD instruction)
{
    BYTE max = (instruction & 0x0F00) >> 8;

    for(int x = 0; x<= max; x++)
    {
        cpu->game_registers[x] = cpu->game_memory[cpu->i_register+x];
    }
    cpu->i_register+= max+1;

}

void draw_sprite(Chip8_CPU *cpu, WORD instruction)
{
    BYTE coordX = get_vx(cpu, instruction) & 63;
    BYTE coordY = get_vy(cpu, instruction) & 31;
    cpu->game_registers[0xF] = 0;
    BYTE height = instruction & 0xF;
    BYTE pixel;

    for (int yline = 0; yline < height; yline++)
    {
        pixel = cpu->game_memory[cpu->i_register + yline];
        for (int xline = 0; xline < 8; xline++)
        {
            if ((pixel & (0x80 >> xline)) != 0)
            {
                if (cpu->screen_buffer[(coordX + xline + ((coordY + yline) * 64))] == 1)
                    cpu->game_registers[0xF] = 1;
                cpu->screen_buffer[coordX + xline + ((coordY + yline) * 64)] ^= 1;
            }
        }
    }
}

WORD fetch_instruction(Chip8_CPU *cpu)
{
    WORD instruction = cpu->game_memory[cpu->program_counter++];
    instruction <<= 8;
    instruction |= cpu->game_memory[cpu->program_counter++];
    return instruction;
}

// OP-CODE Guide from https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
void exec_instruction(Chip8_CPU *cpu, WORD inst)
{
    BYTE value;
    BYTE NN;
    BYTE vx;
    BYTE vy;

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
        // 0000: NOP instruction.
        default:
            ASSERT((0),"[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
            break;
        }
        break;

    // 1NNN: Jump to address NNN
    case (0x1000):
        cpu->program_counter = (inst & 0x0fff);
        break;
    // 2NNN: Execute subroutine starting at address NNN
    case (0x2000):
        jump_subroutine(cpu, (inst & 0x0fff));
        break;
    // 3XNN: Skip the following instruction if the value of register VX equals NN
    case (0x3000):
        value = get_vx(cpu, inst);
        NN = inst & 0xFF;
        if (value == NN)
            cpu->program_counter += 2;
        break;
    // 4XNN: Skip the following instruction if the value of register VX is not equal to NN
    case (0X4000):
        value = get_vx(cpu, inst);
        NN = inst & 0xFF;
        if (value != NN)
            cpu->program_counter += 2;
        break;
    // 5XY0: Skip the following instruction if the value of register VX is equal to the value of register VY
    case (0X5000):
        vx = get_vx(cpu, inst);
        vy = get_vy(cpu, inst);
        if (vx == vy)
            cpu->program_counter += 2;
        break;
    // 6XNN: Store number NN in register VX
    case (0x6000):
        set_vx(cpu, inst);
        break;
    // 7XNN: Add the value NN to register VX
    case (0x7000):
        vx = get_vx(cpu, inst);
        value = (inst & 0x00FF);
        set_vx_value(cpu, inst, (vx + value));
        break;

    // Instrucciones 8XXX
    case (0x8000):
        switch (inst & 0xF00F)
        {
        // 8XY0: Store the value of register VY in register VX
        case (0x8000):
            value = get_vy(cpu, inst);
            set_vx_value(cpu, inst, value);
            break;
        // 8XY1: Set VX to VX OR VY
        case (0x8001):
            vx = get_vx(cpu, inst);
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vx | vy));
            break;
        // 8XY2: Set VX to VX AND VY
        case (0x8002):
            vx = get_vx(cpu, inst);
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vx & vy));
            break;
        // 8XY3: Set VX to VX XOR VY
        case (0x8003):
            vx = get_vx(cpu, inst);
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vx ^ vy));
            break;
        // 8XY4: Add the value of register VY to register VX. Set VF to 01 if a carry occurs. Set VF to 00 if a carry does not occur
        case (0x8004):
            vx = get_vx(cpu, inst);
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vx + vy));
            cpu->game_registers[15] = 0;
            if (vx + vy > 255)
                cpu->game_registers[0xF] = 1;
            break;
        // 8XY5: Subtract the value of register VY from register VX. Set VF to 00 if a borrow occurs. Set VF to 01 if a borrow does not occur
        case (0x8005):
            vx = get_vx(cpu, inst);
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vx - vy));
            cpu->game_registers[15] = 1;
            if (vy > vx)
                cpu->game_registers[0xF] = 0;
            break;
        // 8XY6: Store the value of register VY shifted right one bit in register VX. Set register VF to the least significant bit prior to the shift. VY is unchanged
        case (0x8006):
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vy >> 1));
            cpu->game_registers[0xF] = (vy & 0x1);
            break;
        // 8XY7: Set register VX to the value of VY minus VX .Set VF to 00 if a borrow occurs. Set VF to 01 if a borrow does not occur.
        case (0x8007):
            vx = get_vx(cpu, inst);
            vy = get_vy(cpu, inst);
            set_vx_value(cpu, inst, (vy - vx));
            cpu->game_registers[15] = 1;
            if (vx > vy)
                cpu->game_registers[0xF] = 0;
            break;
        // 8XYE: Store the value of register VY shifted left one bit in register VX. Set register VF to the most significant bit prior to the shift. VY is unchanged.
        case (0x800E):
            vy = get_vy(cpu, inst);
            set_vx_value(cpu,inst, (vy << 1));
            cpu->game_registers[0xF] = (vy & 0x80) >> 7;
            break;
        }
        break;

    // 9XY0: Skip the following instruction if the value of register VX is not equal to the value of register VY
    case (0x9000):
        vx = get_vx(cpu, inst);
        vy = get_vy(cpu, inst);
        if(vx != vy) cpu->program_counter+=2;
        break;
    // ANNN: Store memory address NNN in register I
    case (0xA000):
        cpu->i_register = (inst & 0x0FFF);
        break;
    // BNNN: Jump to address NNN + V0
    case (0xB000):
        cpu->program_counter += cpu->game_registers[0] + (inst & 0x0FFF);
        break;
    // CXNN: Set VX to a random number with a mask of NN
    case (0xC000):
        ASSERT((0),"[ERROR] Unimplemented instruction 'CXNN'\n");
        break;
    // DXYN: Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I. Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
    case (0xD000):
        draw_sprite(cpu, inst);
        break;

    case (0xE000):
        switch (inst & 0xF0FF)
        {
        // EX9E: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed.
        case (0xE09E):
            ASSERT((0),"[ERROR] Unimplemented instruction 'EX9E'\n");
            break;
        // EXA1: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
        case (0xE0A1):
            ASSERT((0),"[ERROR] Unimplemented instruction 'EXA1'\n");
            break;
        }
        break;

    case (0xF000):
        switch (inst & 0xF0FF)
        {
        // FX07: Store the current value of the delay timer in register VX
        case (0xF007):
            ASSERT((0),"[ERROR] Unimplemented instruction 'XXXX'\n");
            break;
        // FX0A: Wait for a keypress and store the result in register VX
        case (0xF00A):
            ASSERT((0),"[ERROR] Unimplemented instruction 'XXXX'\n");
            break;
        // FX15: Set the delay timer to the value of register VX
        case (0xF015):
            ASSERT((0),"[ERROR] Unimplemented instruction 'XXXX'\n");
            break;
        // FX18: Set the sound timer to the value of register VX
        case (0xF018):
            ASSERT((0),"[ERROR] Unimplemented instruction 'XXXX'\n");
            break;
        // FX1E: Add the value stored in register VX to register I
        case (0xF01E):
            vx = get_vx(cpu,inst);
            cpu->i_register+= vx;
            break;
        // FX29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
        case (0xF029):
            ASSERT((0),"[ERROR] Unimplemented instruction 'XXXX'\n");
            break;
        // FX33: Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
        case (0xF033):
            vx = get_vx(cpu,inst);
            cpu->game_memory[cpu->i_register] = vx / 100;
            cpu->game_memory[cpu->i_register+1] = (vx / 10) % 10;
            cpu->game_memory[cpu->i_register+2] = vx % 10;
            break;
        // FX55: Store the values of registers V0 to VX inclusive in memory starting at address I. I is set to I + X + 1 after operation.
        case (0xF055):
            dump_vx(cpu,inst);
            break;
        // FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation.
        case (0xF065):
            load_vx(cpu,inst);
            break;
        }
        break;
    default:
        ASSERT((0),"[ERROR] Unimplemented instruction \"0x%04x\" at PC: 0x%04x\n", inst, cpu->program_counter);
    }
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

void init_cpu(Chip8_CPU *cpu, FILE *stream)
{
    cpu_reset(cpu);
    fread(&cpu->game_memory[0x200], sizeof(BYTE), 0xfff, stream);
}

void run_instructions(Chip8_CPU *cpu, int n_instructions)
{
    WORD inst;
    for (int i = 0; i < n_instructions; i++)
    {
        inst = fetch_instruction(cpu);
        // printf("0x%04x\n",inst);
        exec_instruction(cpu, inst);
    }
}
