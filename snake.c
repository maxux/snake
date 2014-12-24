#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct segment_t {
	int x;
	int y;
	
} segment_t;

int mouvement;

//
// die with error
//
void diep(char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

//
// check if it's the last snake segment
//
int last(segment_t *segment) {
	return (segment->x == 0 && segment->y == 0);
}

//
// update snake direction
//
void direction(int key) {
	// not a new direction
	if(key != KEY_LEFT && key != KEY_RIGHT && key != KEY_UP && key != KEY_DOWN)
		return;
	
	// disallow rewind
	if((mouvement == KEY_LEFT && key == KEY_RIGHT) || (mouvement == KEY_RIGHT && key == KEY_LEFT))
		return;
	
	if((mouvement == KEY_UP && key == KEY_DOWN) || (mouvement == KEY_DOWN && key == KEY_UP))
		return;
	
	// allowed	
	mouvement = key;
}

//
// check snake bound
//
int collision(segment_t *segment, segment_t *snake, int width, int height) {
	int i;
	
	// wall dection
	if(segment->x == 0 || segment->y == 0)
		return true;
	
	if(segment->x == width - 1 || segment->y == height - 1)
		return true;
	
	// snake detection
	for(i = 0; !last(snake + i + 1); i++)
		if(snake[i].x == segment->x && snake[i].y == segment->y)
			return true;
	
	return false;
}

//
// add random food on game
//
void supply(char *screen, segment_t *snake, segment_t *food, int width, int height) {
	int index;
	
	food->x = (rand() % (width - 1)) + 1;
	food->y = (rand() % (height - 1)) + 1;
	
	while(collision(food, snake, width, height)) {
		food->x += 1;
		
		if(food->x == width - 1)
			food->x = 1;
			
		food->y += 1;
		if(food->y == height - 1)
			food->y = 1;
	}
	
	index = (food->y * width) + food->x;
	screen[index] = '@';
}

//
// moving snake on memory
//
int step(char *screen, segment_t *snake, segment_t *food, int width, int height) {
	int deltax = 0, deltay = 0;
	int prevx, prevy, tempx, tempy;
	int i, size;
	
	switch(mouvement) {
		case KEY_UP:
			deltay = -1;
			break;
		
		case KEY_DOWN:
			deltay = 1;
			break;
		
		case KEY_LEFT:
			deltax = -1;
			break;
		
		case KEY_RIGHT:
			deltax = 1;
			break;
	}
	
	size = width * height;
	for(i = 0; i < size; i++)
		if(last(&snake[i]))
			break;
	
	// new snake head
	i--;
	prevx = snake[i].x;
	prevy = snake[i].y;
	snake[i].x += deltax;
	snake[i].y += deltay;
	
	// head collision
	if(collision(&snake[i], snake, width, height))
		return 0;
	
	// food eating: if detected, growing up
	if(food->x == snake[i].x && food->y == snake[i].y) {
		memcpy((snake + i + 1), (snake + i), sizeof(segment_t));
		supply(screen, snake, food, width, height);
	}
	
	// moving snake body
	for(--i; i >= 0; i--) {
		tempx = snake[i].x;
		tempy = snake[i].y;
		
		snake[i].x = prevx;
		snake[i].y = prevy;
		
		prevx = tempx;
		prevy = tempy;
	}
	
	return 1;
}

//
// redraw the whole screen
//
void update(WINDOW *game, segment_t *snake, char *screen, int width, int height) {
	int i, j, k, size;
	
	// reset snake on screen
	size = width * height;
	for(i = 0; i < size; i++)
		if(screen[i] == '*')
			screen[i] = ' ';
	
	// drawing snake position on screen
	for(i = 0; i < size; i++) {
		// end of snake
		if(last(snake + i))
			break;
		
		// updating snake position
		j = (width * snake[i].y) + snake[i].x;
		screen[j] = '*';
	}
	
	// reset cursor position
	wmove(game, 0, 0);
	k = 0;
	
	for(i = 0; i < height; i++)
		for(j = 0; j < width; j++)
			wprintw(game, "%c", screen[k++]);
}

//
// initialize a default game
//
void initializer(char *screen, segment_t *snake, segment_t *food, int width, int height) {
	int i, index;
	
	// initializing empty screen
	memset(screen, ' ', width * height);
	
	// top borders: intializing the begin
	for(i = 0; i < width; i++)
		screen[i] = '#';
	
	// sides borders
	for(i = 1; i < height; i++) {
		index = i * width;
		screen[index] = '#';
		
		index += width - 1;
		screen[index] = '#';
	}
	
	// bottom borders: starting from the end and rewind
	index = (width * height) - 1;
	
	for(i = 0; i < width; i++)
		screen[index - i] = '#';
	
	// default snake position:
	memset(snake, 0x00, sizeof(segment_t) * width * height);
	
	for(i = 0; i < 5; i++) {
		snake[i].x = (width / 4) + i;
		snake[i].y = height / 2;
	}
	
	// default snake direction
	mouvement = KEY_RIGHT;
	
	// first food position
	supply(screen, snake, food, width, height);
}

int main(void) {
	int input, width, height;
	char *screen = NULL;
	segment_t *snake = NULL;
	segment_t *food = NULL;
	WINDOW *game;
	
	srand(time(NULL));
	
	// initializing ncurses
	game = initscr();
	
	clear();
	noecho();
	cbreak();
	timeout(0);
	keypad(game, TRUE);
	
	// grabbing screen size
	getmaxyx(game, height, width);
	
	// building game memory area
	if(!(screen = malloc(sizeof(char) * height * width)))
		diep("[-] screen: malloc");
	
	if(!(snake = malloc(sizeof(segment_t) * height * width)))
		diep("[-] snake: malloc");
	
	if(!(food = malloc(sizeof(segment_t))))
		diep("[-] food: malloc");
	
	// initializing game screen
	initializer(screen, snake, food, width, height);
	
	while(1) {
		// checking for user input
		if((input = wgetch(game)) != -1)
			direction(input);
		
		// next step
		if(!step(screen, snake, food, width, height))
			initializer(screen, snake, food, width, height);
		
		// updating screen
		update(game, snake, screen, width, height);
		wrefresh(game);
		
		// waiting next cycle
		usleep(80000);
	}
	
	
	endwin();
	
	return 0;	
}
