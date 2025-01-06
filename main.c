#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "SDL2/SDL.h"
#include "Chip8_CPU.h"

#define FPS_TARGET 60
#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 256

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
    //printf("\rFPS: %f | dt %f", 1.0 / fpsdt->dt, fpsdt->dt);
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

int main(int argc, char *argv[])
{
    /*     Chip8_CPU cpu = {};
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

        return 0; */

    int retval;
    int running = 1;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen_texture;
    uint32_t* screen_buffer;

    Chip8_CPU cpu = {0};
    const char *filename = "test_opcode.ch8";

    FILE *fd = fopen(filename, "rb");
    ASSERT((fd != NULL), "[ERROR] \"%s\" No such file or directory.\n", filename);

    atexit(SDL_Quit);
    retval = SDL_Init(SDL_INIT_VIDEO);
    ASSERT((retval == 1), "[ERROR] Can't initialize SDL: %s\n", SDL_GetError());

    window = SDL_CreateWindow("Chip8 Emulator", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    ASSERT((window != NULL), "[ERROR] Can't create SDL window: %s\n", SDL_GetError());

    renderer = SDL_CreateRenderer(window,NULL);
    ASSERT((window != NULL), "[ERROR] Can't create SDL renderer: %s\n", SDL_GetError());

    retval = SDL_SetRenderLogicalPresentation(renderer,64,32,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    ASSERT((retval == 1), "[ERROR] Can't set renderer options: %s\n", SDL_GetError());
    
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    ASSERT((screen_texture != NULL), "[ERROR] Can't create screen surface: %s\n", SDL_GetError());

    screen_buffer = calloc(64*32*sizeof(uint32_t),1);
    ASSERT((screen_buffer != NULL), "[ERROR] Can't allocate space for screen buffer : %s\n", strerror(errno));
    


    
    FpsDeltaTime fps_dt = make_fpsdeltatime(FPS_TARGET);

    init_cpu(&cpu, fd);
    fclose(fd);

    while (running)
    {
        frame_start(&fps_dt);

        // Comienza input
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = 0;
                break;
            }
        }
        // Termina input
        SDL_SetRenderDrawColor(renderer,0,0,0,0);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(screen_texture,NULL,screen_buffer,64*sizeof(uint32_t));
        SDL_RenderTextureRotated(renderer,screen_texture,NULL,NULL,0,NULL,SDL_FLIP_VERTICAL);
        SDL_RenderPresent(renderer);
        // Ejecuto instrucciones

        frame_end(&fps_dt);
    }

    SDL_Quit();
}
