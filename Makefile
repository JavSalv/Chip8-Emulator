CC = gcc
SRC_MAIN = chip8.c Chip8_CPU.c
TARGET_MAIN = chip8
SDL_PATH = ./SDL2
SDL_LIB = $(SDL_PATH)/lib
SDL_INCLUDE = $(SDL_PATH)/include
CFLAGS = -Wall -Wextra -pedantic -O2
LDFLAGS = -Wl,-rpath=$(SDL_LIB) -L$(SDL_LIB) -l:libSDL2-2.0.so
INCLUDES = -I$(SDL_INCLUDE)

.DEFAULT_GOAL := $(TARGET_MAIN)

.PHONY: all clean chip8 

all: chip8

$(TARGET_MAIN): $(SRC_MAIN)
	$(CC) $(SRC_MAIN) -o $(TARGET_MAIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES)

clean:
	rm -f $(TARGET_MAIN) $(TARGET_DBG)