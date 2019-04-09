/// threads.h - header file for shield and missile threads
/// 
/// @author Brennan Reed
/// @date Fri Nov 09 15:33:50 EDT 2018
/// 
/// This is the interface for the threads game. 
/// Contains the interfaces for the defense shield, and missile threads.

#ifndef _THREADS_H
#define _THREADS_H
#include <stdbool.h>

/// Shield_S structure represents a missile's row, column and display graphic.

typedef struct Shield_S {

    int row;       ///< vertical row of the shield

    int column;  ///< left-most column of the shield

    char *graphic; ///< graphic is the character array representation of the shield

} Shield;

/// initThreads does setup work for the defense shield before the start of the game
///
/// @param groundHeight the height of the ground / highest possible missile row 
/// @param buildingHeight the height of the highest building in the city
/// @param maxColumn the maximum column value displayed in the curses environment
/// @param defense the name of the defender
/// @param endlessAttack whether the attacker has infinite number of missiles

void initThreads( int groundHeight, int buildingHeight, int maxColumn, char * defense, bool endlessAttack);

/// createShield- Create a new shield.
///
/// @return Shield pointer to a dynamically allocated Shield object

Shield * createShield( int column );

/// destroyShield - Destroy all dynamically allocated storage for a shield.
///
/// @param shield the object to be de-allocated

void destroyShield( Shield *shield );

/// This function is the 'main method' for a shield thread instance.
///
/// @param shield Shield object declared as void* for pthread operability
/// @return void pointer to status. A NULL represents success.
/// @pre shield cannot be NULL.

void *runShield( void *shield );

/// endGame - Informs the shield Thread that the game has ended

void endGame();

/// Missile_S structure represents a missile's row, column and display graphic.

typedef struct Missile_S {

    int height;       ///< vertical row of a missile

    int column;  ///< column of the missile

    char graphic; ///< graphic is the character representation of the missile

    int delay; /// delay is the amount of time the missile waits before starting to fall

    bool exploded; /// exploded whether the missile is still falling
} Missile;

/// createMissile- Create a new missile.
///
/// @param column the column value for the missile
/// @param delay the amount of time the missile waits before starting to fall
/// @return Missile pointer to a dynamically allocated Missile object

Missile * createMissile( int column , int delay);

/// destroyMissile - Destroy all dynamically allocated storage for a missile.
///
/// @param missile the object to be de-allocated

void destroyMissile( Missile *missile );

/// resetMissile - Prepares a Missile object for reuse
//
// @param missile pointer to the missile object being reused

void resetMissile(Missile * missile);

/// This function is the 'main method' for a missile thread instance.
///
/// @param missile Missile  object declared as void* for pthread operability
/// @return void pointer to status. A NULL represents success.
/// @pre missile cannot be NULL.

void *run( void *missile );

#endif
