/*
 *  File:       monstuff.cc
 *  Summary:    Misc monster related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *               <1>     -/--/--        LRH             Created
 */


#ifndef MONSTUFF_H
#define MONSTUFF_H


// useful macro
#define SAME_ATTITUDE(x) (mons_friendly(x)?BEH_FRIENDLY:BEH_HOSTILE)

// for definition of type monsters {dlb}
#include "externs.h"
// for definition of type monsters {dlb}

/* ***********************************************************************
 * called from: monstuff beam effects fight view
 * *********************************************************************** */
void behavior_event(struct monsters *mon, int event_type, int param = 0);


/* ***********************************************************************
 * called from: fight - it_use3 - spells
 * *********************************************************************** */
bool curse_an_item(char which, char power);


/* ***********************************************************************
 * called from: fight
 * *********************************************************************** */
void monster_blink(struct monsters *monster);


/* ***********************************************************************
 * called from: spells1 spells4 monstuff
 * defaults are set up for player blink;  monster blink should call with
 * false, false
 * *********************************************************************** */
bool random_near_space( int ox, int oy, int &tx, int &ty,
    bool allow_adjacent = false, bool restrict_LOS = true);


/* ***********************************************************************
 * called from: beam - effects - fight - monstuff - mstuff2 - spells1 -
 *              spells2 - spells4
 * *********************************************************************** */
bool simple_monster_message(struct monsters *monster, const char *event,
                            int channel = MSGCH_PLAIN, int param = 0);


/* ***********************************************************************
 * called from: acr
 * *********************************************************************** */
bool swap_places(struct monsters *monster);


/* ***********************************************************************
 * called from: bang - beam - direct - fight - spells1 - spells2 - spells3
 * *********************************************************************** */
void print_wounds(struct monsters *monster);


/* ***********************************************************************
 * called from: fight
 * *********************************************************************** */
bool wounded_damaged(int wound_class);


/* ***********************************************************************
 * called from: acr
 * *********************************************************************** */
void monster(void);


/* ***********************************************************************
 * called from: acr - bang - beam - direct - dungeon - fight - files -
 *              monplace - mstuff2 - spells3 - view
 * *********************************************************************** */
unsigned char monster_habitat(int which_class);


/* ***********************************************************************
 * called from: misc
 * *********************************************************************** */
bool monster_descriptor(int which_class, unsigned char which_descriptor);


/* ***********************************************************************
 * called from: direct - item_use - spells1
 * *********************************************************************** */
bool message_current_target(void);


/* ***********************************************************************
 * called from: xxx
 * *********************************************************************** */
unsigned int monster_index(struct monsters *monster);


// last updated 08jun2000 {dlb}
/* ***********************************************************************
 * called from: bang - beam - effects - fight - monstuff - mstuff2 -
 *              spells2 - spells3 - spells4
 * *********************************************************************** */
bool hurt_monster(struct monsters *victim, int damage_dealt);


/* ***********************************************************************
 * called from: beam - fight - files - monstuff - spells1
 * *********************************************************************** */
bool heal_monster(struct monsters *patient, int health_boost, bool permit_growth);


#endif
