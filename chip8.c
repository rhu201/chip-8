/* Simple chip-8 emulator written in C.
 * @Author: Alexandre Valadão Delazeri
 * @email: delazeri@alunos.utfpr.edu.br
 *****************************************
 * This file: Implementation of the emulator's guts.
 * */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

#include "chip8.h"

#ifdef __DEBUG__

/* If debug is set, the program will write the opcodes to OPLOG
 * and dump the screen to GFX.
 *
 * @TODO: Other types of memory dumps?
 * */

#define OPLOG "opcode.log"
#define GFXLOG "gfx.log"

#endif

uint8_t chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

/* Functions */

void c8_init (State* c8) {
	c8->opcode = 0;
	c8->pc = 0x200; /* By all sources this is the standard initial position */
	c8->sp = 0;
	c8->I = 0;

	c8->last_delay_update = c8->last_sound_update = clock();

	memset (&(c8->memory), 0, sizeof(uint8_t) * 4096);
	memset (&(c8->v), 0, sizeof(uint8_t) * 16);
	memset (&(c8->display), 0, sizeof(uint8_t) * 64 * 32);

	memset (&(c8->stack), 0, sizeof(uint16_t) * 16);

	memcpy(c8->memory, chip8_fontset, sizeof (uint8_t) * 80);

	srand(time(NULL));

#ifdef __DEBUG__
	c8->op_log = fopen (OPLOG, "w");
	c8->gfx_log = fopen (GFXLOG, "w");
#endif
}

void c8_load_game (State* c8, char* fp) {
	FILE* game = fopen (fp, "rb");

	//get file size
	fseek (game, 0, SEEK_END);
	size_t size = ftell (game);
	fseek (game, 0, SEEK_SET);

	/* Test if the file actually fits in memory */
	assert (512 < 4096 - size);

	//read entire file to memory starting at 0x200
	fread ((c8->memory + 512), size, 1, game);

	fclose (game);
}

void c8_clear_screen (State *c8) {
	for (int i = 0 ; i < 64*32 ; i++)
		c8->display[i] = 0;
}

void c8_emulate_cycle (State* c8) {

	//fetch

	c8->opcode = (c8->memory[c8->pc] << 8) | c8->memory[c8->pc+1];

	//decode & execute

	/* Decoding the upcodes could be done better, for sure.
	 * We could break it in pieces before (extract the first 4 bits, 
	 * the second 4, the third four bits, the last 8 and the last 12 and the last 4)
	 * and use then for tests, reducind the number of operations and constants in the code.
	 *
	 * Also, we could group all the c8->pc+=2 in a single line after the switch.
	 *
	 * Maybe make a function for each instruction?
	 * */

	switch (c8->opcode & 0xF000) { //get first 4 bits of the opcode

		//We will not implement 0x0NNN
		case 0x0000:
			if ((c8->opcode & 0x000F) == 0x0000) {
				c8_clear_screen (c8);
				c8->pc+=2; 
			} else if ((c8->opcode & 0xFF00) == 0x0000) {
				//return from a routine
				c8->pc = c8->stack[--(c8->sp)];
				c8->pc += 2; /* Forgetting this little guy cost me +2 hours */
			}
			break;

		case 0x1000:
			c8->pc = c8->opcode & 0x0FFF;
			break;

		case 0x2000:
			c8->stack[(c8->sp)++] = c8->pc;
			c8->pc = c8->opcode & 0x0FFF;
			break;

		case 0x3000:
			if (c8->v[(c8->opcode & 0x0F00) >> 8] == (c8->opcode & 0x00FF)) {
				c8->pc += 4;
			} else {
				c8->pc += 2;
			}
			break;

		case 0x4000:
			if (c8->v[(c8->opcode & 0x0F00) >> 8] != (c8->opcode & 0x00FF)) {
				c8->pc += 4;
			} else {
				c8->pc += 2;
			}
			break;

		case 0x5000:
			if (c8->v[(c8->opcode & 0x0F00) >> 8] == c8->v[(c8->opcode & 0x00F0) >> 4]) {
				c8->pc += 4;
			} else {
				c8->pc += 2;
			}
			break;

		case 0x6000:
			c8->v[(c8->opcode & 0x0F00) >> 8] = (c8->opcode & 0x00FF);
			c8->pc += 2;
			break;

		case 0x7000:
			c8->v[(c8->opcode & 0x0F00) >> 8] += (c8->opcode & 0x00FF);
			c8->pc += 2;
			break;

		/* Our shift instructions follow the official standard of 
		 * Vx = Vy shift 1 instead of Vx = Vx shift 1 that is often seen on intenet.
		 *
		 * */

		case 0x8000:
			switch (c8->opcode & 0x000F) {
				case 0x0000:
					c8->v[(c8->opcode & 0x0F00) >> 8] = c8->v[(c8->opcode & 0x00F0) >> 4];
					break;
				case 0x0001:
					c8->v[(c8->opcode & 0x0F00) >> 8] |= c8->v[(c8->opcode & 0x00F0) >> 4];
					break;

				case 0x0002:
					c8->v[(c8->opcode & 0x0F00) >> 8] &= c8->v[(c8->opcode & 0x00F0) >> 4];
					break;

				case 0x0003:
					c8->v[(c8->opcode & 0x0F00) >> 8] ^= c8->v[(c8->opcode & 0x00F0) >> 4];
					break;

				case 0x0004:
					if (c8->v[(c8->opcode & 0x0F00) >> 8] > INT8_MAX - c8->v[(c8->opcode & 0x00F0) >> 4]) {
						c8->v[15] = 0;
					} else {
						c8->v[15] = 1;
					}
					c8->v[(c8->opcode & 0x0F00) >> 8] += c8->v[(c8->opcode & 0x00F0) >> 4];
					break;

				case 0x0005:
					if (c8->v[(c8->opcode & 0x0F00) >> 8] < c8->v[(c8->opcode & 0x00F0) >> 4]) {
						c8->v[15] = 0;
					} else {
						c8->v[15] = 1;
					}
					c8->v[(c8->opcode & 0x0F00) >> 8] -= c8->v[(c8->opcode & 0x00F0) >> 4];
					break;

				case 0x0006:
					c8->v[15] = c8->v[(c8->opcode & 0x0F00) >> 8] & 1;
					c8->v[(c8->opcode & 0x0F00) >> 8] = c8->v[(c8->opcode & 0x00F0)] >> 1;
					break;

				case 0x0007:
					if (c8->v[(c8->opcode & 0x00F0) >> 4] < c8->v[(c8->opcode & 0x0F00) >> 8]) {
						c8->v[15] = 0;
					} else {
						c8->v[15] = 1;
					}
					c8->v[(c8->opcode & 0x0F00) >> 8] = c8->v[(c8->opcode & 0x00F0) >> 4] - c8->v[(c8->opcode & 0x0F00) >> 8];
					break;

				case 0x000E:
					c8->v[15] = (int8_t) c8->v[(c8->opcode & 0x0F00) >> 8] < 0 ? 1 : 0 ;
					c8->v[(c8->opcode & 0x0F00) >> 8] = c8->v[(c8->opcode & 0x00F0) >> 4] << 1;
					break;
			}

			c8->pc += 2;
			break;

		case 0x9000:
			if (c8->v[(c8->opcode & 0x0F00) >> 8] != c8->v[(c8->opcode & 0x00F0) >> 4]) {
				c8->pc += 4;
			} else {
				c8->pc += 2;
			}
			break;

		case 0xA000:
			c8->I = c8->opcode & 0x0FFF;
			c8->pc += 2;
			break;

		case 0xB000:
			c8->pc = c8->v[0] + (c8->opcode & 0xFFF);
			break;

		case 0xC000:
			c8->v[(c8->opcode & 0x0F00) >> 8] = (rand() % 256) & (c8->opcode & 0x00FF);
			c8->pc += 2;
			break;

		case 0xD000:
			/* This one is a conversion from Laurence Muller's excellent tutorial*/
			c8->v[0xF] = 0;
			uint16_t pixel;
			for (int yline = 0; yline < (c8->opcode & 0x000F); yline++) {
				pixel = c8->memory[c8->I + yline];
				for(int xline = 0; xline < 8; xline++) {
					if((pixel & (0x80 >> xline)) != 0) {
						if(c8->display[(c8->v[(c8->opcode & 0x0F00) >> 8] + xline + ((c8->v[(c8->opcode & 0x00F0) >> 4] + yline) * 64))] == 1)
							c8->v[0xF] = 1;                                 
						c8->display[c8->v[(c8->opcode & 0x0F00) >> 8] + xline + ((c8->v[(c8->opcode & 0x00F0) >> 4] + yline) * 64)] ^= 1;
					}
				}
			}
			c8->pc += 2;
			break;

		case 0xE000:
			if ((c8->opcode & 0x00FF) == 0x009E) {
				if (c8->keys[c8->v[(c8->opcode & 0x0F00) >> 8]] != 0) {
					c8->pc += 4;
				} else {
					c8->pc += 2;
				}
			} else {
				if (c8->keys[c8->v[(c8->opcode & 0x0F00) >> 8]] == 0) {
					c8->pc += 4;
				} else {
					c8->pc += 2;
				}
			}
			break;

		case 0xF000:
			if ((c8->opcode & 0x00FF) == 0x0007) {
				c8->v[(c8->opcode & 0x0F00) >> 8] = c8->delay_timer;
				c8->pc += 2;
			} else if ((c8->opcode & 0x00FF) == 0x000A) {
				bool key = false;

				for (int i = 0 ; i < 16 ; i++) {
					if (c8->keys[i] != 0) {
						c8->v[(c8->opcode & 0x0F00) >> 8] = i;
						key = true;
					}
				}

				if (!key) {
					return;
				}

				c8->pc += 2;
			} else if ((c8->opcode & 0x00FF) == 0x0015) {
				c8->delay_timer = c8->v[(c8->opcode & 0x0F00) >> 8];
				c8->last_delay_update = clock();
				c8->pc += 2;
			} else if ((c8->opcode & 0x00FF) == 0x0018) {
				c8->sound_timer = c8->v[(c8->opcode & 0x0F00) >> 8];
				c8->last_sound_update = clock();
				c8->pc += 2;
			} else if ((c8->opcode & 0x00FF) == 0x001E) {
				/*
				   We test the sum for overflow. We could cast the operands for a wider type,
				   but this way also works. Notice that the "usual way" actually doesn't work. 
				   You can't (or at least shouldn't be able to)
				   generate an overflow and then test it 
				   due to how the C standard defines things.
				   */
				if (c8->I > (uint16_t) INT16_MAX - ((uint16_t)c8->v[(c8->opcode & 0x0F00) >> 8])) {
					c8->v[15] = 1;
				} else {
					c8->v[15] = 0;
				}

				c8->I += c8->v[(c8->opcode & 0x0F00) >> 8];
				c8->pc += 2;	
			} else if ((c8->opcode & 0x00FF) == 0x0029) {
				c8->I = c8->v[(c8->opcode & 0x0F00) >> 8] * 0x5;
				c8->pc += 2;
			} else if ((c8->opcode & 0x00FF) == 0x0033) {
				c8->memory[c8->I] = c8->v[(c8->opcode & 0x0F00) >> 8] / 100;
				c8->memory[c8->I + 1] = (c8->v[(c8->opcode & 0x0F00) >> 8] % 100) / 10;
				c8->memory[c8->I + 2] = c8->v[(c8->opcode & 0x0F00) >> 8] % 10;
				c8->pc += 2;
			} else if ((c8->opcode & 0x00FF) == 0x0055) {
				for (int i = 0 ; i <= ((c8->opcode & 0x0F00) >> 8) ; i++) {
					c8->memory[c8->I + i] = c8->v[i];
				}
				//c8->I += 1 + ((c8->opcode & 0x0F00) >> 8);
				c8->pc += 2;
			} else {
				for (int i = 0 ; i <= ((c8->opcode & 0x0F00) >> 8) ; i++) {
					c8->v[i] = c8->memory[c8->I + i];
				}
				c8->pc += 2;
			} 
			break;
	}

	//update timers

	if (c8->delay_timer > 0) {
		if (((double)(clock() - c8->last_delay_update))/CLOCKS_PER_SEC > 1/60.0) {
			c8->delay_timer--;
			c8->last_delay_update = clock();
		}
	}

	if (c8->sound_timer > 0) {
		if (((double)(clock() - c8->last_sound_update))/CLOCKS_PER_SEC > 1/60.0) {
			printf ("\a");
			c8->sound_timer--;
			c8->last_sound_update = clock();
		}
	}

#ifdef __DEBUG__
	fprintf (c8->op_log, "%#05x\n", c8->opcode);
	if ((c8->opcode & 0xF000) == 0xD000) {
		for (int i = 0 ; i < 64 ; i++) {
			for (int j = 0 ; j < 32 ; j++) {
				fprintf (c8->gfx_log, "%d", c8->display[j*64+i]);
			}
			fprintf (c8->gfx_log, "\n");
		}
		fprintf (c8->gfx_log, "\n\n\n\n\n");
	}
#endif
}
