/* Simple chip-8 emulator written in C.
 * @Author: Alexandre Valadão Delazeri
 * @email: delazeri@alunos.utfpr.edu.br
 *****************************************
 * This file: Drawing logic.
 * */

#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include <limits.h>
#include <time.h>

#include "chip8.h"

/* Functions. */
void Init(void);
void Reshape(int w, int h);
void Display(void);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void DrawPoints(void);
void DrawLines();

/* Globals */

int winwidth, winheight;
State c8;
clock_t last;

#ifdef __DEBUG__
clock_t control;
int nops = 0;
#endif
/* Functions */

void Init(){
	/* Select clearing color. */
	glClearColor(155.0/255, 188.0/255, 15.0/255, 0.0);
}

void Reshape(int w, int h){
	winwidth  = w;
	winheight = h;

	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, (GLsizei) w, 0, (GLsizei) h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}

/* Maps keyboard keys to Chip-8 keys.
 * We use the convention set in Octo (https://github.com/JohnEarnest/Octo).
 *
 * 1 2 3 4         1 2 3 C
 * q w e r   ->    4 5 6 D
 * a s d f         7 8 9 E
 * z x c v         A 0 B F
 *
 * keyboard       chip-8
 *
 * */
int keyGetNum (unsigned char key) {
	switch (key) {
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 0xC;
		case 'q':
		case 'Q':
			return 4;
		case 'w':
		case 'W':
			return 5;
		case 'e':
		case 'E':
			return 6;
		case 'r':
		case 'R':
			return 0xD;
		case 'a':
		case 'A':
			return 7;
		case 's':
		case 'S':
			return 8;
		case 'd':
		case 'D':
			return 9;
		case 'f':
		case 'F':
			return 0xE;
		case 'z':
		case 'Z':
			return 0xA;
		case 'x':
		case 'X':
			return 0;
		case 'c':
		case 'C':
			return 0xB;
		case 'v':
		case 'V':
			return 0xF;
	}

	return -1;
}

void Keyboard(unsigned char key, __attribute__((unused)) int x, __attribute__((unused)) int y){

	/* ESC */
	if (key == 27)
		exit(0);

	c8.keys[keyGetNum (key)] = 1;

	glutPostRedisplay ();
}

void KeyUp (unsigned char key, __attribute__((unused)) int x, __attribute__((unused)) int y) {
	c8.keys[keyGetNum (key)] = 0;

	glutPostRedisplay ();
}

void Display (void) {

	/* If __DEBUG__ is set, keep track of how many instructions are executed by second.
	 * This is a very rough extimative, not taking into account the time it takes to OpenGL
	 * to render everything.
	 *
	 * On my machine it oscillates a bunch.
	 * */
#ifdef __DEBUG__
	if ((double)(clock() - control)/CLOCKS_PER_SEC >= 1) {
		printf ("In ~1 second, %d operations\n", nops);
		nops = 0;
		control = clock();
	}
#endif

	if (last == 0 ||(double)(clock() - last)/CLOCKS_PER_SEC >= 1/500.0) {
		// Do the next emulation cycle and get the time it ended.
		c8_emulate_cycle (&c8);
		last = clock();
		
#ifdef __DEBUG__
		nops++;
#endif
		
		/* Your run in the mill OpenGL 2D rendering.
		 * Not best pratices I guess.
		 * Keep in mind the screen is stored in the emulator's memory
		 * with the y-axis "inverted" (i.e. the same standard used for images).
		 * */

		glClear (GL_COLOR_BUFFER_BIT);
		glColor3f (15.0/255, 56.0/255, 15.0/255);
		glPointSize (9);

		glBegin (GL_POINTS);
		for (int i = 0 ; i < 64 ; i++) {
			for (int j = 0 ; j < 32 ; j++) {
				double x, y;
				x = i * ((double)winwidth)/64;
				y = j * ((double)winheight)/32;

				if (c8.display[j*64 + i])
					glVertex2d (x-5, winheight-y-5);
			}
		}
		glEnd ();
	}
	/* Draw. */
	glFlush ();
}

int main(int argc, char** argv){

	if (argc < 2) {
		printf ("Use as: $%s [rom_path]\n", argv[0]);
	}

	/* OpenGL Setup */

	/* Call glut init functions. */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition(10, 10);
	glutInitWindowSize(640, 320); 
	glutCreateWindow("chip-8");
	Init();

	/* Set glut callback functions. */
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyUp);
	glutMouseFunc(NULL);
	glutDisplayFunc(Display);
	glutIdleFunc (Display);

	/* Setup do Emulador */

	c8_init (&c8);
	c8_load_game (&c8, argv[1]);

#ifdef __DEBUG__
	control = clock();
#endif

	glutMainLoop();
	return 0;
}


