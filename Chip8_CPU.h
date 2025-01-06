#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STACK_SIZE 16

#define ASSERT(_bool, ...)                \
    do                                    \
    {                                     \
        if (!(_bool))                     \
        {                                 \
            fprintf(stderr, __VA_ARGS__); \
            exit(EXIT_FAILURE);           \
        }                                 \
    } while (0);

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


void init_cpu(Chip8_CPU* cpu, FILE* stream);