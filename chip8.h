/* Simple chip-8 emulator written in C.
 * @Author: Alexandre Valadão Delazeri
 * @email: delazeri@alunos.utfpr.edu.br
 *****************************************
 * This file: Declaration of the emulator's structs
 * and the functions that implement the emulator's operation.
 * */


#ifndef __CHIP8__
#define __CHIP8__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Struct defining the emulator and it's internal state.
 * We use c99's uintN_t as a way of being explicit about
 * the sizes of our everything.
 * */

typedef struct chip8 {
	uint8_t memory[4096];
	uint8_t v[16];
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint8_t display[64*32]; //replace by bit array?
	uint8_t keys[16];

	uint16_t stack[16];
	uint16_t sp;
	uint16_t I;
	uint16_t pc;
	uint16_t opcode;

	clock_t last_delay_update;
	clock_t last_sound_update;

#ifdef __DEBUG__
	FILE* op_log;
	FILE *gfx_log;
#endif

} State;

/* Function prototypes */

void c8_init (State* c8);
void c8_load_game (State* c8, char* fp);
void c8_clear_screen (State* c8);
void c8_emulate_cycle (State *c8);

#endif
