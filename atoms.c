#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "atoms.h"

void help();
void quit();
void display();
void start(char *p, char *wh, char *ht, char *dummy);
void place(char *run, char *rise, char *dummy);
void undo();
void stat();
void save(char *filename);
void load(char *filename);
void playfrom(char *turn, char *filename);
void startHelper();
void placeHelper(int x, int y);
void pop(int x, int y);
void popHelp(int x, int y);
void whosTurn();
void checkWin();
void checkLoss();
void prepareList();
void addMove(int x, int y);
void undoHelper(int x, int y);

int gameStatus = 0; //0 - not started, 1 - started
grid_t **gameGrid;
player_t **player; //player[0] = red... etc...
int width;
int height;
int playerTurn = 0; //keeps track of who's turn 0 = red, 1 = blue... etc...
int playerCount = 0; //number of players in the game
int totalMoves = 0;
move_t *head = NULL;
move_t *move;
move_t *newMove;
char saveName[50]; //This is probably not a good idea...
int loaded = 0; //0 - not loaded, 1 - loaded, 2 - loaded

int main(int argc, char** argv) {
	char cmd[50]; 	//Generous buffer size
	char *token[5];	//Should make this dynamic

	while(fgets(cmd, 50, stdin) != NULL) {
		token[0] = strtok(cmd, " ");
		token[1] = strtok(NULL, " ");
		token[2] = strtok(NULL, " ");
		token[3] = strtok(NULL, " ");
		token[4] = strtok(NULL, " ");

		if(strcmp(token[0], "HELP\n") == 0) {
			help();
		}
		else if(strcmp(token[0], "QUIT\n") == 0) {
			quit();
		}
		else if(strcmp(token[0], "DISPLAY\n") == 0) {
			display();
		}
		else if(strcmp(token[0], "START") == 0) {
			start(token[1], token[2], token[3], token[4]);
		}
		else if(strcmp(token[0], "PLACE") == 0) {
			place(token[1], token[2], token[3]);
		}
		else if(strcmp(token[0], "UNDO\n") == 0) {
			undo();
		}
		else if(strcmp(token[0], "STAT\n") == 0) {
			stat();
		}
		else if(strcmp(token[0], "SAVE") == 0) {
			save(token[1]);
		}
		else if(strcmp(token[0], "LOAD") == 0) {
			load(token[1]);
		}
		else if(strcmp(token[0], "PLAYFROM") == 0) {
			playfrom(token[1], saveName);
		}
		else {
			printf("Invalid Command\n");
		}
	}
}

void help() {
	if(gameStatus == 1 && loaded == 1) {
		printf("Invalid Command\n\n");
		return;
	}
	printf("\nHELP displays this help message\n");
	printf("QUIT quits the current game\n\n");
	printf("DISPLAY draws the game board in terminal\n");
	printf("START <number of players> <width> <height> starts the game\n");
	printf("PLACE <x> <y> places an atom in a grid space\n");
	printf("UNDO undoes the last move made\n");
	printf("STAT displays game statistics\n\n");
	printf("SAVE <filename> saves the state of the game\n");
	printf("LOAD <filename> loads a save file\n");
	printf("PLAYFROM <turn> plays from n steps into the game\n\n");
	return;
}

void quit() {
	printf("Bye!\n");
	exit(0);
}

void display() {
	if(gameStatus == 1 && loaded == 1) {
		printf("Invalid Command\n\n");
		return;
	}

	printf("\n+");
	for(int i = 0; i < width * 3 - 1; i++) {
		printf("-");
	}
	printf("+\n");
	for(int i = 0; i < height; i++) {
		for(int j = 0; j < width; j++) {
			printf("|");
			if(gameGrid[i][j].atom_count > 0) {
				printf("%s%d", gameGrid[i][j].owner->colour, gameGrid[i][j].atom_count);
			}
			else {
				printf("  ");
			}
		}
		printf("|\n");
	}
	printf("+");
	for(int i = 0; i < width * 3 - 1; i++) {
		printf("-");
	}
	printf("+\n\n");
	return;
}

void start(char *p, char *wh, char *ht, char *dummy) {
	//Check the number of arguments
	if(dummy != NULL) {
		printf("Too Many Arguments\n\n");
		return;
	}
	if(ht == NULL) {
		printf("Missing Argument\n\n");
		return;
	}

	//Check if arguments are integers
	if(!isdigit(*p) || !isdigit(*wh) || !isdigit(*ht)) {
		printf("Invalid Command Argument\n\n");
		return;
	}

	//Set global player count, width, height
	playerCount = atoi(p);
	width = atoi(wh);
	height = atoi(ht);

	if(width < 0 || height < 0) {
		printf("Invalid Command Argument\n\n");
		return;
	}

	//Check players(n) >= 2 && <=6
	if(playerCount < MIN_PLAYERS || playerCount > MAX_PLAYERS) {
		printf("Invalid command argument\n\n");
		return;
	}

	//Check grid size vs # of players
	if(width * height < playerCount) {
		printf("Cannot Start Game\n\n");
		return;
	}

	//Don't allow start if game has already started
	if(gameStatus == 1) {
		printf("Invalid Command\n\n");
		return;
	}

	if(gameStatus == 1 && loaded == 1) {
		printf("Invalid Command\n\n");
		return;
	}
	startHelper();
	printf("Game Ready\n");
	whosTurn();
	return;
}

void startHelper() {
	//Allocate memory for the gamegrid
	gameGrid = malloc(width * sizeof(grid_t));
	for(int i = 0; i < width; i++) {
		gameGrid[i] = malloc(height * sizeof(grid_t));
	}

	//Allocate memory for players and set player attributes
	player = malloc(playerCount * sizeof(player_t));
	for (int i = 0; i < playerCount; i++) {
		player[i] = malloc(sizeof(player_t));
	}

	//We should zero out the array incase of garbage
	for(int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			gameGrid[i][j].owner = NULL;
			gameGrid[i][j].atom_count = 0;
		}
	}

	for (int i = 0; i < playerCount; i++) {
		if(i == 0) {
			player[i]->colour = "R"; //red
		}
		else if(i == 1) {
			player[i]->colour = "G"; //green
		}
		else if(i == 2) {
			player[i]->colour = "P"; //purple
		}
		else if(i == 3) {
			player[i]->colour = "B"; //blue
		}
		else if(i == 4) {
			player[i]->colour = "Y"; //yellow
		}
		else if(i == 5) {
			player[i]->colour = "W"; //white
		}
		player[i]->grids_owned = 0;
		player[i]->lost = 0; //0 - lost, 1 - not lost
	}

	prepareList(); //Linkelist of moves
	gameStatus = 1; //Set gameStatus to started
	return;
}

void place(char *run, char *rise, char *dummy) {
	if(gameStatus == 1 && loaded == 1) {
		printf("Invalid Command\n\n");
		return;
	}
	//Check length of arguments
	if(dummy != NULL) {
		printf("Invalid Coordinates\n\n");
		return;
	}

	//Check if arguments are integers
	if(!isdigit(*run) || !isdigit(*rise)) {
		printf("Invalid Coordinates\n\n");
		return;
	}

	int x = atoi(run);
	int y = atoi(rise);

	if(x < 0 || x >= width || y < 0 || y >= height) {
		printf("Invalid Coordinates\n\n");
		return;
	}

	//Player places in existing grid not owned by them
	if(gameGrid[y][x].owner != player[playerTurn] && gameGrid[y][x].owner !=NULL) {
		printf("Cannot Place Atom Here\n\n");
		return;
	}
	placeHelper(x, y);
	whosTurn(playerTurn);
	return;
}

void placeHelper(int x, int y) {
	//Player places in empty grid
	if(gameGrid[y][x].owner == NULL) {
		gameGrid[y][x].owner = player[playerTurn];
		player[playerTurn]->grids_owned+=1; //Update grids owned
		gameGrid[y][x].atom_count = 1; //Update atom count on grid
		addMove(x, y);
	}

	//Player places in existing grid
  else if(gameGrid[y][x].owner == player[playerTurn]) {
		gameGrid[y][x].atom_count+=1;
		addMove(x, y);
		pop(x, y); //Check if we need to "pop"
		checkLoss(); //Check if any player has lost after popping
		checkWin();
	}
	playerTurn++;
	totalMoves++;
	return;
}

void prepareList() {
	//Create an empty head node for move linkedlist
	head = calloc(1, sizeof(move_t));
	head->extra = NULL;
	head->parent = NULL;
	move = head;
}

void addMove(int x, int y) {
	newMove = calloc(1, sizeof(move_t));
	newMove->x = x;
	newMove->y = y;
	newMove->parent = move;	//Set to point at previous move
	move->extra = newMove;	//Set previous node's next to point at new move
	move = newMove;			//Move pointer to the new move
}

void stat() {
	if(gameStatus == 1 && loaded == 1) {
		printf("Invalid Command\n\n");
		return;
	}
	if(gameStatus == 0) {
		printf("Game Not In Progress\n\n");
		return;
	}

	for(int i = 0; i < playerCount; i++) {
		if(i == 0) {
			printf("Player Red:\n");
		}
		else if(i == 1) {
			printf("Player Green:\n");
		}
		else if(i == 2) {
			printf("Player Purple:\n");
		}
		else if(i == 3) {
			printf("Player Blue:\n");
		}
		else if(i == 4) {
			printf("Player Yellow:\n");
		}
		else {
			printf("Player White:\n");
		}
		if(player[i]->lost == 1) {
			printf("Lost\n\n");
		}
		else{
			printf("Grid Count: %d\n\n", player[i]->grids_owned);
		}
	}
	return;
}

void whosTurn() {
	//Reset back to first player
	if(playerTurn == playerCount) {
		playerTurn = 0;
	}

	//Sketchy way of skipping players who have lost
	if(player[playerTurn]->lost == 1) {
		playerTurn+=1;
	}
	if(playerTurn >= playerCount) {
		playerTurn = 0;
	}

	switch(playerTurn) {
		case 0 :
			printf("Red's Turn\n\n");
			return;
		case 1 :
			printf("Green's Turn\n\n");
			return;
		case 2 :
			printf("Purple's Turn\n\n");
			return;
		case 3 :
			printf("Blue's Turn\n\n");
			return;
		case 4 :
			printf("Yellow's Turn\n\n");
			return;
		default :
			printf("White's Turn\n\n");
			return;
		}
}

void pop(int x, int y) {
	//We need to calculate the limit at which we pop
	int limit = 0;

	if(x - 1 >=0) {
		limit+=1;
	}
	if(x + 1 < width) {
		limit+=1;
	}
	if(y-1 >= 0) {
		limit+=1;
	}
	if(y+1 < width) {
		limit+=1;
	}

	//Check if we need to pop
	if(gameGrid[y][x].atom_count != limit) {
		return;
	}

	//Empty full grid space
	gameGrid[y][x].owner = NULL;
	gameGrid[y][x].atom_count = 0;
	player[playerTurn]->grids_owned-=1;

	//Top
	if(y - 1 >= 0) {
		popHelp(x, y-1);
	}

	//Right
	if(x + 1 < width) {
		popHelp(x+1, y);
	}

	//Bottom
	if(y + 1 < height) {
		popHelp(x, y+1);
	}

	//Left
	if(x - 1 >= 0) {
		popHelp(x-1, y);
	}
	return;
}

void popHelp(int x, int y) {
	//If empty
	if(gameGrid[y][x].owner == NULL) {
		gameGrid[y][x].owner = player[playerTurn];
		gameGrid[y][x].atom_count = 1;
		player[playerTurn]->grids_owned+=1;
	}
	//If owned by player
	else if(gameGrid[y][x].owner == player[playerTurn]) {
		gameGrid[y][x].atom_count+=1;
	}
	//If owned by other player
	else if(gameGrid[y][x].owner != player[playerTurn]) {
		gameGrid[y][x].owner->grids_owned-=1;
		gameGrid[y][x].owner = player[playerTurn];
		player[playerTurn]->grids_owned+=1;
		gameGrid[y][x].atom_count+=1;
	}
	pop(x, y);
	//We should check win condition here incase of infinite recursion
	return;
}

void checkWin() {
	int win = 0;
	for (int i = 0; i < playerCount; i++) {
		if(player[i]->grids_owned > 0) {
			win+=1;
		}
	}

	if(win == 1) {
		if(playerTurn == 0) {
			printf("Red Wins!\n");
		}
		else if(playerTurn == 1) {
			printf("Green Wins!\n");
		}
		else if(playerTurn == 2) {
			printf("Purple Wins!\n");
		}
		else if(playerTurn == 3) {
			printf("Blue Wins!\n");
		}
		else if(playerTurn == 4) {
			printf("Yellow Wins!\n");
		}
		else {
			printf("White Wins!\n");
		}
		exit(0);
	}
	return;
}

void checkLoss() {
	for(int i = 0; i < playerCount; i++) {
		if(player[i]->grids_owned == 0) {
			player[i]->lost = 1;
		}
	}
	return;
}

void save(char *filename) {
	if(gameStatus == 1 && loaded == 1) {
		printf("Invalid Command\n\n");
		return;
	}
	strtok(filename, "\n");
	FILE *save_file = fopen(filename, "r");

	//Check if the save already exists
	if(save_file != NULL) {
		printf("File Already Exists\n\n");
		fclose(save_file); //Make sure we close the file
		return;
	}

	save_file = fopen(filename, "w");
	//width, height, no_players
	fwrite(&width, sizeof(uint8_t), 1, save_file);
	fwrite(&height, sizeof(uint8_t), 1, save_file);
	fwrite(&playerCount, sizeof(uint8_t), 1, save_file);

	//move data
	int padding = 0;
	move_t *movePointer = head->extra;
	if(movePointer != NULL) {
		while(movePointer != NULL) {
			fwrite(&movePointer->x, sizeof(uint8_t), 1, save_file);
			fwrite(&movePointer->y, sizeof(uint8_t), 1, save_file);
			fwrite(&padding, sizeof(uint8_t), 1, save_file);	//Need to pad Coordinates with 2 bytes of 0
			fwrite(&padding, sizeof(uint8_t), 1, save_file);
			movePointer = movePointer->extra;
		}
	}

	fclose(save_file); //Make sure we close the file
	printf("Game Saved\n\n");
	return;
}

void load(char *filename) {
	//Check if a save has already been loaded
	if(loaded == 1 || loaded == 2) {
		printf("Restart Application To Load Save\n\n");
		return;
	}

	strcpy(saveName, filename); //Save the filename for PLAYFROM
	strtok(filename, "\n");
	
	//Check if save exits
	if(access(filename, F_OK) == -1) {
		printf("Cannot Load Save\n\n");
		return;
	}

	loaded = 1;
	printf("Game Loaded\n\n");
	return;
}

void playfrom(char *turn, char *filename) {
	//Check if save has been loaded
	if(loaded == 0) {
		printf("Invalid Command\n\n");
		return;
	}

	char *ptr;
	int numberOfTurns = strtol(turn, &ptr, 10);

	//Check if turn number is valid
	if(numberOfTurns < 0) {
		printf("Invalid Turn Number\n\n");
		return;
	}

	uint8_t x;
	uint8_t y;
	uint8_t padding;

	strtok(filename, "\n");
	FILE *file = fopen(filename, "r");
	fread(&width, sizeof(uint8_t), 1, file);
	fread(&height, sizeof(uint8_t), 1, file);
	fread(&playerCount, sizeof(uint8_t), 1, file);
	startHelper();

	if(strncmp(turn, "END", 3) == 0) {
		for(;;) {
			fread(&x, sizeof(uint8_t), 1, file);
			fread(&y, sizeof(uint8_t), 1, file);
			fread(&padding, sizeof(uint8_t), 1, file);
			fread(&padding, sizeof(uint8_t), 1, file);

			//Check for EOF
			if(feof(file)) {
				break;
			}

			placeHelper(x, y);
		}
	}
	else {
		for(int i = 0; i < numberOfTurns; i++) {
			fread(&x, sizeof(uint8_t), 1, file);
			fread(&y, sizeof(uint8_t), 1, file);
			fread(&padding, sizeof(uint8_t), 1, file);
			fread(&padding, sizeof(uint8_t), 1, file);

			//Check for EOF
			if(feof(file)) {
				break;
			}

			placeHelper(x, y);
		}
	}
	fclose(file);
	loaded = 2;
	printf("Game Ready\n");
	whosTurn();
}

void undo() {
	//Don't allow undos at start of the game
	if(totalMoves == 0) {
		printf("Cannot Undo\n\n");
		return;
	}

	//This is probably a very very very bad idea
	for(int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			gameGrid[i][j].owner = NULL;
			gameGrid[i][j].atom_count = 0;
		}
	}

	for (int i = 0; i < playerCount; i++) {
		player[i]->grids_owned = 0;
		player[i]->lost = 0;
	}
	int temp = totalMoves;
	totalMoves = 0;
	playerTurn = 0;
	move_t *movePointer = head->extra;
	for(int i = 0; i < temp - 1; i++) {
		undoHelper(movePointer->x, movePointer->y);
		movePointer = movePointer->extra;
	}
	movePointer->extra = NULL;
	whosTurn();
}

void undoHelper(int x, int y) {
	if(gameGrid[y][x].owner == NULL) {
		gameGrid[y][x].owner = player[playerTurn];
		player[playerTurn]->grids_owned+=1;
		gameGrid[y][x].atom_count = 1;
	}
  	else if(gameGrid[y][x].owner == player[playerTurn]) {
		gameGrid[y][x].atom_count+=1;
		pop(x, y);
		checkLoss();
	}
	totalMoves++;
	playerTurn++;

	if(playerTurn == playerCount) {
		playerTurn = 0;
	}

	if(player[playerTurn]->lost == 1) {
		playerTurn+=1;
	}
	if(playerTurn >= playerCount) {
		playerTurn = 0;
	}
	return;
}