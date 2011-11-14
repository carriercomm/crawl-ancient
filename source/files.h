/*
 *  File:       files.cc
 *  Summary:    Functions used to save and load levels/games.
 *  Written by: Linley Henzell and Alexey Guzeev
 *
 *  Change History (most recent first):
 *
 *               <1>     -/--/--        LRH             Created
 */


#ifndef FILES_H
#define FILES_H

#include "FixAry.h"


// referenced in files - newgame - ouch - overmap:
#define MAX_LEVELS 50
// referenced in files - newgame - ouch - overmap:
#define MAX_BRANCHES 30         // there must be a way this can be extracted from other data


// referenced in files - newgame - ouch:
extern FixedArray<bool, MAX_LEVELS, MAX_BRANCHES> tmp_file_pairs;


// last updated 12may2000 {dlb}
/* ***********************************************************************
 * called from: acr - misc
 * *********************************************************************** */
void load(unsigned char stair_taken, bool moving_level,
          bool was_a_labyrinth, char old_level, bool want_followers,
          bool just_made_new_lev, char where_were_you2);


// last updated 12may2000 {dlb}
/* ***********************************************************************
 * called from: acr - libmac - misc
 * *********************************************************************** */
void save_game(bool leave_game);


// last updated 12may2000 {dlb}
/* ***********************************************************************
 * called from: acr
 * *********************************************************************** */
void restore_game(void);


// last updated 12may2000 {dlb}
/* ***********************************************************************
 * called from: ouch
 * *********************************************************************** */
void save_ghost(void);


#endif