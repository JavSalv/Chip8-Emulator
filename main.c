#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "SDL2/SDL.h"
#include "Chip8_CPU.h"

#define FPS_TARGET 60

#define RENDER_WIDTH 64
#define RENDER_HEIGHT 32

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 512

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

void cpu_to_screen(BYTE cpu_buffer[RENDER_WIDTH][RENDER_HEIGHT], uint32_t *screen_buffer)
{

    for (int y = 0; y < RENDER_HEIGHT; y++)
        for (int x = 0; x < RENDER_WIDTH; x++)
        {
            if (cpu_buffer[x][y] == 1)
                screen_buffer[y * RENDER_WIDTH + x] = 0xFFFFFFFF;
            else
                screen_buffer[y * RENDER_WIDTH + x] = 0x0;
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

    Chip8_CPU cpu = {0};
    const char *filename = "test_opcode.ch8";

    FILE *fd = fopen(filename, "rb");
    ASSERT((fd != NULL), "[ERROR] \"%s\" No such file or directory.\n", filename);

    atexit(SDL_Quit);
    retval = SDL_Init(SDL_INIT_VIDEO);
    ASSERT((retval == 0), "[ERROR] Can't initialize SDL: %s\n", SDL_GetError());

    window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    ASSERT((window != NULL), "[ERROR] Can't create SDL window: %s\n", SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    ASSERT((window != NULL), "[ERROR] Can't create SDL renderer: %s\n", SDL_GetError());

    retval = SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH, RENDER_HEIGHT);
    retval |= SDL_RenderSetIntegerScale(renderer, 1);
    ASSERT((retval == 0), "[ERROR] Can't set SDL render settings: %s\n", SDL_GetError());

    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);
    ASSERT((screen_texture != NULL), "[ERROR] Can't create screen surface: %s\n", SDL_GetError());

    screen_buffer = calloc(RENDER_HEIGHT * RENDER_WIDTH,sizeof(uint32_t));
    ASSERT((screen_buffer != NULL), "[ERROR] Can't allocate space for screen buffer : %s\n", strerror(errno));

    FpsDeltaTime fps_dt = make_fpsdeltatime(FPS_TARGET);

    init_cpu(&cpu, fd);
    fclose(fd);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

    while (running)
    {
        frame_start(&fps_dt);

        // Comienza input
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
                break;
            }
        }
        // Termina input
        // Ejecuto ciclo
        run_instructions(&cpu, CYCLES_PER_FRAME);
        cpu_to_screen(cpu.screen_buffer,screen_buffer);
        // Muestro en pantalla
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(screen_texture, NULL, screen_buffer, RENDER_WIDTH * sizeof(uint32_t));
        SDL_RenderCopy(renderer,screen_texture,NULL,NULL);
        SDL_RenderPresent(renderer);

        frame_end(&fps_dt);
    }

    SDL_Quit();
}
