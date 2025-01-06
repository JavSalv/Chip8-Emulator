CC = gcc
SRC_MAIN = main.c Chip8_CPU.c
SRC_TEST = SDL_test.c
TARGET_MAIN = main
TARGET_TEST = sdl_test
SDL_PATH = ./SDL2
SDL_LIB = $(SDL_PATH)/lib
SDL_INCLUDE = $(SDL_PATH)/include
CFLAGS = -Wall -Werror -pedantic
LDFLAGS = -Wl,-rpath=$(SDL_LIB) -L$(SDL_LIB) -l:libSDL2-2.0.so
INCLUDES = -I$(SDL_INCLUDE)

.DEFAULT_GOAL := main

.PHONY: all clean main test

all: main test

$(TARGET_MAIN): $(SRC_MAIN)
	$(CC) $(SRC_MAIN) -o $(TARGET_MAIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES)

$(TARGET_TEST): $(SRC_TEST)
	$(CC) $(SRC_TEST) -o $(TARGET_TEST) $(CFLAGS) $(LDFLAGS) $(INCLUDES)

clean:
	rm -f $(TARGET_MAIN) $(TARGET_TEST)