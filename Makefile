all: sdlfix.so test_sdlfix

sdlfix.so: sdlfix.o
	$(CC) -shared -fPIC -o $@ $<

test_sdlfix: test_sdlfix.o
	$(CC) -o $@ $^ `sdl-config --libs`

test_sdlfix.o: test_sdlfix.c
	$(CC) -c -o $@ $^ `sdl-config --cflags`

CFLAGS = -fPIC -g -O2 -Wall -Wextra -Werror 

.PHONY: clean

clean:
	$(RM) sdlfix.so sdlfix.o test_sdlfix.o test_sdlfix
