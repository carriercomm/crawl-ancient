/*
 *  File:       religion.cc
 *  Summary:    Misc religion related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *               <1>     -/--/--        LRH             Created
 */


#ifndef RELIGION_H
#define RELIGION_H


// last updated 03jun2000 {dlb}
/* ***********************************************************************
 * called from: ouch - religion
 * *********************************************************************** */
bool simple_god_message(char which_deity, bool colour_message, const char *event);


// last updated 03jun2000 {dlb}
/* ***********************************************************************
 * called from: chardump - overmap - religion
 * *********************************************************************** */
char *god_name(int which_god);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: religion - spell
 * *********************************************************************** */
void dec_penance(int val);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: acr - decks - fight - player - religion - spell
 * *********************************************************************** */
void Xom_acts(bool niceness, int sever, bool force_sever);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: beam - decks - fight - religion
 * *********************************************************************** */
void done_good(char thing_done, int pgain);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: ability - religion
 * *********************************************************************** */
void excommunication(void);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: acr - religion - spell
 * *********************************************************************** */
void gain_piety(char pgn);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: spell - religion
 * *********************************************************************** */
void god_speaks(int god);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: ability - religion
 * *********************************************************************** */
void lose_piety(char pgn);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: acr - beam - fight - it_use2 - item_use - religion - spell -
 *              spellbook - spells4
 * *********************************************************************** */
void naughty(char type_naughty, int naughtiness);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: food
 * *********************************************************************** */
void offer_corpse(int corpse);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: acr
 * *********************************************************************** */
void pray(void);


// last updated 24may2000 {dlb}
/* ***********************************************************************
 * called from: items
 * *********************************************************************** */
void handle_god_time(void);


#endif
