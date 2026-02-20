all: main.exe

CC=gcc
CFLAGS=-Wall -g -O0 -mwindows

SDL_VERSION=3.4.0
SDL_PLATFORM=x86_64-w64-mingw32

INCLUDE_PATHS=-I libraries/SDL3-$(SDL_VERSION)/$(SDL_PLATFORM)/include
LIBRARIES=-lSDL3
LIBRARY_PATHS=-L libraries/SDL3-$(SDL_VERSION)/$(SDL_PLATFORM)/lib

%.exe: %.CC
	$(CC) $(CFLAGS) $*.c -o $*.exe $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(LIBRARIES)

clean:
	del /f *.exe
