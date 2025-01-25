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


#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif


#define CHIP8_STACK_SIZE 16
#define CHIP8_CYCLES_PER_FRAME 1

#define CHIP8_MEMSIZE 0x0FFF
#define XOCHIP_MEMSIZE 0xFFFF

#define CHIP8_SCREEN_WIDTH 128
#define CHIP8_SCREEN_HEIGHT 64

#define SMALL_FONT_ADDRESS 0x0A0
#define BIG_FONT_ADDRESS 0x000


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
    WORD stack[CHIP8_STACK_SIZE];
    BYTE n_elements;
} Stack;

typedef struct
{
    BYTE game_memory[0xFFFF];
    BYTE game_registers[16];
    WORD i_register;
    WORD program_counter;
    Stack call_stack;

    BYTE screen_plane1[CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH];
    BYTE screen_plane2[CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH];
    BYTE dirty_flag;
    Display_Mode mode;
    BYTE bitplane;

    BYTE keys[16];
    BYTE pressed_key;

    BYTE delay_timer;
    BYTE sound_timer;

    Target_Platform target;
} Chip8_CPU;

static const BYTE small_font[] = {
    // Start at 0x0A0
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
    // End at 0x0F0
};


static const BYTE big_font[] = {
    // Start at 0x000
    0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, // 0
    0x18, 0x78, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0xFF, // 1
    0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // 2
    0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 3
    0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, // 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 5
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 6
    0xFF, 0xFF, 0x03, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18, // 7
    0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 8
    0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 9
    0x7E, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3, // A
    0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, // B
    0x3C, 0xFF, 0xC3, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xFF, 0x3C, // C
    0xFC, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0xFC, // D
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // E
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0  // F
    // End at 0x0A0
};


void init_cpu(Chip8_CPU *cpu, FILE *stream, Target_Platform target);

void run_instructions(Chip8_CPU *cpu, uint32_t CPF);

void update_timers(Chip8_CPU *cpu);

#endif