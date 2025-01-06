#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "SDL3/SDL.h"
#include "Chip8_CPU.h"


#define FPS_TARGET 60
#define CYCLES_PER_FRAME 10





//Frame cap from https://github.com/tsoding/sowon/blob/master/main.c
typedef struct {
    Uint32 frame_delay;
    float dt;
    Uint64 last_time;
} FpsDeltaTime;

FpsDeltaTime make_fpsdeltatime(const Uint32 fps_cap)
{
    return (FpsDeltaTime){
        .frame_delay=(1000 / fps_cap),
        .dt=0.0f,
        .last_time=SDL_GetPerformanceCounter(),
    };
}

void frame_start(FpsDeltaTime *fpsdt)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - fpsdt->last_time;
    fpsdt->dt = ((float)elapsed)  / ((float)SDL_GetPerformanceFrequency());
    printf("\rFPS: %f | dt %f", 1.0 / fpsdt->dt, fpsdt->dt);
    fpsdt->last_time = now;
}

void frame_end(FpsDeltaTime *fpsdt)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - fpsdt->last_time;
    const Uint32 cap_frame_end = (Uint32) ((((float)elapsed) * 1000.0f) / ((float)SDL_GetPerformanceFrequency()));

    if (cap_frame_end < fpsdt->frame_delay) {
        SDL_Delay((fpsdt->frame_delay - cap_frame_end) );
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
    SDL_Window* window;
    SDL_Renderer* rend;
    Chip8_CPU cpu = {0};
    const char *filename = "test_opcode.ch8";

    FILE *fd = fopen(filename, "rb");
    ASSERT((fd != NULL), "[ERROR] \"%s\" No such file or directory.\n", filename);

    atexit(SDL_Quit);
    retval = SDL_Init(SDL_INIT_VIDEO);
    ASSERT((retval == 1), "[ERROR] Can't initialize SDL: %s\n", SDL_GetError());

    retval = SDL_CreateWindowAndRenderer("Chip8 Emulator", 512,256,SDL_WINDOW_OPENGL, &window, &rend);
    ASSERT((retval == 1), "[ERROR] Can't create SDL window or renderer: %s\n", SDL_GetError());
    
    FpsDeltaTime fps_dt = make_fpsdeltatime(FPS_TARGET);

    init_cpu(&cpu,fd);
    fclose(fd);
    


    while(running)
    {   
        frame_start(&fps_dt);
        SDL_Event event = {0};
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_QUIT)
            {
                running = 0;
                break;
            }
        }
        frame_end(&fps_dt);
    }

    SDL_Quit();

}
