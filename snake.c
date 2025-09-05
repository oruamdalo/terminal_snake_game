#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>

#define BODY  'O'
#define DEAD  'X'
#define FRUIT '@'

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

#define ROWS 20
#define COLS 20
#define SIZE (ROWS * COLS)

typedef struct SnakeNode{
	int x;
	int y;
	struct SnakeNode *next;
} SnakeNode;

typedef struct{
	SnakeNode *head;
	SnakeNode *tail;
	int apple_x;
	int apple_y;
	int direction;
	int length;
	char board[SIZE];
	int running;
} GameState;

int get_cell_index(int x, int y){
	x = (x + COLS) % COLS;
	y = (y + ROWS) % ROWS;
	return y * COLS + x;
}

void init_board(char *board){
	for (int x=0; x<ROWS; x++){
		for (int y=0; y<COLS; y++){
			board[get_cell_index(y, x)] = '.';
		}
	}
}

void print_board(char *board){
	printf("\x1b[3J\x1b[H\x1b[2J");
	for (int x=0; x<ROWS; x++){
		for (int y=0; y<COLS; y++){
			printf("%c", board[get_cell_index(y, x)]);
		}
		printf("\n");
	}
}

void update_apple_position(GameState *game){
	do{
		game->apple_x = rand() % COLS;	
		game->apple_y = rand() % ROWS;	
	}while (game->board[get_cell_index(game->apple_x, game->apple_y)] == BODY);
}

int get_direction(char input){
	switch(input){
		case 'n': return WEST;
		case 'o': return EAST;
		case 'i': return NORTH;
		case 'e': return SOUTH;
	}
	return -1;
}

/*
 * 0 -> NORTH,
 * 1 -> EAST,
 * 2 -> SOUTH,
 * 3 -> WEST,
 * */
void update_game_state(GameState *game, int direction){ 

	SnakeNode *new_head = (SnakeNode*) malloc(sizeof(SnakeNode)); // = *(state->head);
	if (!new_head){
		printf("Error on SnakeNode malloc\n");
		exit(1);
	}
	new_head->x = game->head->x; 
	new_head->y = game->head->y; 
	
	// prevent direction update is direction is invalid
	if (direction != -1 && 
		!((game->direction == 0 && direction == 2) || // North vs South
		  (game->direction == 2 && direction == 0) || // South vs North  
		  (game->direction == 1 && direction == 3) || // East vs West
		  (game->direction == 3 && direction == 1))) { // West vs East
		game->direction = direction;
	}

	// only update direction if it's different
	switch(game->direction){
		case 0:
			new_head->y -= 1;
			break;
		case 1:
			new_head->x += 1;
			break;
		case 2:
			new_head->y += 1;
			break;
		case 3:
			new_head->x -= 1;
			break;
	}		
	
	new_head->x = (new_head->x + COLS) % COLS;
	new_head->y = (new_head->y + ROWS) % ROWS;

	// you died
	if (game->board[get_cell_index(new_head->x, new_head->y)] == BODY){
		game->running = 0;

		return;
	}

	// I think I could shrink this code
	int ate_apple = (new_head->x == game->apple_x && new_head->y == game->apple_y);
	if (ate_apple){
		game->length++;
		update_apple_position(game);
	}
	if (game->length == 1){
		new_head->next = NULL;
		game->head = new_head;
		game->tail = new_head;	
	}else if (game->length == 2){
		game->tail = game->head;
		game->tail->next = NULL;
		new_head->next = game->tail;
		game->head = new_head;
	}else {
		new_head->next = game->head;
		game->head = new_head;

		if (!ate_apple){	
			SnakeNode *current = game->head;	
			while (current->next != NULL && current->next->next != NULL){
				current = current->next;
			}
			// if we are here, are are the second-to-last element. next is tail
			free(current->next);
			current->next = NULL;
			game->tail = current;	
		}
	}
}


void draw_board(char *board, GameState *game){
    printf("\x1b[3J\x1b[H\x1b[2J");
	
	init_board(board);
	board[get_cell_index(game->apple_x, game->apple_y)] = FRUIT;

	SnakeNode *current = game->head;

	board[get_cell_index(current->x, current->y)] = (game->running) ? BODY : DEAD;
	current = current->next;
	while (current != NULL){
		board[get_cell_index(current->x, current->y)] =  BODY;
		current = current->next;
	}
}

void game_stop(GameState *state){
	SnakeNode *node = state->head, *tmp;
	while (node != NULL){
		tmp = node;
		node = node->next;
		free(tmp);
	}
}

int main(void){
	static struct termios oldt, newt;
	tcgetattr( STDIN_FILENO, &oldt);

    /*now the settings will be copied*/
    newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO); 
	newt.c_cc[VMIN] = 0;   // Return immediately even if 0 characters are available, prevent blocking
	newt.c_cc[VTIME] = 0;  
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	/* init seed randomizer for apple position */
	srand(time(NULL));
	
	int direction = 0;

	GameState game;

	SnakeNode *node = (SnakeNode*) malloc(sizeof(SnakeNode));
	if (!node){
		printf("Error on SnakeNode malloc\n");
		exit(1);
	}
	node->x = 10;
	node->y = 10;
	node->next = NULL;

	game.head = node;
	game.tail = node;
	game.length = 1;
	game.direction = direction;
	
	update_apple_position(&game);

	game.running = 1;

	while (game.running){

		char input;		
		if (read(STDIN_FILENO, &input, 1) > 0){
			if (input == 'q'){
				game.running = 0;
			}else{
				direction = get_direction(input);
				if (direction == -1)
					direction = game.direction;
			}
		}

		update_game_state(&game, direction);
		draw_board(game.board, &game);
		print_board(game.board);
		usleep(150000);

	}
	
	
	game_stop(&game);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return 0;
}
