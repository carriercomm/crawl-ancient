/*
 *  File:       defines.h
 *  Summary:    Various definess used by Crawl.
 *  Written by: Linley Henzel
 *
 *      Abstract:       A variety of miscellaneous constant values are found here.
 *                      I think we should move the colors into an enum or something
 *                      because there are in numerical order.  But I'm too lazy to
 *                      do it myself.
 *
 *  Copyright © 1999 Brian Robinson.  // Me?  How come?
 *
 *  Change History (most recent first):
 *
 *       <3>     9/25/99        CDL             linuxlib -> liblinux
 *       <2>     6/17/99        BCR             indented and added header
 *       <1>     --/--/--       LRH             Created
 */


#ifndef DEFINES_H
#define DEFINES_H


// there's got to be a better way...
#ifdef _LIBLINUX_IMPLEMENTATION
#elif macintosh
#else

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif


// max size of inventory array {dlb}:
#define ENDOFPACK 52


// max size of monter array {dlb}:
#define MNST 200
// non-monster for mgrd[][] -- (MNST + 1) {dlb}:
#define MNG 201

// max size of item list {dlb}:
#define ITEMS 500
// non-item -- (ITEMS + 1) {dlb}
#define ING 501

// max size of cloud array {dlb}:
#define CLOUDS 30
// empty cloud -- (CLOUDS + 1) {dlb}:
#define CNG 31

// max x-bound for level generation {dlb}
#define GXM 80
// max y-bound for level generation {dlb}
#define GYM 70

// max traps per level
#define NTRAPS 30

// lowest grid value which can be passed by walking etc.
#define MINMOVE 31

// lowest grid value which can be seen through
#define MINSEE 11

// some shortcuts:
#define menv env.mons
#define mitm env.it[0]
#define grd env.grid
#define mgrd env.mgrid
#define igrd env.igrid


// (MNG) -- for a reason! see usage {dlb}:
#define MHITNOT 201
// (MNG + 1) -- for a reason! see usage {dlb}:
#define MHITYOU 202

// colors, such pretty colors ...
#ifndef DOS
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHTGREY 7
#define DARKGREY 8
#define LIGHTBLUE 9
#define LIGHTGREEN 10
#define LIGHTCYAN 11
#define LIGHTRED 12
#define LIGHTMAGENTA 13
#define YELLOW 14
#define WHITE 15
#else
#include <conio.h>
#define LIGHTGREY LIGHTGRAY
#define DARKGREY DARKGRAY
#endif

// required for stuff::coinflip()
#define IB1 1
#define IB2 2
#define IB5 16
#define IB18 131072
#define MASK (IB1 + IB2 + IB5)
// required for stuff::coinflip()


#endif
