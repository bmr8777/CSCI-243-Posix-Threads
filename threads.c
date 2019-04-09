/// Program: threads.c
///---------------------
/// C-based war game inspired by the arcade games Space Invaders and Galaga
///
/// @author Brennan Reed

#define _DEFAULT_SOURCE
#define MAX(a,b) 	((a < b) ? (b) : (a))
#define MIN(a,b)	((a > b) ? (b) : (a))
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <curses.h>
#include <pthread.h>
#include <unistd.h>
#include "threads.h"

/// Global variables used by the game
char * defenseForce;
char * attackForce;
size_t missileCount;
int maxHeight;
int tallestBuilding;
int maxWidth;
int * heights;
size_t arraySize;
size_t column;
volatile bool attack;

/// Function: gameBuilder
///-----------------------
/// Takes the provided config-file and attempts to build the game
///
/// @param fp file pointer to the configuration file
/// @returns true/false whether the game was build successfully

bool gameBuilder(FILE * fp){
	
	size_t len = 0;
	char * line = NULL;
	ssize_t nread;
	size_t lineCount = 0;
	char * token;
	size_t height;
	bool valid = true;
	heights = malloc(arraySize * sizeof(int));

	while((nread = getline(&line, &len, fp)) > 0 && valid == true){
		if (line[0] == '#')
			continue;
		switch (lineCount){
			case 0:
				if (nread <= 80){
					defenseForce = malloc(nread * sizeof(char));
					strncpy(defenseForce, line, nread - 1);
					lineCount++;
				} else {
					fprintf(stderr, "%s", "Error: missing defender name.\n");
					if (line != NULL)
						free(line);
					return false;
				}
				break;
			case 1:
				if (nread <= 80){
					attackForce = malloc(nread * sizeof(char));
					strncpy(attackForce, line, nread - 1);
					lineCount++;
				} else {
					fprintf(stderr, "%s", "Error: missing attacker name.\n");
					if (line != NULL)
						free(line);
					return false;
				}
				break;
			case 2:
				for (ssize_t i = 0; i < nread; i++){
					if (isdigit(line[i]) == 0 && isspace(line[i]) == 0){
						if (line != NULL)
                                                	free(line);
						fprintf(stderr, "%s", "Error: missing missile specification.\n");
                                        	return false;
					}
				}
				if (valid == true){
					missileCount = atoi(line);
					lineCount++;
				}
				break;
			default:
				token = strtok(line, " ");
				while (token != NULL){
					for (size_t i = 0; i < strlen(token); i++){
						if (isdigit(token[i]) == 0 && isspace(token[i]) == 0){
							if (line != NULL)
                                                		free(line);
							fprintf(stderr, "%s", "Error: missing city layout.\n");
                                        		return false;
						}
					}
					height = atoi(token);
					if (column == arraySize){ // reallocates array when it's full
						arraySize *= 2;
						heights = realloc(heights, arraySize * sizeof(int));
					}
					heights[column] = height;
					column++;
					token = strtok(NULL, " ");
				}
				break;
		}
	}
	if (line != NULL)
		free(line);
	switch (lineCount){
		case 0:
			fprintf(stderr, "%s", "Error: missing defender name.\n");
			return false;
		case 1:
			fprintf(stderr, "%s", "Error: missing attacker name.\n");
                        return false;
		case 2:
			fprintf(stderr, "%s", "Error: missing missile specification.\n");
                        return false;
		default:
			return valid;
	}	
}

/// Function: restartAttack
///-------------------------
/// Resets all missile objects to continue attack for unlimited missile loop
///
/// @param missiles array containing all created missile objects

void restartAttack(Missile ** missiles){
	assert (missiles != NULL);
	for (size_t i = 0; i < missileCount; i++)
		resetMissile(missiles[i]);
}

/// Function: main
///----------------
/// Controls the main logic of the program
///
/// @param argc number of commandline arguments
/// @param argv array of commandline argument strings
/// @return 0 if success, else EXIT_FAILURE

int main(int argc, char* argv[]){
	missileCount = 0; // initializes missileCount variable
	arraySize = 256;
	tallestBuilding = 0;
	char * usage = "./threads config-file\n";
	attack = true;
	int height, previousHeight = 2;
	Shield * shield;
	Missile ** missiles;
	pthread_t * threads;
	bool valid = true, endless;
	int delay = 0;
	srand(time(NULL)); //  Seeding random number generator
	if (argc != 2){
		fprintf(stderr, "%s", usage);
		return EXIT_FAILURE;
	}
	char * fileName = argv[1];
	FILE * fp = fopen(fileName, "r");
	if (fp == NULL){
		fprintf(stderr, "%s", "Error: specified config-file not found.\n");
		return EXIT_FAILURE;
	}
	valid = gameBuilder(fp); // creates the game
	fclose(fp); // closes the config-file
	if (valid == false){
		if (heights != NULL)
			free(heights);
		return EXIT_FAILURE;
	}
	initscr(); // initializes stdscr, a curses global variable
	cbreak();
	noecho(); // disables typed characters appearing in terminal
	keypad(stdscr, TRUE);
	maxWidth = getmaxx(stdscr);
	maxHeight = getmaxy(stdscr);
	for (size_t i = 0; i < column; i++){ // displays the specified city in the terminal
		height = heights[i];
		if (height > tallestBuilding)
			tallestBuilding = height;
		if (height != previousHeight){
			for (int x = 2; x < MAX(height, previousHeight); x++){
				if (i > 0)
					mvaddch(maxHeight - x, i, '|');
			}
		} else
			mvaddch(maxHeight - height, i, '_');
		previousHeight = height;
	}
	for (int i = MIN((int)column, maxWidth) - 1; i <= MAX((int)column, maxWidth); i++)
		mvaddch(maxHeight - 2, i, '_');
	refresh();
	getch();
	if (missileCount == 0)
		endless = true;
	else
		endless = false;
	initThreads(maxHeight - 2, tallestBuilding, column, defenseForce, endless);

	shield = createShield((column / 2) + 3);
	pthread_t shieldThread;
	if (missileCount == 0){
		missileCount = 20;
		missiles = malloc(missileCount * sizeof(struct Missile_S));
                threads = calloc(missileCount, sizeof(pthread_t));
		for (size_t i = 0; i < missileCount; i++){
			int x = rand() % (MIN((int)column, maxWidth) + 1);
			missiles[i] = createMissile(x, delay);
			delay += 1000000;
		}
		pthread_create(&shieldThread, NULL, &runShield, shield);
		
		while (attack == true){
			if (threads != NULL)
				free(threads);
			threads = calloc(missileCount, sizeof(pthread_t));
			restartAttack(missiles);
			for (size_t i = 0; i < missileCount; i++)
                       		pthread_create(&threads[i], NULL, &run, missiles[i]);
                	for (size_t i = 0; i < missileCount; i++)
				pthread_join(threads[i], NULL); // waits for threads to finish
		}
	} else {
		missiles = malloc(missileCount * sizeof(struct Missile_S));
		threads = calloc(missileCount, sizeof(pthread_t));
		for (size_t i = 0; i < missileCount; i++){
			int x = rand() % (MIN((int)column, maxWidth) + 1); // randomly generates the column for each missile
			missiles[i] = createMissile(x, delay);
			delay += 1000000; // figure out appropriate value
		}

		pthread_create(&shieldThread, NULL, &runShield, shield);
		for (size_t i = 0; i < missileCount; i++)
			pthread_create(&threads[i], NULL, &run, missiles[i]);
		for (size_t i = 0; i < missileCount; i++)
			pthread_join(threads[i], NULL); // waits for threads to finish
		mvprintw(3,6, "The %s attack has ended.", attackForce);
		refresh();
		endGame(); // informs the shield that they attackers turn has ended
		pthread_join(shieldThread, NULL); // waits for the shieldThread to finish
	}

	for (size_t i = 0; i < missileCount; i++) // free all dynamically allocated memory for missile objects
                        destroyMissile(missiles[i]);
	if (shield != NULL)
		destroyShield(shield);
	if (missiles != NULL)
		free(missiles);
	if (threads != NULL)
		free(threads);
	if (attackForce != NULL)
		free(attackForce);
	if (defenseForce != NULL)
		free(defenseForce);
	if (heights != NULL)
		free(heights);

	endwin(); // terminates the curses environment
	return 0;
}

