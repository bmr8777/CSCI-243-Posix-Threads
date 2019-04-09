/// Program: thread.c
///--------------------
/// Program that contains the attack and defense threads
///
/// @author Brennan Reed

#define _DEFAULT_SOURCE
#define MAX_SPEED_DELAY 500000
#include "threads.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <curses.h>
#include <pthread.h>
#include <unistd.h>

/// global variables that are set by the init_missiles function
int ground; // the ground height
int height; // the shield height
int columns; // the maximum number of columns
bool shieldLock; // whether the shield is trying to acquire the mutex lock
bool quit; // flag for defender game loop
bool game; // whether the attack is occurring
bool endless; // whether the attacker has unlimited missiles
char * defenseForce; // the name of the defender

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/// Function: initThreads
///------------------------
/// does setup work for the missile before the start of the game
///
/// @param groundHeight the height of the ground / highest possible missile row
/// @param buildingHeight the height of the defense shield
/// @param maxColumn the maximum column value displayed in the curses window
/// @param defense the name of the defender
/// @param endlessAttack whether the attacker has an infinite number of missiles

void initThreads(int groundHeight, int buildingHeight, int maxColumn, char * defense, bool endlessAttack){
	ground = groundHeight;
	height = groundHeight - buildingHeight - 2;
	columns = maxColumn;
	game = true;
	endless = endlessAttack;
	shieldLock = false;
	quit = false;
	defenseForce = defense;
}

/// Function: createMissile
///-------------------------
/// Creates a new missile
///
/// @param column the column value for the missile
/// @param delay the amount of time the missile waits before starting to fall
/// @return Missile pointer to the dynamically allocated missile object

Missile * createMissile(int column, int delay){
	Missile * new = malloc(sizeof(struct Missile_S));
	if (new != 0){
		new->height = 6;
		new->column = column;
		new->graphic = '|';
		new->exploded = false;
		new->delay = delay;
	}
	return new;
}

/// Function: resetMissile
///--------------------------
/// Makes a missile object ready to fall again
///
/// @param missile pointer to the missile object being reset

void resetMissile (Missile * missile){
	assert(missile != NULL);
	missile->height = 6;
	missile->column = rand() % columns + 1;
	missile->exploded = false;
}

/// Function: createShield
///------------------------
/// Create a new shield.
///
/// @return Shield pointer to a dynamically allocated Shield object

Shield * createShield( int column ){
        Shield * new = malloc(sizeof(struct Shield_S));
        if (new != 0){
                new->row = height;
                new->column = column;
                char * graphic = "#####";
                new->graphic = graphic;
        }
        return new;
}

/// Function: destroyMissile
///---------------------------
/// De-allocates all dynamic memory for a specified missile object
///
/// @param missile pointer to the missile object being freed

void destroyMissile(Missile * missile){
	assert(missile != NULL);
	free(missile);
}

/// Function: destroyShield 
///--------------------------
/// Destroy all dynamically allocated storage for a shield.
///
/// @param shield the object to be de-allocated

void destroyShield( Shield *shield ){
        assert(shield != NULL);
        free(shield);
}

/// Function: eraseMissile
///------------------------
/// Erases the missile object graphic from the curses window
///
/// @param missile pointer to the missile being erases

void eraseMissile(Missile * missile){
	mvaddch(missile->height, missile->column, ' ');
}

/// Function: eraseShield
///-----------------------
/// Erases the shield object graphic from the curses window
///
/// @param shield pointer to the shield being erased

void eraseShield(Shield * shield){
        move(shield->row, shield->column);
        clrtoeol();
}

/// Function: drawMissile
///------------------------
/// Adds the graphical representation of a missile object to the curses window
///
/// @param missile pointer to the missile object being drawn

void drawMissile(Missile * missile){
	mvaddch(missile->height, missile->column, missile->graphic);
}

/// Function: drawShield
///-----------------------
/// Adds the shields graphical representation to the curses window
///
/// @param shield pointer to the shield being drawn

void drawShield(Shield * shield){
        move(shield->row, shield->column);
        addstr(shield->graphic);
}

/// Function: explode
///-------------------
/// Updates the missile object, and curses window when the missile hit something
///
/// @param missile pointer to the missile object that exploded

void explode(Missile * missile){
	missile->exploded = true;
	mvaddch(missile->height, missile->column, '?');
	mvaddch(missile->height + 1, missile->column, '*');
}

/// Function: advance
///-------------------
/// Moves a missile object one row down, and updates the object and curses window accordingly
///
/// @param missile pointer to the missile object being advanced

void advance(Missile * missile){
	while(shieldLock == true) // gives the shield priority to the lock
		usleep(10000); 
	pthread_mutex_lock(&lock);
	eraseMissile(missile);
	char next = mvinch(missile->height + 1, missile->column);

	if (next == '_' || next == '|'){ //hits a building
		missile->height++;
		explode(missile);
	} else if (next == '#') // hits the shield
		explode(missile);
	else if (next == '?'){ // hits a previous missile
		if (missile->height + 2 == height || missile->height + 1 == ground){ // hits a previous missile that hit the shield or the ground
			missile->height++;
			explode(missile);
		} else { // hits the building
			missile->height++;
			eraseMissile(missile);
			missile->height++;
			explode(missile);
		}
	} else { // continues to fall
		missile->height++;
	}
	if (missile->height >= ground)
		explode(missile);
	if (missile->exploded == false)
		drawMissile(missile);
	refresh();
	pthread_mutex_unlock(&lock);
}

/// Function: advanceShield
///-------------------------
/// Attempts to move the shield and update the curses window
///
/// @param shield pointer to the shield object being moved
/// @param left true == move left, false == move right

void advanceShield(Shield * shield, bool left){
	shieldLock = true;
        pthread_mutex_lock(&lock);
        eraseShield(shield);
        if (left == true){
                if (shield->column > 0)
                        shield->column--;
                drawShield(shield);
        } else {
                if (shield->column < columns)
                        shield->column++;
                drawShield(shield);
        }
        refresh();
        pthread_mutex_unlock(&lock);
	shieldLock = false;
}

/// Function: endGame
///-------------------
/// Informs the shield thread that the game has ended

void endGame(){
        game = false;
}

/// Function: run
///---------------
/// Main method for a missile thread instance
///
/// @param missile Missile object declared as void* for pthread operability
/// @return void pointer to status. A NULL represents success
/// @pre missile cannot be NULL

void *run( void *missile){
        assert(missile != NULL); // missile cannot be NULL
        long delay;
        Missile* missileData = missile;

	usleep(missileData->delay); // increments the missiles

	while (missileData->exploded == false){
		delay = rand() % (MAX_SPEED_DELAY + 1);
		usleep(delay);
		advance(missileData);
	}
	pthread_exit(NULL);
}

/// Function: runShield
/// -------------------
///  The 'main method' for a shield thread instance.
///
/// @param shield Shield object declared as void* for pthread operability
/// @return void pointer to status. A NULL represents success.
/// @pre shield cannot be NULL.

void *runShield( void *shield ){
        assert(shield != NULL);
	int ch;
        Shield * shieldData = shield;
	pthread_mutex_lock(&lock);
	if (endless == true) // prompts the user with the correct exit instructions
		mvprintw(0, 6, "%s", "Endless Attack Mode. Enter control-C to quit.");
	else
		mvprintw(0, 6, "%s", "Enter '?' to quit at end of attack, or control-C.");
	drawShield(shieldData); // displays the shield on the curses window
	refresh();
	pthread_mutex_unlock(&lock);
        while (game == true || quit == false){
                ch = getch();
                switch(ch){
                        case KEY_LEFT:
                                advanceShield(shieldData, true);
                                break;
                        case KEY_RIGHT:
                                advanceShield(shieldData, false);
                                break;
			case '?': // the user entered '?'
				quit = true;
				break;
                        default:
                                break;
                }
        }
	mvprintw(5, 6, "The %s defense has ended.", defenseForce);
        mvprintw(6, 6, "%s", "hit enter to close...");
	while (ch != 13 && ch != 10){
		ch = getch();
	}
        pthread_exit(NULL);
}
