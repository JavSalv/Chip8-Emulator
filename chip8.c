#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "SDL2/SDL.h"
#include "Chip8_CPU.h"

#define FPS_TARGET 60 // Dont change this or cpu timing will get weird.

// #define BACKGROUND 0x99660000
// #define FOREGROUND 0xFFCC0000
#define BACKGROUND 0xF9FFB300
#define FOREGROUND 0x3D802600

#define PLANE0 BACKGROUND
#define PLANE1 FOREGROUND
#define PLANE2 0x00000000
#define PLANE3 0xFF00FFFF

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 512

const struct
{
    SDL_Keycode keycode;
    BYTE hex_value;
} KEY_MAPPINGS[] = {
    {SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC}, {SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD}, {SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE}, {SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF}};

const uint32_t colors[] = {PLANE0, PLANE1, PLANE2, PLANE3};

void key_event_handler(Chip8_CPU *cpu, SDL_Event *event)
{
    BYTE value = (event->type == SDL_KEYDOWN) ? 1 : 0;

    for (BYTE i = 0; i < 16; i++)
    {
        if (KEY_MAPPINGS[i].keycode == event->key.keysym.sym)
        {
            cpu->keys[KEY_MAPPINGS[i].hex_value] = value;
            break;
        }
    }
}

// Frame cap from https://github.com/tsoding/sowon/blob/master/main.c
typedef struct
{
    Uint32 frame_delay;
    float dt;
    Uint64 last_time;
} FpsDeltaTime;

FpsDeltaTime make_fpsdeltatime(const Uint32 fps_cap)
{
    return (FpsDeltaTime){
        .frame_delay = (1000 / fps_cap),
        .dt = 0.0f,
        .last_time = SDL_GetPerformanceCounter(),
    };
}

void frame_start(FpsDeltaTime *fpsdt)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - fpsdt->last_time;
    fpsdt->dt = ((float)elapsed) / ((float)SDL_GetPerformanceFrequency());
    // printf("\rFPS: %f | dt %f", 1.0 / fpsdt->dt, fpsdt->dt);
    fpsdt->last_time = now;
}

void frame_end(FpsDeltaTime *fpsdt)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - fpsdt->last_time;
    const Uint32 cap_frame_end = (Uint32)((((float)elapsed) * 1000.0f) / ((float)SDL_GetPerformanceFrequency()));

    if (cap_frame_end < fpsdt->frame_delay)
    {
        SDL_Delay((fpsdt->frame_delay - cap_frame_end));
    }
}

void cpu_to_screen(BYTE *screen_plane1, BYTE *screen_plane2, uint32_t *screen_buffer)
{
    for (int i = 0; i < CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT; i++)
    {
        screen_buffer[i] = colors[(screen_plane2[i] << 1) | screen_plane1[i]];
    }
}

int main(int argc, char *argv[])
{
    int retval;
    int running = 1;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen_texture;
    uint32_t *screen_buffer;
    char title[255];

    Chip8_CPU cpu = {0};
    Target_Platform target = XOCHIP;
    uint32_t cpf = CHIP8_CYCLES_PER_FRAME;
    const char *filename;

    char c;
    while ((c = getopt(argc, argv, "ht:c:")) != -1)
    {
        switch (c)
        {
        case 't': // Emulation Target
            if (strncmp(optarg, "Chip8", strlen(optarg)) == 0)
            {
                target = CHIP8;
            }
            else if (strncmp(optarg, "SuperChip", strlen(optarg)) == 0)
            {
                target = SCHIPC;
            }
            else if (strncmp(optarg, "XO-Chip", strlen(optarg)) == 0)
            {
                target = XOCHIP;
            }
            else
            {
                fprintf(stderr, "Unknown Chip8 variant '%s'.\nPossible options: Chip8 | SuperChip | XO-Chip \n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'c': // Cycles per frame
            cpf = atoi(optarg);
            if (cpf <= 0)
            {
                fputs("-c value must be greater than 0\n", stderr);
                exit(EXIT_FAILURE);
            }
            break;
        case 'h': // Help
            puts("A simple Chip 8 emulator/interpreter.\n"
                "\n"
                "Usage:\n"
                "    chip8 [OPTIONS] rom_filepath\n"
                "\n"
                "ARGS:\n"
                "    <rom_filepath>\n"
                "            Path to the .ch8 ROM you want to run.\n"
                "\n"
                "OPTIONS:\n"
                "    -t <TARGET>\n"
                "            Select the variant you want the emulator to target.\n"
                "            Possible targets: Chip8 | SuperChip | XO-Chip.\n"
                "    -c <CYCLES>\n"
                "            Speed you want the emulator to run, measured in\n"
                "            Cycles per Frame. One cycle equals one instruction.\n"
                "            Recommended value: 15-30.\n"
                "    -h\n"
                "            Displays this text.");
                exit(EXIT_SUCCESS);
            break;
        default:
            fprintf(stderr, "Usage: %s [-t target] [-c cycles] ROM\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc)
    {
        fputs("Missing ROM filepath\n", stderr);
        exit(EXIT_FAILURE);
    }

    filename = argv[optind];

    snprintf(title, sizeof(title), "Chip8 Emulator - ROM: %s", filename);
    FILE *fd = fopen(filename, "rb");
    ASSERT((fd != NULL), "[ERROR] \"%s\" No such file or directory.\n", filename);

    atexit(SDL_Quit);
    retval = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    ASSERT((retval == 0), "[ERROR] Can't initialize SDL: %s\n", SDL_GetError());

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    ASSERT((window != NULL), "[ERROR] Can't create SDL window: %s\n", SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    ASSERT((renderer != NULL), "[ERROR] Can't create SDL renderer: %s\n", SDL_GetError());

    retval = SDL_RenderSetLogicalSize(renderer, CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT);
    retval |= SDL_RenderSetIntegerScale(renderer, 1);
    ASSERT((retval == 0), "[ERROR] Can't set SDL render settings: %s\n", SDL_GetError());

    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT);
    ASSERT((screen_texture != NULL), "[ERROR] Can't create screen surface: %s\n", SDL_GetError());

    screen_buffer = calloc(CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH, sizeof(uint32_t));
    ASSERT((screen_buffer != NULL), "[ERROR] Can't allocate space for screen buffer : %s\n", strerror(errno));

    FpsDeltaTime fps_dt = make_fpsdeltatime(FPS_TARGET);

    init_cpu(&cpu, fd, target);
    fclose(fd);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

    while (running)
    {
        frame_start(&fps_dt);

        // Comienza input
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                key_event_handler(&cpu, &event);
                break;
            }
        }
        // Termina input
        // Ejecuto ciclo
        run_instructions(&cpu, cpf);
        update_timers(&cpu);
        if (cpu.dirty_flag)
        {
            cpu_to_screen(cpu.screen_plane1, cpu.screen_plane2, screen_buffer);
            cpu.dirty_flag = 0;
        }

        // Muestro en pantalla
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(screen_texture, NULL, screen_buffer, CHIP8_SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        frame_end(&fps_dt);
    }

    SDL_Quit();
}
