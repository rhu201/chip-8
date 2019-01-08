all:
	gcc main.c chip8.c -lGL -lGLU -lglut -lm -std=c99 -Wall -Wextra -pedantic -g -D__DEBUG__
