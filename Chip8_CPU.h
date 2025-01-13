#ifndef CHIP8_CPU_H
#define CHIP8_CPU_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef ASSERT
#define ASSERT(_bool, ...)                \
    do                                    \
    {                                     \
        if (!(_bool))                     \
        {                                 \
            fprintf(stderr, __VA_ARGS__); \
            exit(EXIT_FAILURE);           \
        }                                 \
    } while (0);
#endif


#define STACK_SIZE 16
#define CYCLES_PER_FRAME 12

typedef uint8_t BYTE;
typedef uint16_t WORD;

typedef enum
{
    CHIP8,
    SCHIPC,
    XOCHIP
}Target_Platform;

typedef enum
{
    LORES,
    HIRES
}Display_Mode;

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

    BYTE screen_buffer[128 * 64];
    BYTE dirty_flag;
    Display_Mode mode;
    BYTE bitplane;

    BYTE keys[16];
    BYTE pressed_key;

    BYTE delay_timer;
    BYTE sound_timer;

    Target_Platform target;
} Chip8_CPU;

static const BYTE digits[] = {
    // Start at 0x050
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
    // End at 0x09F
};


void init_cpu(Chip8_CPU *cpu, FILE *stream, Target_Platform target);

void run_instructions(Chip8_CPU *cpu, int n_instructions);

void update_timers(Chip8_CPU *cpu);

#endif