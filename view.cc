/*
 *  File:       view.cc
 *  Summary:    Misc function used to render the dungeon.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *   <8>      11/23/99   LRH    Added colour-coded play-screen map & clean_map
 *                                                              init options
 *   <7>      9/29/99    BCR    Removed first argument from draw_border
 *   <6>      9/11/99    LRH    Added calls to overmap functions
 *   <5>      6/22/99    BWR    Fixed and improved the stealth
 *   <4>      5/20/99    BWR    show_map colours all portals,
 *                              exits from subdungeons now
 *                              look like up stairs.
 *   <3>      5/09/99    JDJ    show_map draws shops in yellow.
 *   <2>      5/09/99    JDJ    show_map accepts '\r' along with '.'.
 *   <1>      -/--/--    LRH    Created
 */

#include "AppHdr.h"
#include "view.h"

#include <string.h>

#ifdef DOS
#include <conio.h>
#endif

#ifdef USE_CURSES
#include <curses.h>
#endif

#include "externs.h"

#include "debug.h"
#include "monstuff.h"
#include "mon-util.h"
#include "overmap.h"
#include "player.h"
#include "stuff.h"

#ifdef MACROS
#include "macro.h"
#endif


unsigned char your_sign;        // accessed as extern in transfor.cc and acr.cc
unsigned char your_colour;      // accessed as extern in transfor.cc and acr.cc

unsigned int show_backup[19][19];

unsigned char show_green;
extern int stealth;             // defined in acr.cc
extern char visible[10];        // defined in acr.cc

char colour_map;                /* used as an extern in initfile; controls whether the
                                   play-screen map is colour-coded */

char clean_map;                 /* also used as an extern in initfile; controls whether
                                   clouds and monsters are put on the map */


bool check_awaken(int mons_aw);
char colour_code_map(unsigned char map_value);
unsigned char (*mapch) (unsigned char);
unsigned char (*mapch2) (unsigned char);
unsigned char mapchar(unsigned char ldfk);
unsigned char mapchar2(unsigned char ldfk);
unsigned char mapchar3(unsigned char ldfk);
unsigned char mapchar4(unsigned char ldfk);
void cloud_grid(void);
void monster_grid(bool do_updates);




//---------------------------------------------------------------
//
// get_ibm_symbol
//
// Returns the DOS character code and color for everything drawn
// with the IBM graphics option.
//
//---------------------------------------------------------------
static void get_ibm_symbol(unsigned int object, unsigned char *ch, unsigned char *color)
{
    ASSERT(color != NULL);
    ASSERT(ch != NULL);

    switch (object)
    {
    case DNGN_UNSEEN:
        *ch = 0;
        break;

    case DNGN_ROCK_WALL:
        *color = env.rock_colour;
        *ch = 177;
        break;                  // remember earth elementals

// stone in the realm of Zot is coloured the same as rock
    case DNGN_STONE_WALL:
        *color = ( (you.where_are_you == BRANCH_HALL_OF_ZOT) ? env.rock_colour : LIGHTGREY );
        *ch = 177;
        break;

    case DNGN_CLOSED_DOOR:
        *ch = 254;
        break;

    case DNGN_METAL_WALL:
        *ch = 177;
        *color = CYAN;
        break;

    case DNGN_SECRET_DOOR:
        *ch = 177;
        *color = env.rock_colour;
        break;

    case DNGN_GREEN_CRYSTAL_WALL:
        *ch = 177;
        *color = GREEN;
        break;

    case DNGN_ORCISH_IDOL:
        *ch = '8';
        *color = DARKGREY;
        break;

    case DNGN_WAX_WALL:
        *ch = 177;
        *color = YELLOW;
        break;                  // wax wall
        /* Anything added here must also be added to the PLAIN_TERMINAL viewwindow2 below */

    case DNGN_SILVER_STATUE:
        *ch = '8';
        *color = WHITE;
        if (visible[1] == 0)
            visible[1] = 3;
        else
            visible[1] = 2;
        visible[0] = 2;
        break;

    case DNGN_GRANITE_STATUE:
        *ch = '8';
        *color = LIGHTGREY;

        break;

    case DNGN_ORANGE_CRYSTAL_STATUE:
        *ch = '8';
        *color = LIGHTRED;

        if (visible[2] == 0)
            visible[2] = 3;
        else
            visible[2] = 2;
        visible[0] = 2;
        break;

    case DNGN_STATUE_35:
        *ch = '#';
        break;

    case DNGN_LAVA:
        *ch = 247;
        *color = RED;
        break;

    case DNGN_DEEP_WATER:
        *ch = 247;             // this wavy thing also used for water elemental
        *color = BLUE;
        break;

    case DNGN_SHALLOW_WATER:
        *ch = 247;             // this wavy thing also used for water elemental
        *color = CYAN;
        break;

    case DNGN_FLOOR:
        *color = env.floor_colour;
        *ch = 249;
        break;

    case DNGN_ENTER_HELL:
        *ch = 239;
        *color = RED;
        seen_other_thing(object);
        break;

    case DNGN_OPEN_DOOR:
        *ch = 39;
        break;

    case DNGN_BRANCH_STAIRS:
        *ch = 240;
        *color = BROWN;
        break;

    case DNGN_TRAP_MECHANICAL:
        *color = LIGHTCYAN;
        *ch = 94;
        break;

    case DNGN_TRAP_MAGICAL:
        *color = MAGENTA;
        *ch = 94;
        break;

    case DNGN_TRAP_III:
        *color = LIGHTGREY;
        *ch = 94;
        break;

    case DNGN_UNDISCOVERED_TRAP:
        *ch = 249;
        *color = env.floor_colour;
        break;

    case DNGN_ENTER_SHOP:
        *ch = 239;
        *color = YELLOW;

        seen_other_thing(object);
        break;
// if I change anything above here, must also change magic mapping!

    case DNGN_ENTER_LABYRINTH:
        *ch = 239;
        *color = LIGHTGREY;
        seen_other_thing(object);
        break;

    case DNGN_ROCK_STAIRS_DOWN:
        *color = BROWN;           // ladder    // odd {dlb}
    case DNGN_STONE_STAIRS_DOWN_I:
    case DNGN_STONE_STAIRS_DOWN_II:
    case DNGN_STONE_STAIRS_DOWN_III:
        *ch = '>';
        break;

    case DNGN_ROCK_STAIRS_UP:
        *color = BROWN;          // ladder    // odd {dlb}
    case DNGN_STONE_STAIRS_UP_I:
    case DNGN_STONE_STAIRS_UP_II:
    case DNGN_STONE_STAIRS_UP_III:
        *ch = '<';
        break;

    case DNGN_ENTER_DIS:
        *color = CYAN;
        *ch = 239;
        break;

    case DNGN_ENTER_GEHENNA:
        *color = RED;
        *ch = 239;
        break;

    case DNGN_ENTER_COCYTUS:
        *color = LIGHTCYAN;
        *ch = 239;
        break;

    case DNGN_ENTER_TARTARUS:
        *color = DARKGREY;
        *ch = 239;
        break;

    case DNGN_ENTER_ABYSS:
        *color = random2(16);
        *ch = 239;
        seen_other_thing(object);
        break;

    case DNGN_EXIT_ABYSS:
        *color = random2(16);
        *ch = 239;
        break;

    case DNGN_STONE_ARCH:
        *color = LIGHTGREY;
        *ch = 239;
        break;

    case DNGN_ENTER_PANDEMONIUM:
        *color = LIGHTBLUE;
        *ch = 239;
        seen_other_thing(object);
        break;

    case DNGN_EXIT_PANDEMONIUM:
        *color = LIGHTBLUE;
        *ch = 239;
        break;

    case DNGN_TRANSIT_PANDEMONIUM:
        *color = LIGHTGREEN;
        *ch = 239;
        break;

    case DNGN_ENTER_ORCISH_MINES:
    case DNGN_ENTER_HIVE:
    case DNGN_ENTER_LAIR_I:
    case DNGN_ENTER_SLIME_PITS:
    case DNGN_ENTER_VAULTS:
    case DNGN_ENTER_CRYPT_I:
    case DNGN_ENTER_HALL_OF_BLADES:
    case DNGN_ENTER_TEMPLE:
    case DNGN_ENTER_SNAKE_PIT:
    case DNGN_ENTER_ELVEN_HALLS:
    case DNGN_ENTER_TOMB:
    case DNGN_ENTER_SWAMP:
    case 123:
    case 124:
    case 125:
    case 126:
        *color = YELLOW;
        *ch = '>';
        seen_staircase(object);
        break;

    case DNGN_ENTER_ZOT:
        *color = MAGENTA;
        *ch = 239;
        seen_staircase(object);
        break;

    case DNGN_RETURN_DUNGEON_I:
    case DNGN_RETURN_DUNGEON_II:
    case DNGN_RETURN_DUNGEON_III:
    case DNGN_RETURN_LAIR_II:
    case DNGN_RETURN_DUNGEON_IV:
    case DNGN_RETURN_VAULTS:
    case DNGN_RETURN_CRYPT_II:
    case DNGN_RETURN_DUNGEON_V:
    case DNGN_RETURN_LAIR_III:
    case DNGN_RETURN_MINES:
    case DNGN_RETURN_CRYPT_III:
    case DNGN_RETURN_LAIR_IV:
    case 143:
    case 144:
    case 145:
    case 146:
        *color = YELLOW;
        *ch = '<';
        break;

    case DNGN_EXIT_ZOT:
        *color = MAGENTA;
        *ch = 239;
        break;

    case DNGN_ALTAR_ZIN:
        *color = WHITE;
        *ch = 220;
        seen_altar(GOD_ZIN);
        break;

    case DNGN_ALTAR_SHINING_ONE:
        *color = YELLOW;
        *ch = 220;
        seen_altar(GOD_SHINING_ONE);
        break;

    case DNGN_ALTAR_KIKUBAAQUDGHA:
        *color = DARKGREY;
        *ch = 220;
        seen_altar(GOD_KIKUBAAQUDGHA);
        break;

    case DNGN_ALTAR_YREDELEMNUL:
        *color = ( (one_chance_in(3)) ? RED : DARKGREY );
        *ch = 220;
        seen_altar(GOD_YREDELEMNUL);
        break;

    case DNGN_ALTAR_XOM:
        *color = random_colour();
        *ch = 220;
        seen_altar(GOD_XOM);
        break;

    case DNGN_ALTAR_VEHUMET:
        *color = LIGHTBLUE;
        if (one_chance_in(3))
            *color = LIGHTMAGENTA;
        if (one_chance_in(3))
            *color = LIGHTRED;
        *ch = 220;
        seen_altar(GOD_VEHUMET);
        break;

    case DNGN_ALTAR_OKAWARU:
        *color = CYAN;
        *ch = 220;
        seen_altar(GOD_OKAWARU);
        break;

    case DNGN_ALTAR_MAKHLEB:
        *color = RED;
        if (one_chance_in(3))
            *color = LIGHTRED;
        if (one_chance_in(3))
            *color = YELLOW;
        *ch = 220;
        seen_altar(GOD_MAKHLEB);
        break;

    case DNGN_ALTAR_SIF_MUNA:
        *color = BLUE;
        *ch = 220;
        seen_altar(GOD_SIF_MUNA);
        break;

    case DNGN_ALTAR_TROG:
        *color = RED;
        *ch = 220;
        seen_altar(GOD_TROG);
        break;

    case DNGN_ALTAR_NEMELEX_XOBEH:
        *color = LIGHTMAGENTA;
        *ch = 220;
        seen_altar(GOD_NEMELEX_XOBEH);
        break;

    case DNGN_ALTAR_ELYVILON:
        *color = LIGHTGREY;
        *ch = 220;
        seen_altar(GOD_ELYVILON);
        break;

    case DNGN_BLUE_FOUNTAIN:
        *color = BLUE;
        *ch = 159;
        break;

    case DNGN_SPARKLING_FOUNTAIN:
        *color = LIGHTBLUE;
        *ch = 159;
        break;

    case DNGN_DRY_FOUNTAIN_I:
    case DNGN_DRY_FOUNTAIN_II:
    case DNGN_PERMADRY_FOUNTAIN:
        *color = LIGHTGREY;
        *ch = 159;
        break;

    case 256:
        *ch = '0';
        break;

    case 257:
        *color = CYAN;
        *ch = '~';
        break;                  /* Invis creature walking through water */

    case 258:
        *ch = ')';
        break;                  // weapon )

    case 259:
        *ch = '[';
        break;                  // armour [

    case 260:
        *ch = '/';
        break;                  // wands, etc.

    case 261:
        *ch = '%';
        break;                  // food

    case 262:
        *ch = '+';
        break;                  // books +

    case 263:
        *ch = '?';
        break;                  // scroll ?

    case 264:
        *ch = '=';
        break;                  // ring = etc

    case 265:
        *ch = '!';
        break;                  // potions !

    case 266:
        *ch = '(';
        break;                  // stones

    case 267:
        *ch = '+';
        break;                  // book +

    case 268:
        *ch = '%';
        break;                  // corpses part 1

    case 269:
        *ch = '\\';
        break;                  // magical staves

    case 270:
        *ch = '}';
        break;                  // gems

    case 271:
        *ch = '%';
        break;                  // don't know ?

    case 272:
        *ch = '$';
        break;                  // $ gold

    case 273:
        *ch = '"';
        break;                  // amulet

    default:
        *ch = ( (object >= 297) ? mons_char(object - 297) : object );
        break;
    }

}          // end get_ibm_symbol()




//---------------------------------------------------------------
//
// viewwindow2
//
// Draws the main window using the extended IBM character set.
//
// This function should not interfer with the game condition,
// unless do_updates is set (ie.  stealth checks for visible
// monsters).
//
//---------------------------------------------------------------
void viewwindow2( char draw_it, bool do_updates )
{

    const long BUFFER_SIZE = 1550;
    unsigned char buffy[BUFFER_SIZE];   //[800]; //392];

    unsigned char ch, color;

    _setcursortype(_NOCURSOR);

#ifdef WIZARD
    memset(buffy, 255, sizeof(buffy));
#endif

    losight(env.show, grd, you.x_pos, you.y_pos);

    int count_x, count_y;

    for (count_x = 0; count_x < 18; count_x++)
      for (count_y = 0; count_y < 18; count_y++)
      {
          env.show_col[count_x][count_y] = LIGHTGREY;
          show_backup[count_x][count_y] = 0;
      }

    item();
    cloud_grid();
    monster_grid(do_updates);
    int bufcount = 0;

    if (draw_it == 1)
    {
        for (count_y = you.y_pos - 8; count_y < you.y_pos + 9; count_y++)
        {
            bufcount += 16;

            for (count_x = you.x_pos - 8; count_x < you.x_pos + 9; count_x++)
            {
                color = env.show_col[count_x - you.x_pos + 9][count_y - you.y_pos + 9];         // may be overriden by the code below

                if (count_x != you.x_pos || count_y != you.y_pos)
                {
                    unsigned int object = env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9];

                    get_ibm_symbol(object, &ch, &color);

                }
                else
                {
                    ch = your_sign;
                    color = your_colour;
                }

                ASSERT(bufcount + 1 < BUFFER_SIZE);
                buffy[bufcount] = ch;
                buffy[bufcount + 1] = color;

                bufcount += 2;
            }

            bufcount += 16;
        }

        if (you.level_type != LEVEL_LABYRINTH && you.level_type != LEVEL_ABYSS)
        {
            bufcount = 0;

            for (count_y = 0; count_y < 17; count_y++)
            {
                bufcount += 16;

                for (count_x = 0; count_x < 17; count_x++)
                {
                    ASSERT(bufcount < BUFFER_SIZE);

                    if (buffy[bufcount] != 0)
                        env.map[count_x + you.x_pos - 9][count_y + you.y_pos - 9] = buffy[bufcount];
                    if (clean_map == 1 && show_backup[count_x + 1][count_y + 1] != 0)
                    {
                        get_ibm_symbol(show_backup[count_x + 1][count_y + 1], &ch, &color);
                        env.map[count_x + you.x_pos - 9][count_y + you.y_pos - 9] = ch;
                    }
                    bufcount += 2;
                }
                bufcount += 16;
            }
        }

        bufcount = 0;
        for (count_y = 0; count_y < 17; count_y++)
        {
            for (count_x = 0; count_x < 33; count_x++)
            {
                if (count_x + you.x_pos - 17 < 3 || count_y + you.y_pos - 9 < 3 || count_x + you.x_pos - 14 > 77 || count_y + you.y_pos - 9 > 67)
                {
                    ASSERT(bufcount < BUFFER_SIZE);
                    buffy[bufcount] = 0;
                    bufcount++;
                    buffy[bufcount] = 0;
                    bufcount++;
                    continue;
                }
                if (count_x >= 8 && count_x <= 24 && count_y >= 0 && count_y <= 16 && buffy[bufcount] != 0)
                {
                    bufcount += 2;
                    continue;
                }
                ASSERT(bufcount + 1 < BUFFER_SIZE);
                buffy[bufcount] = env.map[count_x + you.x_pos - 17][count_y + you.y_pos - 9];
                buffy[bufcount + 1] = DARKGREY;
                if (colour_map)
                {
                    if ( env.map[count_x + you.x_pos - 16][count_y + you.y_pos - 8] != 0 )
                      buffy[bufcount + 1] = colour_code_map(grd[count_x + you.x_pos - 16][count_y + you.y_pos - 8]);
                }
                bufcount += 2;
            }
        }

        if ( you.berserker )
        {
            for (count_x = 1; count_x < 1400; count_x += 2)
              if ( buffy[count_x] != DARKGREY )
                buffy[count_x] = RED;
        }

        if ( show_green != BLACK )
        {
            for (count_x = 1; count_x < 1400; count_x += 2)
              if ( buffy[count_x] != DARKGREY )
                buffy[count_x] = show_green;

            show_green = BLACK;

            if ( you.special_wield == SPWLD_SHADOW )
              show_green = DARKGREY;
        }

#ifdef DOS_TERM
        puttext(2, 1, 34, 17, buffy);
#endif

#ifdef PLAIN_TERM
        gotoxy(2, 1);
        bufcount = 0;

        // if (you.running == 0) // this line is purely optional
        for (count_x = 0; count_x < 1120; count_x += 2)
        {                       // 1056

            ch = buffy[count_x];
            color = buffy[count_x + 1];
            ASSERT(color < 16);
            ASSERT(ch < 255);

            textcolor(color);
            putch(ch);
            if ( count_x % 66 == 64 && count_x > 0 )
              gotoxy(2, wherey() + 1);
        }
#endif
    }

}          // end viewwindow2()




char colour_code_map( unsigned char map_value )
{
    switch (map_value)
    {
      case DNGN_TRAP_MECHANICAL:
        return LIGHTCYAN;
        break;

      case DNGN_TRAP_MAGICAL:
      case DNGN_TRAP_III:
        return MAGENTA;
        break;

      case DNGN_ENTER_SHOP:
        return YELLOW;
        break;

      case DNGN_ENTER_DIS:
        return CYAN;
        break;

      case DNGN_ENTER_HELL:
      case DNGN_ENTER_GEHENNA:
        return RED;
        break;

      case DNGN_ENTER_COCYTUS:
        return LIGHTCYAN;
        break;

      case DNGN_ENTER_ABYSS:
        return random2(16);     // so it can be black - is this right? {dlb}
        break;

      case DNGN_ENTER_LABYRINTH:
      case DNGN_STONE_ARCH:
        return LIGHTGREY;
        break;

      case DNGN_ENTER_PANDEMONIUM:
        return LIGHTBLUE;
        break;

      case DNGN_EXIT_PANDEMONIUM:
      case DNGN_TRANSIT_PANDEMONIUM:
        // These are Pandemonium gates, I'm using light
        // green for all types of gates as to maintain
        // the fact that the player should have to
        // visit them to verify what they are...
        // not just use a crystal ball. -- bwross
        return LIGHTGREEN;
        break;

      case DNGN_ENTER_ZOT:
      case DNGN_EXIT_ZOT:
        return MAGENTA;
        break;

      case DNGN_STONE_STAIRS_DOWN_I:
      case DNGN_STONE_STAIRS_DOWN_II:
      case DNGN_STONE_STAIRS_DOWN_III:
      case DNGN_ROCK_STAIRS_DOWN:
        return RED;
        break;

      case DNGN_STONE_STAIRS_UP_I:
      case DNGN_STONE_STAIRS_UP_II:
      case DNGN_STONE_STAIRS_UP_III:
      case DNGN_ROCK_STAIRS_UP:
        return BLUE;
        break;

      case DNGN_ENTER_ORCISH_MINES:
      case DNGN_ENTER_HIVE:
      case DNGN_ENTER_LAIR_I:
      case DNGN_ENTER_SLIME_PITS:
      case DNGN_ENTER_VAULTS:
      case DNGN_ENTER_CRYPT_I:
      case DNGN_ENTER_HALL_OF_BLADES:
      case DNGN_ENTER_TEMPLE:
      case DNGN_ENTER_SNAKE_PIT:
      case DNGN_ENTER_ELVEN_HALLS:
      case DNGN_ENTER_TOMB:
      case DNGN_ENTER_SWAMP:
      case 123:
      case 124:
      case 125:
      case 126:
        return LIGHTRED;
        break;

      case DNGN_RETURN_DUNGEON_I:
      case DNGN_RETURN_DUNGEON_II:
      case DNGN_RETURN_DUNGEON_III:
      case DNGN_RETURN_LAIR_II:
      case DNGN_RETURN_DUNGEON_IV:
      case DNGN_RETURN_VAULTS:
      case DNGN_RETURN_CRYPT_II:
      case DNGN_RETURN_DUNGEON_V:
      case DNGN_RETURN_LAIR_III:
      case DNGN_RETURN_MINES:
      case DNGN_RETURN_CRYPT_III:
      case DNGN_RETURN_LAIR_IV:
      case 143:
      case 144:
      case 145:
      case 146:
        return LIGHTBLUE;
        break;

    default:
        return DARKGREY;
    }

}




void monster_grid( bool do_updates )
{

    //int mnc = 0;
    struct monsters *monster = 0;    // NULL {dlb}

#ifdef DEBUG
    if ( do_updates )
      mpr("Stealth checks...");
#endif

    for (int s = 0; s < MNST; s++)
    {
        monster = &menv[s];

        if ( monster->type != -1 )
        {
            //mnc++;

            if ( mons_near(monster) )
            {
                if ( do_updates && ( monster->behavior == BEH_SLEEP || monster->behavior == BEH_WANDER ) && check_awaken(s) )
                {
                    monster->behavior = BEH_CHASING_I;
                    monster->target_x = you.x_pos;
                    monster->target_y = you.y_pos;

                    if ( you.turn_is_over == 1
                        && mons_shouts(monster->type) > 0
                        && random2(30) >= you.skills[SK_STEALTH] )
                    {
                        if ( !silenced(you.x_pos, you.y_pos) && !silenced(monster->x, monster->y) )
                        {
                             char the_shout = mons_shouts(monster->type);

                             strcpy(info, "You hear ");
                             strcat(info, (the_shout ==  1) ? "a shout!" :
                                          (the_shout ==  2) ? "a bark!" :
                                          (the_shout ==  3) ? "two shouts!" :
                                          (the_shout ==  4) ? "a roar!" :
                                          (the_shout ==  5) ? "a hideous shriek!" :
                                          (the_shout ==  6) ? "a bellow!" :
                                          (the_shout ==  7) ? "a screech!" :
                                          (the_shout ==  8) ? "an angry buzzing noise." :
                                          (the_shout ==  9) ? "a chilling moan." :
                                          (the_shout == 10) ? "an irritating high-pitched whine." :
                                          (the_shout == 11) ? "a croak."
                                                            : "buggy behavior!" );
                             mpr(info);
                        }

                        noisy(8, monster->x, monster->y);
                    }
                }

                if ( monster->enchantment[2] == ENCH_INVIS
                    && ( !player_see_invis() || ( monster_habitat(monster->type) != DNGN_FLOOR && monster->number == 1 ) ) )
                {
                    if ( grd[monster->x][monster->y] == DNGN_SHALLOW_WATER && !mons_flies(monster->type) )
                    {
                        show_backup[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9] = env.show[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9];
                        env.show[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9] = 257;
                    }
                    continue;
                }
                else if ( monster->behavior != BEH_ENSLAVED && mons_category( monster->type ) != MC_MIMIC )
                  you.running = 0;    /* Friendly monsters or mimics don't disturb */

                if ( mons_category( monster->type ) != MC_MIMIC )         // mimics are always left on map
                  show_backup[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9] = env.show[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9];

                env.show[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9] = monster->type + 297;
                env.show_col[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9] = ( (mcolour[monster->type] == BLACK) ? monster->number : mcolour[monster->type] );

            }        // end "if mons_near(monster)"

        }        // end "if (monster->type != -1)"

    }        // end "for s"

}          // end monster_grid()




bool check_awaken( int mons_aw )
{

    int mons_perc = 0;
    struct monsters *monster = &menv[mons_aw];

// berserkers aren't really concerned about stealth
    if ( you.berserker )
      return true;

// I assume that creatures who can see invisible are very perceptive
    mons_perc = 10 + (mons_intel(monster->type) * 4) + monster->hit_dice + mons_see_invis(monster->type) * 5;

    if ( you.invis && !mons_see_invis(monster->type) )
      mons_perc -= 75;

    if ( mons_perc < 0 )
      mons_perc = 0;

    if ( random2(stealth) <= mons_perc )
      return true;

    return false;

}          // end check_awaken()




void item( void )
{

    char count_x, count_y;

    for (count_y = (you.y_pos - 8); (count_y < you.y_pos + 9); count_y++)
      for (count_x = (you.x_pos - 8); (count_x < you.x_pos + 9); count_x++)
      {
          if ( igrd[count_x][count_y] != ING )
          {
              if ( env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] != 0 )
              {
                  if ( grd[count_x][count_y] == DNGN_SHALLOW_WATER )
                    env.show_col[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = CYAN;
                  else
                    env.show_col[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = mitm.colour[igrd[count_x][count_y]];

                  switch ( mitm.base_type[igrd[count_x][count_y]] )
                  {
                    case OBJ_ORBS:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 256;
                      break;
                // need + 6 because show is 0 - 12, not -6 - +6
                    case OBJ_WEAPONS:
                    case OBJ_MISSILES:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 258;
                      break;
                    case OBJ_ARMOUR:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 259;
                      break;
                    case OBJ_WANDS:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 260;
                      break;
                    case OBJ_FOOD:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 261;
                      break;
                    case OBJ_UNKNOWN_I:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 262;
                      break;
                    case OBJ_SCROLLS:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 263;
                      break;
                    case OBJ_JEWELLERY:
                      if ( mitm.sub_type[igrd[count_x][count_y]] >= AMU_RAGE )
                          env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 273;
                      else
                          env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 264;
                      break;
                    case OBJ_POTIONS:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 265;
                      break;
                    case OBJ_UNKNOWN_II:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 266;
                      break;
                    case OBJ_BOOKS:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 267;
                      break;
                    case OBJ_STAVES:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 269;
                      break;
                    case OBJ_MISCELLANY:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 270;
                      break;
                    case OBJ_CORPSES:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 271;
                      break;
                    case OBJ_GOLD:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = 272;
                      env.show_col[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = YELLOW;
                      break;
                    default:
                      env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9] = '8';
                      break;
                  }
              }
          }
      }                       // end of "for count_y, count_x"

}          // end item()




void cloud_grid( void )
{

    int mnc = 0;

    unsigned char which_color = LIGHTGREY;     // btw, this is also the 'default' color {dlb}

    for (int s = 0; s < CLOUDS; s++)
    {
        if ( mnc > env.cloud_no )    // can anyoneexplain this??? {dlb}
          break;

        if ( env.cloud_type[s] != CLOUD_NONE )
        {
            mnc++;

            if ( see_grid(env.cloud_x[s], env.cloud_y[s]) )
            {
                show_backup[env.cloud_x[s] - you.x_pos + 9][env.cloud_y[s] - you.y_pos + 9] = env.show[env.cloud_x[s] - you.x_pos + 9][env.cloud_y[s] - you.y_pos + 9];

                env.show[env.cloud_x[s] - you.x_pos + 9][env.cloud_y[s] - you.y_pos + 9] = '#';

                switch ( env.cloud_type[s] )
                {
                  case CLOUD_FIRE:
                  case CLOUD_FIRE_MON:
                    if ( env.cloud_decay[s] <= 20 )
                      which_color = RED;
                    else if ( env.cloud_decay[s] <= 40 )
                      which_color = LIGHTRED;
                    else if ( one_chance_in(4) )
                      which_color = RED;
                    else if ( one_chance_in(4) )
                      which_color = LIGHTRED;
                    else
                      which_color = YELLOW;
                    break;

                  case CLOUD_STINK:
                  case CLOUD_STINK_MON:
                    which_color = GREEN;
                    break;

                  case CLOUD_COLD:
                  case CLOUD_COLD_MON:
                    if ( env.cloud_decay[s] <= 20 )
                      which_color = BLUE;
                    else if ( env.cloud_decay[s] <= 40 )
                      which_color = LIGHTBLUE;
                    else if ( one_chance_in(4) )
                      which_color = BLUE;
                    else if ( one_chance_in(4) )
                      which_color = LIGHTBLUE;
                    else
                      which_color = WHITE;
                    break;

                  case CLOUD_POISON:
                  case CLOUD_POISON_MON:
                    which_color = ( one_chance_in(3) ? LIGHTGREEN : GREEN );
                    break;

                  case CLOUD_BLUE_SMOKE:
                  case CLOUD_BLUE_SMOKE_MON:
                    which_color = LIGHTBLUE;
                    break;

                  case CLOUD_PURP_SMOKE:
                  case CLOUD_PURP_SMOKE_MON:
                    which_color = MAGENTA;
                    break;

                  case CLOUD_MIASMA:
                  case CLOUD_MIASMA_MON:
                  case CLOUD_BLACK_SMOKE:
                  case CLOUD_BLACK_SMOKE_MON:
                    which_color = DARKGREY;
                    break;

                  default:
                    which_color = LIGHTGREY;
                    break;
                }

                env.show_col[env.cloud_x[s] - you.x_pos + 9][env.cloud_y[s] - you.y_pos + 9] = which_color;

            }

        }        // end 'if != CLOUD_NONE'

    }        // end 'for s' loop

}                    // end cloud_grid()




// All items must have show values >= 38, all grid squares must be < 38
// because of monster invisibility.
//jmf: does above comment refer to noisy in some way?
void noisy( char loudness, char nois_x, char nois_y )
{

    int p;
    struct monsters *monster = 0;    // NULL {dlb}

    if ( silenced(nois_x, nois_y) )
      return;

    int dist = int(loudness) * int(loudness);

    for (p = 0; p < MNST; p++)
    {
        monster = &menv[p];

        //if (monster->x >= nois_x - loudness && monster->x <= nois_x + loudness
        //  && monster->y >= nois_y - loudness && monster->y <= nois_y + loudness)
        //jmf: now that we have a working distance function ... 26mar2000

        if ( dist <= distance(monster->x, monster->y, nois_x, nois_y)
            && !silenced(monster->x, monster->y) )
        {
            if ( monster->behavior == BEH_SLEEP )
              monster->behavior = BEH_CHASING_I;

            monster->target_x = nois_x;
            monster->target_y = nois_y;
        }

    }

}          // end noisy()




/*
 The losight function is so complex and tangled that I daren't even look at it.
 Good luck trying to work out what each bit does.
*/
void losight( unsigned int sh[19][19], unsigned char gr[GXM][GYM], int x_p, int y_p )
{

    int loopy = 0;         // general purpose loop variable {dlb}
    char shad;

    bool see;                // 'true' means 'visible'
    bool see_section;
    //bool behind = false;   // variable meaningless in usage (check below) {dlb}

    char startPoint_x = 0;   // = 8;
    char startPoint_y = 0;   // = 7;

    char xs = 0;             // the multiplier of the x addition thing
    char ys = 0;

    char xsmult[6] = {0,0,0,0,0,0};    // simply (xs * (index + 1)) {dlb}
    char ysmult[6] = {0,0,0,0,0,0};    // simply (ys * (index + 1)) {dlb}

    char cx = 0;
    char cy = 0;


// first comes the horizontal east:
    see = true;

    for (cx = (x_p + 1); (cx < x_p + 9); cx++)
    {
        if ( see && gr[cx - 1][y_p] < MINSEE )
          see = false;

        sh[cx - x_p + 9][9] = ( (see) ? gr[cx][y_p] : 0 );
    }

// now the horizontal West:
    see = true;

    for (cx = (x_p - 1); (cx > x_p - 9); cx--)
    {
        if ( see && gr[cx + 1][y_p] < MINSEE )
          see = false;

        sh[cx - x_p + 9][9] = ( (see) ? gr[cx][y_p] : 0 );
    }

// now for the North:
    see = true;

    for (cy = (y_p - 1); (cy > y_p - 9); cy--)
    {
        if ( see && gr[x_p][cy + 1] < MINSEE )
          see = false;

        sh[9][cy - y_p + 9] = ( (see) ? gr[x_p][cy] : 0 );
    }

// and the South...
    see = true;

    for (cy = (y_p + 1); (cy < y_p + 9); cy++)
    {
        if ( see && gr[x_p][cy - 1] < MINSEE )
          see = false;

        sh[9][cy - y_p + 9] = ( (see) ? gr[x_p][cy] : 0 );
    }

// Try the Southeast:
    see = true;
    cy = y_p + 1;

    for (cx = x_p + 1; cx < x_p + 7; cx++)
    {
        if ( see && gr[cx - 1][cy - 1] < MINSEE )
          see = false;

        sh[cx - x_p + 9][cy - y_p + 9] = ( (see) ? gr[cx][cy] : 0 );

        cy++;
    }

// Now for the Northeast:
    see = true;
    cy = y_p - 1;

    for (cx = x_p + 1; cx < x_p + 7; cx++)
    {
        if ( see && gr[cx - 1][cy + 1] < MINSEE )
          see = false;

        sh[cx - x_p + 9][cy - y_p + 9] = ( (see) ? gr[cx][cy] : 0 );

        cy--;
    }

// The Northwest:
    see = true;
    cy = y_p - 1;

    for (cx = x_p - 1; cx > x_p - 7; cx--)
    {
        if ( see && gr[cx + 1][cy + 1] < MINSEE )
          see = false;

        sh[cx - x_p + 9][cy - y_p + 9] = ( (see) ? gr[cx][cy] : 0 );

        cy--;
    }

// And the Southwest
    see = true;
    cy = y_p + 1;

    for (cx = x_p - 1; cx > x_p - 7; cx--)
    {
        if ( see && gr[cx + 1][cy - 1] < MINSEE )
          see = false;

        sh[cx - x_p + 9][cy - y_p + 9] = ( (see) ? gr[cx][cy] : 0 );

        cy++;
    }

// Anyway, now for the Fun part!
    //see = true;    // not needed -- set within loops that follow {dlb}

    for (shad = 1; shad < 5; shad++)
    {
        xs = ( (shad == 1 || shad == 2) ? 1 : -1 );
        ys = ( (shad == 1 || shad == 3) ? 1 : -1 );

        startPoint_x = ( (shad == 1 || shad == 2) ? 11 : 7 );
        startPoint_y = ( (shad == 1 || shad == 3) ? 10 : 8 );

    // why do the math each and every time?
    // array looks cleaner, but separate variables may be quicker {dlb}:
        for(loopy = 0; loopy < 6; loopy++)
        {
            xsmult[loopy] = ( xs * (1 + loopy) );
            ysmult[loopy] = ( ys * (1 + loopy) );
        }

        //behind = false;

        see = true;
        see_section = !(gr[x_p + xs][y_p + ys] < MINSEE && gr[x_p + xs][y_p] < MINSEE);
        see = see_section;

        sh[startPoint_x][startPoint_y] = ( (see) ? gr[x_p + startPoint_x - 9][y_p + startPoint_y - 9] : 0 );

        if ( see && gr[x_p + startPoint_x - 9][y_p + startPoint_y - 9] < MINSEE )
          see = false;

        sh[startPoint_x + xs][startPoint_y + ys] = ( (see) ? gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ys - 9] : 0 );

        if ( see && gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ys - 9] < MINSEE )
          see = false;

        sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );

        if ( see && gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[1] - 9] < MINSEE )
          see = false;

        sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[2]] = ( (see) ? gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[2] - 9] : 0 );

        sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );

// Wider:

        // This is done in a different way: see the >= MINSEE instead of < MINSEE

        if ( gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[2] - 9] >= MINSEE )     //see = false;
        {
            if ( see )
              sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[3]] = gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[3] - 9];
            else
            {
                sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[3]] = 0;
                see = false;
            }
        }
        else
        {
            sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[3]] = 0;
            see = false;
        }


/*
        if ( sh [startPoint_x - 3*xs] [startPoint_y + 6*ys] != 0 ) //see = false;
          if (!(gr [x_p + startPoint_x - 3*xs - 9] [y_p + startPoint_y + 6*ys - 9] < MINSEE)) //see = false;
            {
              if (!(gr [x_p + startPoint_x - 3*xs - 9] [y_p + startPoint_y + 5*ys - 9] < MINSEE)) //see = false;
                {
                  if ( see ) sh [startPoint_x - 2*xs] [startPoint_y + 7*ys] =  gr [x_p + startPoint_x - 2*xs - 9] [y_p + startPoint_y + 7*ys - 9]; else sh [startPoint_x - 2*xs] [startPoint_y + 7*ys] = 0;
                } else sh [startPoint_x - 2*xs] [startPoint_y + 7*ys] = 0;
            } else sh [startPoint_x - 2*xs] [startPoint_y + 7*ys] = 0;
        else sh [startPoint_x - 2*xs] [startPoint_y + 7*ys] = 0;
*/


// That's one line done...

        see_section = true;

        if ( gr[x_p + xs][y_p + ys] < MINSEE && gr[x_p + xs][y_p] < MINSEE || gr[x_p + xsmult[1]][y_p + ys] < MINSEE && gr[x_p + xsmult[1]][y_p] < MINSEE )
          see_section = false;

        see = see_section;

        sh[startPoint_x + xs][startPoint_y] = ( (see) ? gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y - 9] : 0 );

        if ( see && gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y - 9] < MINSEE )
          see = false;

        if ( see && gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y - 9] >= MINSEE)
        {
            sh[startPoint_x + xsmult[2]][startPoint_y + ys] = gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ys - 9];

        // Wider:
            if ( gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y - 9] >= MINSEE )
              sh[startPoint_x + xsmult[3]][startPoint_y + ys] = gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ys - 9];        //else sh [startPoint_x + 4*xs] [startPoint_y + ys] = 0;
            else
              sh[startPoint_x + xsmult[3]][startPoint_y + ys] = 0;

        // Okay.
        }
        else
        {
            for (loopy = 3; loopy < 7; loopy++)
              sh[startPoint_x + loopy * xs][startPoint_y + ys] = 0;
        }

        sh[startPoint_x + xsmult[1]][startPoint_y + ys] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ys - 9] : 0 );

        if (see && gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ys - 9] < MINSEE)
          see = false;

        sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );

    // Wider:
        if (gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[1] - 9] >= MINSEE)     //see = false;
          sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[2]] = ( (see) ? gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[2] - 9] : 0 );
        else
          sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[2]] = 0;

    // This should work better:
        if (gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ys - 9] >= MINSEE)         //see = false;
        {
            if ( see )
              sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[1]] = gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[1] - 9];
            else
            {
                sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[1]] = 0;
                see = false;
            }
        }
        else
        {
            see = true;
            sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[1]] = 0;
        }

        //if ( see ) sh [startPoint_x + 5*xs] [startPoint_y + 2*ys] = gr [x_p + startPoint_x + 5*xs - 9] [y_p + startPoint_y + 2*ys - 9]; else sh [startPoint_x + 5*xs] [startPoint_y + 2*ys] = 0;

        //see = see_section;    // why set it here if it is immediately set below? {dlb}

    // And one more for this section:
        if ( see_section && gr[x_p + xsmult[2]][y_p] < MINSEE && gr[x_p + xsmult[2]][y_p + ys] < MINSEE )
          see_section = false;

        see = see_section;


        if ( see && gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y - 9] < MINSEE )
          see = false;

        if ( see_section && gr[x_p + xsmult[2]][y_p + ys] < MINSEE && gr[x_p + xsmult[2]][y_p + ysmult[1]] < MINSEE )
          see_section = false;

        see = see_section;

        sh[startPoint_x + xsmult[1]][startPoint_y] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y - 9] : 0 );

        if ( see && gr[x_p + xsmult[3]][y_p + ysmult[1]] < MINSEE && gr[x_p + xsmult[3]][y_p + ys] < MINSEE )
          see = false;

        if ((gr[x_p + xs][y_p] < MINSEE && gr[x_p + xs][y_p + ys] < MINSEE) || (gr[x_p + xsmult[1]][y_p] < MINSEE && gr[x_p + xsmult[1]][y_p + ys] < MINSEE) || (gr[x_p + xsmult[2]][y_p] < MINSEE && gr[x_p + xsmult[2]][y_p + ys] < MINSEE))
          sh[startPoint_x + xsmult[1]][startPoint_y] = 0;
        else
          sh[startPoint_x + xsmult[1]][startPoint_y] = gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y - 9];


        if ((gr[x_p + xs][y_p] < MINSEE && gr[x_p + xs][y_p + ys] < MINSEE) || (gr[x_p + xsmult[1]][y_p] < MINSEE && gr[x_p + xsmult[1]][y_p + ys] < MINSEE) || (gr[x_p + xsmult[2]][y_p] < MINSEE && gr[x_p + xsmult[2]][y_p + ys] < MINSEE) || (gr[x_p + xsmult[3]][y_p] < MINSEE && gr[x_p + xsmult[3]][y_p + ys] < MINSEE))
        {
            sh[startPoint_x + xsmult[2]][startPoint_y] = 0;
            sh[startPoint_x + xsmult[3]][startPoint_y] = 0;
        }
        else
        {
            sh[startPoint_x + xsmult[2]][startPoint_y] = gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y - 9];

            if (gr[x_p + xsmult[4]][y_p] < MINSEE && gr[x_p + xsmult[4]][y_p + ys] < MINSEE)
              sh[startPoint_x + xsmult[3]][startPoint_y] = 0;
            else
              sh[startPoint_x + xsmult[3]][startPoint_y] = gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y - 9];
        }

    // These do the far two layers.
        see = true;

        if (sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[2]] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))   //see = false;
                  sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[3]] = ( (see) ? gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y + ysmult[3] - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[3]] = 0;
            }
            else
              sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[3]] = 0;
        }
        else
          sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[3]] = 0;

        //if ( !see ) sh [startPoint_x + 3*xs] [startPoint_y + 6*ys] = 0;
        //if (sh [startPoint_x] [startPoint_y] == 0) see = false;

        if (sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[1]] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[1] - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[1] - 9] < MINSEE))   //see = false;
                  sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[2]] = ( (see) ? gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y + ysmult[2] - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[2]] = 0;
            }
            else
              sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[2]] = 0;
        }
        else
          sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[2]] = 0;

        //if ( !see ) sh [startPoint_x + 2*xs] [startPoint_y + 6*ys] = 0;
        //if (sh [startPoint_x] [startPoint_y] == 0) see = false;

        if (sh[startPoint_x + xsmult[3]][startPoint_y + ys] != 0)  //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ys - 9] < MINSEE))   //see = false;
            {
                if ( !(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ys - 9] < MINSEE) )       //see = false;
                  sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[1]] = 0;
            }
            else
              sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[1]] = 0;
        }
        else
          sh[startPoint_x + xsmult[4]][startPoint_y + ysmult[1]] = 0;

        //if ( !see ) sh [startPoint_x + xs] [startPoint_y + 6*ys] = 0;

        if (sh[startPoint_x + xsmult[3]][startPoint_y] != 0)       //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y - 9] < MINSEE))        //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y - 9] < MINSEE))    //see = false;
                  sh[startPoint_x + xsmult[4]][startPoint_y + ys] = ( (see) ? gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y + ys - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[4]][startPoint_y + ys] = 0;
            }
            else
              sh[startPoint_x + xsmult[4]][startPoint_y + ys] = 0;
        }
        else
          sh[startPoint_x + xsmult[4]][startPoint_y + ys] = 0;



        if (sh[startPoint_x + xsmult[3]][startPoint_y - ys] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y - ys - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y - ys - 9] < MINSEE))   //see = false;
                  sh[startPoint_x + xsmult[4]][startPoint_y] = ( (see) ? gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[4]][startPoint_y] = 0;
            }
            else
              sh[startPoint_x + xsmult[4]][startPoint_y] = 0;
        }
        else
          sh[startPoint_x + xsmult[4]][startPoint_y] = 0;



        if (sh[startPoint_x + xsmult[4]][startPoint_y] != 0)       //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y - 9] < MINSEE))        //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y - 9] < MINSEE))    //see = false;
                  sh[startPoint_x + xsmult[5]][startPoint_y + ys] = ( (see) ? gr[x_p + startPoint_x + xsmult[5] - 9][y_p + startPoint_y + ys - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[5]][startPoint_y + ys] = 0;
            }
            else
              sh[startPoint_x + xsmult[5]][startPoint_y + ys] = 0;
        }
        else
          sh[startPoint_x + xsmult[5]][startPoint_y + ys] = 0;



        if (sh[startPoint_x + xsmult[4]][startPoint_y - ys] != 0)  //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[4] - 9][y_p + startPoint_y - ys - 9] < MINSEE))   //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y - ys - 9] < MINSEE))       //see = false;
                  sh[startPoint_x + xsmult[5]][startPoint_y] = ( (see) ? gr[x_p + startPoint_x + xsmult[5] - 9][y_p + startPoint_y - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[5]][startPoint_y] = 0;
            }
            else
              sh[startPoint_x + xsmult[5]][startPoint_y] = 0;
        }
        else
          sh[startPoint_x + xsmult[5]][startPoint_y] = 0;

    }        // end first "for shad"



// The second lot:
    for (shad = 1; shad < 5; shad++)
    {
        xs = ( (shad == 1 || shad == 2) ? 1 : -1 );
        ys = ( (shad == 1 || shad == 3) ? 1 : -1 );

        startPoint_x = ( (shad == 1 || shad == 2) ? 10 : 8 );
        startPoint_y = ( (shad == 1 || shad == 3) ? 11 : 7 );

    // why do the math each and every time?
    // array looks cleaner, but separate variables may be quicker {dlb}:
        for(loopy = 0; loopy < 6; loopy++)
        {
            xsmult[loopy] = ( xs * (1 + loopy) );
            ysmult[loopy] = ( ys * (1 + loopy) );
        }


        //behind = false;

        see = true;
        see_section = !(gr[x_p + xs][y_p + ys] < MINSEE && gr[x_p][y_p + ys] < MINSEE);
        see = see_section;

        sh[startPoint_x][startPoint_y] = ( (see) ? gr[x_p + startPoint_x - 9][y_p + startPoint_y - 9] : 0 );

        if ( see && gr[x_p + startPoint_x - 9][y_p + startPoint_y - 9] < MINSEE )
          see = false;

        sh[startPoint_x + xs][startPoint_y + ys] = ( (see) ? gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ys - 9] : 0 );

        if ( see && gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ys - 9] < MINSEE )
          see = false;

        sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );

        if ( see && gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[1] - 9] < MINSEE )
          see = false;

        sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[2]] = ( (see) ? gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[2] - 9] : 0 );

        sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[2]] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[2] - 9] : 0 );

    // Wider:

        if (gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[2] - 9] >= MINSEE)     //see = false;
          sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[3]] = ( (see) ? gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[3] - 9] : 0 );
        else
          sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[3]] = 0;

    // Okay.


/* ******************************************************************

   if (sh [startPoint_x + 6*xs] [startPoint_y - 3*ys] != 0)
   if (!(gr [x_p + startPoint_x + 6*xs - 9] [y_p + startPoint_y - 3*ys - 9] < MINSEE))
   {
   if (!(gr [x_p + startPoint_x + 5*xs - 9] [y_p + startPoint_y - 3*ys - 9] < MINSEE))
   {
   sh [startPoint_x + 7*xs] [startPoint_y - 2*ys] =  gr [x_p + startPoint_x + 7*xs - 9] [y_p + startPoint_y - 2*ys - 9]; //else sh [startPoint_x + 7*xs] [startPoint_y - 2*ys] = 0;
   } else sh [startPoint_x + 7*xs] [startPoint_y - 2*ys] = 0;
   } else sh [startPoint_x + 7*xs] [startPoint_y - 2*ys] = 0;
   else sh [startPoint_x + 7*xs] [startPoint_y - 2*ys] = 0;

   if (sh [startPoint_x + 6*xs] [startPoint_y - 3*ys] != 0)
   if (!(gr [x_p + startPoint_x + 6*xs - 9] [y_p + startPoint_y - 3*ys - 9] < MINSEE))
   {
   if (!(gr [x_p + startPoint_x + 5*xs - 9] [y_p + startPoint_y - 3*ys - 9] < MINSEE))
   {
   sh [startPoint_x + 7*xs] [startPoint_y - 3*ys] =  gr [x_p + startPoint_x + 7*xs - 9] [y_p + startPoint_y - 3*ys - 9]; //else sh [startPoint_x + 7*xs] [startPoint_y - 3*ys] = 0;
   } else sh [startPoint_x + 7*xs] [startPoint_y - 3*ys] = 0;
   } else sh [startPoint_x + 7*xs] [startPoint_y - 3*ys] = 0;
   else sh [startPoint_x + 7*xs] [startPoint_y - 3*ys] = 0;

****************************************************************** */


// That's one line done...

        if ( see_section && ( gr[x_p + xs][y_p + ys] < MINSEE && gr[x_p][y_p + ys] < MINSEE || gr[x_p + xs][y_p + ysmult[1]] < MINSEE && gr[x_p][y_p + ysmult[1]] < MINSEE ) )
          see_section = false;

        see = see_section;

        sh[startPoint_x][startPoint_y + ys] = ( (see) ? gr[x_p + startPoint_x - 9][y_p + startPoint_y + ys - 9] : 0 );

        if ( see && gr[x_p + startPoint_x - 9][y_p + startPoint_y + ys - 9] < MINSEE )
          see = false;

        if ( see && gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[1] - 9] >= MINSEE )
        {
            sh[startPoint_x + xs][startPoint_y + ysmult[2]] = gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[2] - 9];
            // Wider:
            if (gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[2] - 9] >= MINSEE)
            {
                sh[startPoint_x + xs][startPoint_y + ysmult[3]] = gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[3] - 9];        //else sh [startPoint_x + 4*xs] [startPoint_y + ys] = 0;
            }
            else
                sh[startPoint_x + xs][startPoint_y + ysmult[3]] = 0;
            // Okay.
        }
        else
        {
            sh[startPoint_x + xs][startPoint_y + ysmult[2]] = 0;
            sh[startPoint_x + xs][startPoint_y + ysmult[3]] = 0;
        }


        sh[startPoint_x + xs][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );

        if ( see && gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[1] - 9] < MINSEE )
          see = false;

      // this was also in the conditional, but meaningless: "&& !behind" {dlb}
        sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[2]] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[2] - 9] : 0 );

// Wider:

        if (gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[2] - 9] >= MINSEE)     //see = false;
          sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[3]] = ( (see) ? gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[3] - 9] : 0 );
        else
          sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[3]] = 0;


        if (gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[2] - 9] >= MINSEE || gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[2] - 9] >= MINSEE)       //see = false;
          sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[3]] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[3] - 9] : 0 );
        else
          sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[3]] = 0;

// Okay.

        //see = see_section;    // why set it here if it set immediately below? {dlb}

// And one more for this section:

        if ( see_section && gr[x_p][y_p + ysmult[2]] < MINSEE && gr[x_p + xs][y_p + ysmult[2]] < MINSEE )
          see_section = false;

        see = see_section;

        sh[startPoint_x][startPoint_y + ysmult[1]] = ( (see) ? gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[1] - 9] : 0 );

        if ( see && gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[1] - 9] < MINSEE )
          see = false;

        if ( see_section && gr[x_p + xs][y_p + ysmult[2]] < MINSEE && gr[x_p + xsmult[1]][y_p + ysmult[2]] < MINSEE )
          see_section = false;

        //see = see_section;    // why set this value twice? {dlb}
        see = see_section;

        if ( see && gr[x_p + xsmult[1]][y_p + ysmult[3]] < MINSEE && gr[x_p + xs][y_p + ysmult[3]] < MINSEE )
          see = false;

        if ((gr[x_p][y_p + ys] < MINSEE && gr[x_p + xs][y_p + ys] < MINSEE) || (gr[x_p][y_p + ysmult[1]] < MINSEE && gr[x_p + xs][y_p + ysmult[1]] < MINSEE) || (gr[x_p][y_p + ysmult[2]] < MINSEE && gr[x_p + xs][y_p + ysmult[2]] < MINSEE))
          sh[startPoint_x][startPoint_y + ysmult[1]] = 0;
        else
          sh[startPoint_x][startPoint_y + ysmult[1]] = gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[1] - 9];


        if ((gr[x_p][y_p + ys] < MINSEE && gr[x_p + xs][y_p + ys] < MINSEE) || (gr[x_p][y_p + ysmult[1]] < MINSEE && gr[x_p + xs][y_p + ysmult[1]] < MINSEE) || (gr[x_p][y_p + ysmult[2]] < MINSEE && gr[x_p + xs][y_p + ysmult[2]] < MINSEE) || (gr[x_p][y_p + ysmult[3]] < MINSEE && gr[x_p + xs][y_p + ysmult[3]] < MINSEE))
        {
            sh[startPoint_x][startPoint_y + ysmult[2]] = 0;
            sh[startPoint_x][startPoint_y + ysmult[3]] = 0;
        }
        else
        {
            sh[startPoint_x][startPoint_y + ysmult[2]] = gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[2] - 9];

            if (gr[x_p][y_p + ysmult[4]] < MINSEE && gr[x_p][y_p + ysmult[4]] < MINSEE)
              sh[startPoint_x][startPoint_y + ysmult[3]] = 0;
            else
              sh[startPoint_x][startPoint_y + ysmult[3]] = gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[3] - 9];
        }

    // These fo the far two layers

        see = true;

        if (sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[3]] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))   //see = false;
                  sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[4]] = ( (see) ? gr[x_p + startPoint_x + xsmult[3] - 9][y_p + startPoint_y + ysmult[4] - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[4]] = 0;
            }
            else
              sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[4]] = 0;
        }
        else
          sh[startPoint_x + xsmult[3]][startPoint_y + ysmult[4]] = 0;


        if (sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[3]] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))   //see = false;
                  sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[4]] = ( (see) ? gr[x_p + startPoint_x + xsmult[2] - 9][y_p + startPoint_y + ysmult[4] - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[4]] = 0;
            }
            else
              sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[4]] = 0;
        }
        else
          sh[startPoint_x + xsmult[2]][startPoint_y + ysmult[4]] = 0;


        //if ( !see ) sh [startPoint_x + 2*xs] [startPoint_y + 6*ys] = 0;
        //if (sh [startPoint_x] [startPoint_y] == 0) see = false;


        if (sh[startPoint_x + xs][startPoint_y + ysmult[3]] != 0)  //see = false;
        {
            if (!(gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))   //see = false;
            {
                if (!(gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))       //see = false;
                  sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[4]] = ( (see) ? gr[x_p + startPoint_x + xsmult[1] - 9][y_p + startPoint_y + ysmult[4] - 9] : 0 );
                else
                  sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[4]] = 0;
            }
            else
              sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[4]] = 0;
        }
        else
          sh[startPoint_x + xsmult[1]][startPoint_y + ysmult[4]] = 0;


       //if ( !see ) sh [startPoint_x + xs] [startPoint_y + 6*ys] = 0;


        if (sh[startPoint_x][startPoint_y + ysmult[3]] != 0)       //see = false;
        {
            if (!(gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))        //see = false;
            {
                if (!(gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))    //see = false;
                  sh[startPoint_x + xs][startPoint_y + ysmult[4]] = ( (see) ? gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[4] - 9] : 0 );
                else
                  sh[startPoint_x + xs][startPoint_y + ysmult[4]] = 0;
            }
            else
              sh[startPoint_x + xs][startPoint_y + ysmult[4]] = 0;
        }
        else
          sh[startPoint_x + xs][startPoint_y + ysmult[4]] = 0;


        if (sh[startPoint_x - xs][startPoint_y + ysmult[3]] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x - xs - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x - xs - 9][y_p + startPoint_y + ysmult[2] - 9] < MINSEE))   //see = false;
                  sh[startPoint_x][startPoint_y + ysmult[4]] = ( (see) ? gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[4] - 9] : 0 );
                else
                  sh[startPoint_x][startPoint_y + ysmult[4]] = 0;
            }
            else
              sh[startPoint_x][startPoint_y + ysmult[4]] = 0;
        }
        else
          sh[startPoint_x][startPoint_y + ysmult[4]] = 0;


        if (sh[startPoint_x][startPoint_y + ysmult[4]] != 0)       //see = false;
        {
            if (!(gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[4] - 9] < MINSEE))        //see = false;
            {
                if (!(gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))    //see = false;
                  sh[startPoint_x + xs][startPoint_y + ysmult[5]] = gr[x_p + startPoint_x + xs - 9][y_p + startPoint_y + ysmult[5] - 9];    //else sh [startPoint_x + 7*xs] [startPoint_y + 0*ys] = 0;
                else
                  sh[startPoint_x + xs][startPoint_y + ysmult[5]] = 0;
            }
            else
              sh[startPoint_x + xs][startPoint_y + ysmult[5]] = 0;
        }
        else
          sh[startPoint_x + xs][startPoint_y + ysmult[5]] = 0;


        if (sh[startPoint_x - xs][startPoint_y + ysmult[4]] != 0)      //see = false;
        {
            if (!(gr[x_p + startPoint_x - xs - 9][y_p + startPoint_y + ysmult[4] - 9] < MINSEE))       //see = false;
            {
                if (!(gr[x_p + startPoint_x - xs - 9][y_p + startPoint_y + ysmult[3] - 9] < MINSEE))   //see = false;
                  sh[startPoint_x][startPoint_y + ysmult[5]] = gr[x_p + startPoint_x - 9][y_p + startPoint_y + ysmult[5] - 9];    //else sh [startPoint_x + 7*xs] [startPoint_y - 1*ys] = 0;
                else
                  sh[startPoint_x][startPoint_y + ysmult[5]] = 0;
            }
            else
              sh[startPoint_x][startPoint_y + ysmult[5]] = 0;
        }
        else
          sh[startPoint_x][startPoint_y + ysmult[5]] = 0;

    }        // end second "for shad"

}          // end losight()




void draw_border( char your_name[kNameLen], char class_name[40], char tspecies )
{

    textcolor(BORDER_COLOR);
// this bit draws the borders:
#ifdef DOS_TERM
    window(1, 1, 80, 25);
#endif

    clrscr();
    gotoxy(40, 1);
    textcolor(LIGHTGREY);
    char print_it[80];
    char print_it2[42];

    int i = 0;
    bool spaces = false;

    strcpy(print_it, your_name);
    strcat(print_it, " the ");
    strcat(print_it, class_name);

    for (i = 0; i < 39; i++)
    {
        print_it2[i] = print_it[i];

        if ( print_it[i] == 0 )
          break;
    }

    for (i = 0; i < 40; i++)
    {
        if ( print_it2[i] == 0 )
          spaces = true;

        if ( spaces )
          print_it2[i] = ' ';
    }

    print_it2[39] = 0;

    textcolor(LIGHTGREY);

#ifdef DOS_TERM
    window(1, 1, 80, 25);
#endif
    gotoxy(40,1);
    textcolor(LIGHTGREY);
    cprintf(print_it2);
    gotoxy(40,2);
    cprintf(species_name(tspecies));
    gotoxy(40,3);
    cprintf("HP:");
    gotoxy(40,4);
    cprintf("Magic:");
    gotoxy(40,5);
    cprintf("AC:");
    gotoxy(40,6);
    cprintf("EV:");
    gotoxy(40,7);
    cprintf("Str:");
    gotoxy(40,8);
    cprintf("Int:");
    gotoxy(40,9);
    cprintf("Dex:");
    gotoxy(40,10);
    cprintf("Gold:");
    gotoxy(40,11);
    cprintf("Experience:");
    gotoxy(40,12);
    cprintf("Level");

}          // end draw_border()




void show_map( int spec_place[2] )
{

    int curs_x = you.x_pos;
    int curs_y = 12;
    int screen_y = you.y_pos;

    int i, j;

    int bufcount2 = 0;

    char move_x = 0;
    char move_y = 0;
    char getty = 0;

#ifdef DOS_TERM

    char buffer[4800];

#endif

    char buffer2[GYM * GXM * 2];    // buffer2[GYM * GXM * 2] segfaults my box {dlb}

    char min_y = 0;
    char max_y = 0;
    char found = 0;
    unsigned char square;


    for (j = 0; j < GYM; j++)
      for (i = 0; i < GXM; i++)
      {
          if ( env.map[i][j] && !found )
          {
              found = 1;
              min_y = j;
          }

          if ( env.map[i][j] )
            max_y = j;
      }

#ifdef DOS_TERM
    gettext(1, 1, 80, 25, buffer);
    window(1, 1, 80, 25);
#endif

    clrscr();
    textcolor(DARKGREY);

put_screen:
    bufcount2 = 0;

#ifdef PLAIN_TERM
    gotoxy(1, 1);
#endif

    for (j = 0; j < NUMBER_OF_LINES; j++)
      for (i = 0; i < 80; i++)
      {
          if ( screen_y + j - 12 >= 65 || screen_y + j - 12 <= 4 )
          {
              buffer2[bufcount2 + 1] = DARKGREY;
              buffer2[bufcount2] = 0;
              bufcount2 += 2;

#ifdef PLAIN_TERM
              goto print_it;
#endif

#ifdef DOS_TERM
              continue;
#endif

          }

          square = grd[i + 1][j + screen_y - 11];
          buffer2[bufcount2 + 1] = colour_code_map(square);

          if ( i == you.x_pos - 1
              && j + screen_y - 11 == you.y_pos )
          {
              buffer2[bufcount2 + 1] = WHITE;
          }

          buffer2[bufcount2] = env.map[i][j + screen_y - 12];
          bufcount2 += 2;

#ifdef PLAIN_TERM

print_it:
          if ( j == NUMBER_OF_LINES - 1 && i == 79 )
            continue;

          if ( i == 79 )
          {
              cprintf(EOL);
              continue;
          }                   /* needed for screens >80 width */

          textcolor(buffer2[bufcount2 - 1]);
          putch(buffer2[bufcount2 - 2]);

#endif

      }

#ifdef DOS_TERM
    puttext(1, 1, 80, 25, buffer2);
#endif

    gotoxy(curs_x, curs_y);

gettything:
    getty = getch();

#ifdef LINUX
    getty = translate_keypad(getty);
    //getty = key_to_command(getty);     // maybe replace with this? {dlb}
#endif

    if (spec_place[0] == 0 && getty != 0 && getty != '+' && getty != '-'
        && getty != 'h' && getty != 'j' && getty != 'k' && getty != 'l'
        && getty != 'y' && getty != 'u' && getty != 'b' && getty != 'n'
        && (getty < '0' || getty > '9'))
        goto putty;

    if (spec_place[0] == 1 && getty != 0 && getty != '+' && getty != '-'
        && getty != 'h' && getty != 'j' && getty != 'k' && getty != 'l'
        && getty != 'y' && getty != 'u' && getty != 'b' && getty != 'n'
        && getty != '.' && getty != 'S' && (getty < '0' || getty > '9'))
        goto gettything;

    if (getty == 0)
      getty = getch();

    switch (getty)
    {
    case 'b':
    case '1':
        move_x = -1;
        move_y = 1;
        break;

    case 'j':
    case '2':
        move_y = 1;
        move_x = 0;
        break;

    case 'u':
    case '9':
        move_x = 1;
        move_y = -1;
        break;

    case 'k':
    case '8':
        move_y = -1;
        move_x = 0;
        break;

    case 'y':
    case '7':
        move_y = -1;
        move_x = -1;
        break;

    case 'h':
    case '4':
        move_x = -1;
        move_y = 0;
        break;

    case 'n':
    case '3':
        move_y = 1;
        move_x = 1;
        break;

    case 'l':
    case '6':
        move_x = 1;
        move_y = 0;
        break;

#ifndef LINUX
        // This is old DOS keypad support
    case 'H':
        move_y = -1;
        move_x = 0;
        break;
    case 'P':
        move_y = 1;
        move_x = 0;
        break;
    case 'K':
        move_x = -1;
        move_y = 0;
        break;
    case 'M':
        move_x = 1;
        move_y = 0;
        break;
    case 'O':
        move_x = -1;
        move_y = 1;
        break;
    case 'I':
        move_x = 1;
        move_y = -1;
        break;
    case 'G':
        move_y = -1;
        move_x = -1;
        break;
    case 'Q':
        move_y = 1;
        move_x = 1;
        break;
#endif

    case '+':
        move_y = 20;
        move_x = 0;
        break;
    case '-':
        move_y = -20;
        move_x = 0;
        break;
    case '.':
    case '\r':
    case 'S':
        spec_place[0] = curs_x;
        spec_place[1] = screen_y + curs_y - 12;
        goto putty;
        break;
    default:
        move_x = 0;
        move_y = 0;
        break;
    }

    if ( curs_x + move_x < 1 || curs_x + move_x > (GXM - 5) )
      move_x = 0;

    curs_x += move_x;

    if (getty == '-' || getty == '+')
    {
        if (getty == '-')
            screen_y -= 20;

        if (screen_y <= 11 + min_y)
            screen_y = 11 + min_y;

        if (getty == '+')
            screen_y += 20;

        if (screen_y >= max_y - 11)
            screen_y = max_y - 11;

        goto put_screen;
    }

    if (curs_y + move_y < 1)
    {
        if (screen_y > 11 + min_y)
        {
            screen_y--;
            goto put_screen;
        }
        else
            move_y = 0;
    }

    if (curs_y + move_y > NUMBER_OF_LINES - 1)
    {
        if (screen_y < max_y - 11)
        {
            screen_y++;
            goto put_screen;
        }
        else
            move_y = 0;
    }

    curs_y += move_y;
    goto put_screen;

putty:

#ifdef DOS_TERM
    puttext(1, 1, 80, 25, buffer);
#endif

    return;

}          // end show_map()




void magic_mapping( int map_radius, int proportion )
{
    int i, j, k, l, empty_count;

    if ( map_radius > 50 )
      map_radius = 50;

    for (i = you.x_pos - map_radius; i < you.x_pos + map_radius; i++)
      for (j = you.y_pos - map_radius; j < you.y_pos + map_radius; j++)
      {
          if (random2(100) > proportion)
            continue;       // note that proportion can be over 100

          if ( i < 5 || j < 5 || i > (GXM - 5) || j > (GYM - 5) )
            continue;

          if ( env.map[i][j] == mapch2(grd[i + 1][j + 1]) )
            continue;

          empty_count = 8;

          if ( grd[i][j] < DNGN_LAVA && grd[i][j] != DNGN_CLOSED_DOOR )
            for (k = 0; k < 3; k++)
              for (l = 0; l < 3; l++)
              {
                  if ( k == 1 && l == 1 )
                    continue;

                  if ( grd[i + k][j + l] <= 60 && grd[i + k][j + l] != DNGN_CLOSED_DOOR )
                    empty_count--;
               }

          if ( empty_count > 0 )
            env.map[i][j] = mapch(grd[i + 1][j + 1]);
      }


}          // end magic_mapping()




/* mapchars 3 & 4 are for non-ibm char sets */

unsigned char mapchar(unsigned char ldfk)
{
    unsigned char showed = 0;

    switch (ldfk)
    {
    case DNGN_UNSEEN:
        showed = 0;
        break;

    case DNGN_SECRET_DOOR:
    case DNGN_ROCK_WALL:
    case DNGN_STONE_WALL:
    case DNGN_METAL_WALL:
    case DNGN_GREEN_CRYSTAL_WALL:
    case DNGN_WAX_WALL:
        showed = 176;
        break;


    case DNGN_CLOSED_DOOR:
        showed = 206;
        break;

    case 20:                    // orcish idol
    case 24:                    // ???
    case 25:                    // ???
    case DNGN_SILVER_STATUE:
    case DNGN_GRANITE_STATUE:
    case DNGN_ORANGE_CRYSTAL_STATUE:
        showed = '8';
        break;

    case DNGN_LAVA_X:
    case DNGN_WATER_X:
    case DNGN_LAVA:
    case DNGN_DEEP_WATER:
    case DNGN_SHALLOW_WATER:
        showed = 247;
        break;

    case DNGN_FLOOR:
    case DNGN_UNDISCOVERED_TRAP:
        showed = 250;
        break;

    //case 68: showed = '>'; break; // < (60)

    case DNGN_OPEN_DOOR:
        showed = 39;
        break;

    //case 72: showed = '<'; break;

    case DNGN_TRAP_MECHANICAL:
    case DNGN_TRAP_MAGICAL:
    case DNGN_TRAP_III:
        showed = '^';
        break;

    case DNGN_STONE_STAIRS_DOWN_I:
    case DNGN_STONE_STAIRS_DOWN_II:
    case DNGN_STONE_STAIRS_DOWN_III:
    case DNGN_ROCK_STAIRS_DOWN:
    case DNGN_ENTER_ORCISH_MINES:
    case DNGN_ENTER_HIVE:
    case DNGN_ENTER_LAIR_I:
    case DNGN_ENTER_SLIME_PITS:
    case DNGN_ENTER_VAULTS:
    case DNGN_ENTER_CRYPT_I:
    case DNGN_ENTER_HALL_OF_BLADES:
    case DNGN_ENTER_TEMPLE:
    case DNGN_ENTER_SNAKE_PIT:
    case DNGN_ENTER_ELVEN_HALLS:
    case DNGN_ENTER_TOMB:
    case DNGN_ENTER_SWAMP:
    case 123:
    case 124:
    case 125:
    case 126:
        showed = '>';
        break;

    case DNGN_STONE_STAIRS_UP_I:
    case DNGN_STONE_STAIRS_UP_II:
    case DNGN_STONE_STAIRS_UP_III:
    case DNGN_ROCK_STAIRS_UP:
    case DNGN_RETURN_DUNGEON_I:
    case DNGN_RETURN_DUNGEON_II:
    case DNGN_RETURN_DUNGEON_III:
    case DNGN_RETURN_LAIR_II:
    case DNGN_RETURN_DUNGEON_IV:
    case DNGN_RETURN_VAULTS:
    case DNGN_RETURN_CRYPT_II:
    case DNGN_RETURN_DUNGEON_V:
    case DNGN_RETURN_LAIR_III:
    case DNGN_RETURN_MINES:
    case DNGN_RETURN_CRYPT_III:
    case DNGN_RETURN_LAIR_IV:
    case 143:
    case 144:
    case 145:
    case 146:
        showed = '<';
        break;

    case DNGN_ENTER_HELL:
    case DNGN_ENTER_LABYRINTH:
    case DNGN_ENTER_SHOP:
    case DNGN_ENTER_DIS:
    case DNGN_ENTER_GEHENNA:
    case DNGN_ENTER_COCYTUS:
    case DNGN_ENTER_TARTARUS:
    case DNGN_ENTER_ABYSS:
    case DNGN_EXIT_ABYSS:
    case DNGN_STONE_ARCH:
    case DNGN_ENTER_PANDEMONIUM:
    case DNGN_EXIT_PANDEMONIUM:
    case DNGN_TRANSIT_PANDEMONIUM:
    case DNGN_ENTER_ZOT:
    case DNGN_EXIT_ZOT:
        showed = 239;
        break;

    case DNGN_ALTAR_ZIN:
    case DNGN_ALTAR_SHINING_ONE:
    case DNGN_ALTAR_KIKUBAAQUDGHA:
    case DNGN_ALTAR_YREDELEMNUL:
    case DNGN_ALTAR_XOM:
    case DNGN_ALTAR_VEHUMET:
    case DNGN_ALTAR_OKAWARU:
    case DNGN_ALTAR_MAKHLEB:
    case DNGN_ALTAR_SIF_MUNA:
    case DNGN_ALTAR_TROG:
    case DNGN_ALTAR_NEMELEX_XOBEH:
    case DNGN_ALTAR_ELYVILON:
        showed = 220;
        break;

    case DNGN_BLUE_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_I:
    case DNGN_SPARKLING_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_II:
    case DNGN_DRY_FOUNTAIN_III:
    case DNGN_DRY_FOUNTAIN_IV:
    case DNGN_DRY_FOUNTAIN_V:
    case DNGN_DRY_FOUNTAIN_VI:
    case DNGN_DRY_FOUNTAIN_VII:
    case DNGN_DRY_FOUNTAIN_VIII:
    case DNGN_PERMADRY_FOUNTAIN:
        showed = 159;
        break;

    default:
        showed = 0;
        break;
    }

    return showed;

}

unsigned char mapchar2(unsigned char ldfk)
{
    unsigned char showed = 0;

    switch (ldfk)
    {
    case DNGN_UNSEEN:
        showed = 0;
        break;

    case DNGN_SECRET_DOOR:
    case DNGN_ROCK_WALL:
    case DNGN_STONE_WALL:
    case DNGN_METAL_WALL:
    case DNGN_GREEN_CRYSTAL_WALL:
    case DNGN_WAX_WALL:
        showed = 177;
        break;

    case DNGN_CLOSED_DOOR:
        showed = 254;
        break;

    //case DNGN_LAVA_X: showed = 247; break;     // deprecated? {dlb}
    //case DNGN_WATER_X: showed = 247; break;    // deprecated? {dlb}

    case 20:                    // orcish idol
    case 24:                    // ???
    case 25:                    // ???
    case DNGN_SILVER_STATUE:
    case DNGN_GRANITE_STATUE:
    case DNGN_ORANGE_CRYSTAL_STATUE:
        showed = '8';
        break;

    case DNGN_LAVA:
    case DNGN_DEEP_WATER:
    case DNGN_SHALLOW_WATER:
        showed = 247;
        break;

    case DNGN_FLOOR:
    case DNGN_UNDISCOVERED_TRAP:
        showed = 249;
        break;

    case 68:
        showed = '>';
        break;                  // <

    case DNGN_OPEN_DOOR:
        showed = 39;
        break;

    case 72:
        showed = '<';
        break;                  // <

    case DNGN_TRAP_MECHANICAL:
    case DNGN_TRAP_MAGICAL:
    case DNGN_TRAP_III:
        showed = '^';
        break;

    case DNGN_STONE_STAIRS_DOWN_I:
    case DNGN_STONE_STAIRS_DOWN_II:
    case DNGN_STONE_STAIRS_DOWN_III:
    case DNGN_ROCK_STAIRS_DOWN:
    case DNGN_ENTER_ORCISH_MINES:
    case DNGN_ENTER_HIVE:
    case DNGN_ENTER_LAIR_I:
    case DNGN_ENTER_SLIME_PITS:
    case DNGN_ENTER_VAULTS:
    case DNGN_ENTER_CRYPT_I:
    case DNGN_ENTER_HALL_OF_BLADES:
    case DNGN_ENTER_TEMPLE:
    case DNGN_ENTER_SNAKE_PIT:
    case DNGN_ENTER_ELVEN_HALLS:
    case DNGN_ENTER_TOMB:
    case DNGN_ENTER_SWAMP:
    case 123:
    case 124:
    case 125:
    case 126:
        showed = '>';
        break;

    case DNGN_STONE_STAIRS_UP_I:
    case DNGN_STONE_STAIRS_UP_II:
    case DNGN_STONE_STAIRS_UP_III:
    case DNGN_ROCK_STAIRS_UP:
    case DNGN_RETURN_DUNGEON_I:
    case DNGN_RETURN_DUNGEON_II:
    case DNGN_RETURN_DUNGEON_III:
    case DNGN_RETURN_LAIR_II:
    case DNGN_RETURN_DUNGEON_IV:
    case DNGN_RETURN_VAULTS:
    case DNGN_RETURN_CRYPT_II:
    case DNGN_RETURN_DUNGEON_V:
    case DNGN_RETURN_LAIR_III:
    case DNGN_RETURN_MINES:
    case DNGN_RETURN_CRYPT_III:
    case DNGN_RETURN_LAIR_IV:
    case 143:
    case 144:
    case 145:
    case 146:
        showed = '<';
        break;

    case DNGN_ENTER_HELL:
    case DNGN_ENTER_LABYRINTH:
    case DNGN_ENTER_SHOP:
    case DNGN_ENTER_DIS:
    case DNGN_ENTER_GEHENNA:
    case DNGN_ENTER_COCYTUS:
    case DNGN_ENTER_TARTARUS:
    case DNGN_ENTER_ABYSS:
    case DNGN_EXIT_ABYSS:
    case DNGN_STONE_ARCH:
    case DNGN_ENTER_PANDEMONIUM:
    case DNGN_EXIT_PANDEMONIUM:
    case DNGN_TRANSIT_PANDEMONIUM:
    case DNGN_ENTER_ZOT:
    case DNGN_EXIT_ZOT:
        showed = 239;
        break;

    case DNGN_ALTAR_ZIN:
    case DNGN_ALTAR_SHINING_ONE:
    case DNGN_ALTAR_KIKUBAAQUDGHA:
    case DNGN_ALTAR_YREDELEMNUL:
    case DNGN_ALTAR_XOM:
    case DNGN_ALTAR_VEHUMET:
    case DNGN_ALTAR_OKAWARU:
    case DNGN_ALTAR_MAKHLEB:
    case DNGN_ALTAR_SIF_MUNA:
    case DNGN_ALTAR_TROG:
    case DNGN_ALTAR_NEMELEX_XOBEH:
    case DNGN_ALTAR_ELYVILON:
        showed = 220;
        break;

    case DNGN_BLUE_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_I:
    case DNGN_SPARKLING_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_II:
    case DNGN_DRY_FOUNTAIN_III:
    case DNGN_DRY_FOUNTAIN_IV:
    case DNGN_DRY_FOUNTAIN_V:
    case DNGN_DRY_FOUNTAIN_VI:
    case DNGN_DRY_FOUNTAIN_VII:
    case DNGN_DRY_FOUNTAIN_VIII:
    case DNGN_PERMADRY_FOUNTAIN:
        showed = 159;
        break;
    default:
        showed = 0;
        break;
    }

    return showed;

}




// realize that this is simply a repackaged version of
// stuff::see_grid() -- make certain they correlate {dlb}:
bool mons_near( struct monsters *monster )
{

    if ( monster->x > you.x_pos - 9 && monster->x < you.x_pos + 9
        && monster->y > you.y_pos - 9 && monster->y < you.y_pos + 9 )
    {
        if ( env.show[monster->x - you.x_pos + 9][monster->y - you.y_pos + 9] )
          return true;
    }

    return false;

}          // end mons_near()




//---------------------------------------------------------------
//
// get_non_ibm_symbol
//
// Returns the character code and color for everything drawn
// without the IBM graphics option.
//
//---------------------------------------------------------------
static void get_non_ibm_symbol(unsigned int object, unsigned char *ch, unsigned char *color)
{
    ASSERT(color != NULL);
    ASSERT(ch != NULL);

    switch (object)
    {

    case DNGN_UNSEEN:
        *ch = 0;
        break;

    case DNGN_ROCK_WALL:
        *color = env.rock_colour;
        *ch = '#';
        break;

    case DNGN_STONE_WALL:
        if ( you.where_are_you == BRANCH_HALL_OF_ZOT )
          *color = env.rock_colour;
        else
          *color = LIGHTGREY;
        *ch = '#';
        break;

    case DNGN_CLOSED_DOOR:
        *ch = '+';
        break;

    case DNGN_METAL_WALL:
        *ch = '#';
        *color = CYAN;
        break;

    case DNGN_SECRET_DOOR:
        *ch = '#';
        *color = env.rock_colour;
        break;

    case DNGN_GREEN_CRYSTAL_WALL:
        *ch = '#';
        *color = GREEN;
        break;

    case DNGN_ORCISH_IDOL:
        *ch = '8';
        *color = DARKGREY;

        break;

    case DNGN_WAX_WALL:
        *ch = '#';
        *color = YELLOW;
        break;

    case DNGN_SILVER_STATUE:
        *ch = '8';
        *color = WHITE;
        if ( visible[1] == 0 )
          visible[1] = 3;
        else
          visible[1] = 2;
        visible[0] = 2;
        break;

    case DNGN_GRANITE_STATUE:
        *ch = '8';
        *color = LIGHTGREY;
        break;

    case DNGN_ORANGE_CRYSTAL_STATUE:
        *ch = '8';
        *color = LIGHTRED;
        if ( visible[2] == 0 )
          visible[2] = 3;
        else
          visible[2] = 2;
        visible[0] = 2;
        break;

    case DNGN_STATUE_35:
        *ch = '#';
        break;

    case DNGN_LAVA:
        *ch = '{';
        *color = RED;
        break;

    case DNGN_DEEP_WATER:
        *ch = '{';              // this wavy thing also used for water elemental
        // note that some monsters which use IBM graphics aren't set for this function - too tricky for now.
        *color = BLUE;
        break;

    case DNGN_SHALLOW_WATER:
        *color = CYAN;
        *ch = '{';
        break;

    case DNGN_FLOOR:
        *color = env.floor_colour;
        *ch = '.';
        break;

    case DNGN_ENTER_HELL:
        *color = RED;
        *ch = '\\';
        seen_other_thing(DNGN_ENTER_HELL);
        break;

    case DNGN_OPEN_DOOR:
        *ch = 39;
        break;

    case DNGN_BRANCH_STAIRS:
        *color = BROWN;
        *ch = '>';
        break;

    case DNGN_TRAP_MECHANICAL:
        *color = 11;
        *ch = '^';
        break;

    case DNGN_TRAP_MAGICAL:
        *color = MAGENTA;
        *ch = '^';
        break;

    case DNGN_TRAP_III:
        *color = LIGHTGREY;
        *ch = '^';
        break;

    case DNGN_UNDISCOVERED_TRAP:
        *color = env.floor_colour;
        *ch = '.';
        break;

    case DNGN_ENTER_SHOP:
        *color = YELLOW;
        *ch = '\\';
        seen_other_thing(DNGN_ENTER_SHOP);
        break;
// if I change anything above here, must also change magic mapping!

    case DNGN_ENTER_LABYRINTH:
        *color = LIGHTGREY;
        *ch = '\\';
        seen_other_thing(DNGN_ENTER_LABYRINTH);
        break;

    case DNGN_ROCK_STAIRS_DOWN:
        *color = BROWN;         // ladder
    case DNGN_STONE_STAIRS_DOWN_I:
    case DNGN_STONE_STAIRS_DOWN_II:
    case DNGN_STONE_STAIRS_DOWN_III:
        *ch = '>';
        break;

    case DNGN_ROCK_STAIRS_UP:
        *color = BROWN;         // ladder
    case DNGN_STONE_STAIRS_UP_I:
    case DNGN_STONE_STAIRS_UP_II:
    case DNGN_STONE_STAIRS_UP_III:
        *ch = '<';
        break;

    case DNGN_ENTER_DIS:
        *color = CYAN;
        *ch = '\\';
        break;

    case DNGN_ENTER_GEHENNA:
        *color = RED;
        *ch = '\\';
        break;

    case DNGN_ENTER_COCYTUS:
        *color = LIGHTCYAN;
        *ch = '\\';
        break;

    case DNGN_ENTER_TARTARUS:
        *color = DARKGREY;
        *ch = '\\';
        break;

    case DNGN_ENTER_ABYSS:
        *color = random2(16);
        *ch = '\\';
        seen_other_thing(DNGN_ENTER_ABYSS);
        break;

    case DNGN_EXIT_ABYSS:
        *color = random2(16);
        *ch = '\\';
        break;

    case DNGN_STONE_ARCH:
        *color = LIGHTGREY;
        *ch = '\\';
        break;

    case DNGN_ENTER_PANDEMONIUM:
        *color = LIGHTBLUE;
        *ch = '\\';
        seen_other_thing(DNGN_ENTER_PANDEMONIUM);
        break;

    case DNGN_EXIT_PANDEMONIUM:
        *color = LIGHTBLUE;
        *ch = '\\';
        break;

    case DNGN_TRANSIT_PANDEMONIUM:
        *color = LIGHTGREEN;
        *ch = '\\';
        break;                  // gate to other part of pandemonium

    case DNGN_ENTER_ORCISH_MINES:
    case DNGN_ENTER_HIVE:
    case DNGN_ENTER_LAIR_I:
    case DNGN_ENTER_SLIME_PITS:
    case DNGN_ENTER_VAULTS:
    case DNGN_ENTER_CRYPT_I:
    case DNGN_ENTER_HALL_OF_BLADES:
    case DNGN_ENTER_TEMPLE:
    case DNGN_ENTER_SNAKE_PIT:
    case DNGN_ENTER_ELVEN_HALLS:
    case DNGN_ENTER_TOMB:
    case DNGN_ENTER_SWAMP:
    case 123:
    case 124:
    case 125:
    case 126:
        *color = YELLOW;
        *ch = '>';
        seen_staircase(object);
        break;

    case DNGN_ENTER_ZOT:
        *color = MAGENTA;
        *ch = '\\';
        seen_staircase(object);
        break;

    case DNGN_RETURN_DUNGEON_I:
    case DNGN_RETURN_DUNGEON_II:
    case DNGN_RETURN_DUNGEON_III:
    case DNGN_RETURN_LAIR_II:
    case DNGN_RETURN_DUNGEON_IV:
    case DNGN_RETURN_VAULTS:
    case DNGN_RETURN_CRYPT_II:
    case DNGN_RETURN_DUNGEON_V:
    case DNGN_RETURN_LAIR_III:
    case DNGN_RETURN_MINES:
    case DNGN_RETURN_CRYPT_III:
    case DNGN_RETURN_LAIR_IV:
    case 143:
    case 144:
    case 145:
    case 146:
        *color = YELLOW;
        *ch = '<';
        break;

    case DNGN_EXIT_ZOT:
        *color = MAGENTA;
        *ch = '\\';
        break;

    case DNGN_ALTAR_ZIN:
        *color = WHITE;
        *ch = '_';
        seen_altar(GOD_ZIN);
        break;

    case DNGN_ALTAR_SHINING_ONE:
        *color = YELLOW;
        *ch = '_';
        seen_altar(GOD_SHINING_ONE);
        break;

    case DNGN_ALTAR_KIKUBAAQUDGHA:
        *color = DARKGREY;
        *ch = '_';
        seen_altar(GOD_KIKUBAAQUDGHA);
        break;

    case DNGN_ALTAR_YREDELEMNUL:
        *color = DARKGREY;
        if (one_chance_in(3))
            *color = RED;
        *ch = '_';
        seen_altar(GOD_YREDELEMNUL);
        break;

    case DNGN_ALTAR_XOM:
        *color = random_colour();
        *ch = '_';
        seen_altar(GOD_XOM);
        break;

    case DNGN_ALTAR_VEHUMET:
        *color = LIGHTBLUE;
        if (one_chance_in(3))
            *color = LIGHTMAGENTA;
        if (one_chance_in(3))
            *color = LIGHTRED;
        *ch = '_';
        seen_altar(GOD_VEHUMET);
        break;

    case DNGN_ALTAR_OKAWARU:
        *color = CYAN;
        *ch = '_';
        seen_altar(GOD_OKAWARU);
        break;

    case DNGN_ALTAR_MAKHLEB:
        *color = RED;
        if (one_chance_in(3))
            *color = LIGHTRED;
        if (one_chance_in(3))
            *color = YELLOW;
        *ch = '_';
        seen_altar(GOD_MAKHLEB);
        break;

    case DNGN_ALTAR_SIF_MUNA:
        *color = BLUE;
        *ch = '_';
        seen_altar(GOD_SIF_MUNA);
        break;

    case DNGN_ALTAR_TROG:
        *color = RED;
        *ch = '_';
        seen_altar(GOD_TROG);
        break;

    case DNGN_ALTAR_NEMELEX_XOBEH:
        *color = LIGHTMAGENTA;
        *ch = '_';
        seen_altar(GOD_NEMELEX_XOBEH);
        break;

    case DNGN_ALTAR_ELYVILON:
        *color = LIGHTGREY;
        *ch = '_';
        seen_altar(GOD_ELYVILON);
        break;

    case DNGN_BLUE_FOUNTAIN:
        *color = BLUE;
        *ch = '}';
        break;

    case DNGN_SPARKLING_FOUNTAIN:
        *color = LIGHTBLUE;
        *ch = '}';
        break;

    case DNGN_DRY_FOUNTAIN_I:
    case DNGN_DRY_FOUNTAIN_II:
    case DNGN_PERMADRY_FOUNTAIN:
        *color = LIGHTGREY;
        *ch = '}';
        break;

    case 256:
        *ch = '0';
        break;

    case 257:
        *color = CYAN;
        *ch = '~';
        break;                  /* Invis creature walking through water */

    case 258:
        *ch = ')';
        break;                  // weapon )

    case 259:
        *ch = '[';
        break;                  // armour [

    case 260:
        *ch = '/';
        break;                  // wands, etc.

    case 261:
        *ch = '%';
        break;                  // food

    case 262:
        *ch = '+';
        break;                  // books +

    case 263:
        *ch = '?';
        break;                  // scroll ?

    case 264:
        *ch = '=';
        break;                  // ring = etc

    case 265:
        *ch = '!';
        break;                  // potions !

    case 266:
        *ch = '(';
        break;                  // stones

    case 267:
        *ch = ':';
        break;                  // book +

    case 268:
        *ch = '%';
        break;                  // corpses part 1

    case 269:
        *ch = '|';
        break;                  // magical staves

    case 270:
        *ch = '}';
        break;                  // gems

    case 271:
        *ch = '%';
        break;                  // don't know ?

    case 272:
        *ch = '$';
        break;                  // $ gold

    case 273:
        *ch = '"';
        break;                  // amulet

    default:
        int mnr = object;
        *ch = ( (mnr >= 297) ? mons_char(mnr - 297) : object );         // yeah
        break;


    }
}




/*
   This is the viewwindow function for computers without IBM graphic displays.
   It is activated by a command line argument, which sets a function pointer.
 */
void viewwindow3( char draw_it, bool do_updates )
{

    int bufcount = 0;
    unsigned char buffy[1500];  //[800]; //392];

    unsigned char showed = 0;    // presently unused ... I think {dlb}
    unsigned char ch, color;

    int count_x, count_y;

    _setcursortype(_NOCURSOR);

    losight(env.show, grd, you.x_pos, you.y_pos);


    for (count_x = 0; count_x < 18; count_x++)
      for (count_y = 0; count_y < 18; count_y++)
      {
          env.show_col[count_x][count_y] = LIGHTGREY;
          show_backup[count_x][count_y] = 0;
      }

    item();
    cloud_grid();
    monster_grid(do_updates);
    bufcount = 0;


    if ( draw_it == 1 )
    {
        for (count_y = (you.y_pos - 8); (count_y < you.y_pos + 9); count_y++)
        {
            bufcount += 16;

            for (count_x = (you.x_pos - 8); (count_x < you.x_pos + 9); count_x++)
            {
                //buffy[bufcount + 1] = env.show_col[count_x - you.x_pos + 9][count_y - you.y_pos + 9];

                color = env.show_col[count_x - you.x_pos + 9][count_y - you.y_pos + 9];

                if (count_x != you.x_pos || count_y != you.y_pos)
                {

                    unsigned int object = env.show[count_x - you.x_pos + 9][count_y - you.y_pos + 9];

                    get_non_ibm_symbol(object, &ch, &color);

                }
                else
                {
                    ch = your_sign;
                    buffy[bufcount + 1] = color;
                }


                buffy[bufcount] = ch;   //showed;

                buffy[bufcount + 1] = color;

                bufcount += 2;
            }
            bufcount += 16;
        }
        bufcount = 0;

        if (you.level_type != LEVEL_LABYRINTH && you.level_type != LEVEL_ABYSS)
        {
            for (count_y = 0; count_y < 17; count_y++)
            {
                bufcount += 16;
                for (count_x = 0; count_x < 17; count_x++)
                {
                    if (buffy[bufcount] != 0)
                        env.map[count_x + you.x_pos - 9][count_y + you.y_pos - 9] = buffy[bufcount];
                    if (clean_map == 1 && show_backup[count_x + 1][count_y + 1] != 0)
                    {
                        get_non_ibm_symbol(show_backup[count_x + 1][count_y + 1], &ch, &color);
                        env.map[count_x + you.x_pos - 9][count_y + you.y_pos - 9] = ch;
                    }
                    bufcount += 2;
                }
                bufcount += 16;
            }
        }

        bufcount = 0;

        for (count_y = 0; count_y < 17; count_y++)
          for (count_x = 0; count_x < 33; count_x++)
          {
              if ( count_x + you.x_pos - 17 < 3 || count_y + you.y_pos - 9 < 3 || count_x + you.x_pos - 14 > (GXM - 3) || count_y + you.y_pos - 9 > (GYM - 3) )
              {
                  buffy[bufcount] = 0;
                  bufcount++;
                  buffy[bufcount] = 0;
                  bufcount++;
                  continue;
              }

              if (count_x >= 8 && count_x <= 24 && count_y >= 0 && count_y <= 16 && buffy[bufcount] != 0)
              {
                  bufcount += 2;
                  continue;
              }

              buffy[bufcount] = env.map[count_x + you.x_pos - 17][count_y + you.y_pos - 9];
              buffy[bufcount + 1] = DARKGREY;

              if (colour_map)
                if (env.map[count_x + you.x_pos - 16][count_y + you.y_pos - 8] != 0)
                  buffy[bufcount + 1] = colour_code_map(grd[count_x + you.x_pos - 16][count_y + you.y_pos - 8]);

              bufcount += 2;
          }

        if ( you.berserker )
          for (count_x = 1; count_x < 1400; count_x += 2)
            if ( buffy[count_x] != DARKGREY )
              buffy[count_x] = RED;

        if ( show_green != BLACK )
        {
            for (count_x = 1; count_x < 1400; count_x += 2)
              if ( buffy[count_x] != DARKGREY )
                buffy[count_x] = show_green;

            show_green = ( (you.special_wield == SPWLD_SHADOW) ? DARKGREY : BLACK );
        }

#ifdef DOS_TERM
        puttext(2, 1, 34, 17, buffy);
#endif

#ifdef PLAIN_TERM
        gotoxy(2, 1);
        bufcount = 0;

        if ( !you.running )   // this line is purely optional
        {
            for (count_x = 0; count_x < 1120; count_x += 2)     // 1056
            {
                textcolor(buffy[count_x + 1]);
                putch(buffy[count_x]);

                if (count_x % 66 == 64 && count_x > 0)
#ifdef DOS_TERM
                    cprintf(EOL " ");
#endif

#ifdef PLAIN_TERM
                gotoxy(2, wherey() + 1);
#endif

            }
        }
#endif

    }                           // end of (if brek...)

}          // end viewwindow3()




unsigned char mapchar3(unsigned char ldfk)
{
    unsigned char showed = 0;

    switch ( ldfk )
    {
    case DNGN_UNSEEN:
        showed = 0;
        break;

    case DNGN_SECRET_DOOR:
    case DNGN_ROCK_WALL:
    case DNGN_STONE_WALL:
    case DNGN_METAL_WALL:
    case DNGN_GREEN_CRYSTAL_WALL:
    case DNGN_WAX_WALL:
        showed = '*';
        break;

    case DNGN_CLOSED_DOOR:
        showed = '+';
        break;

    case 20:                    // orcish idol
    case 24:                    // ???
    case 25:                    // ???
    case DNGN_SILVER_STATUE:
    case DNGN_GRANITE_STATUE:
    case DNGN_ORANGE_CRYSTAL_STATUE:
        showed = '8';
        break;

    case DNGN_LAVA_X:
    case DNGN_WATER_X:
    case DNGN_LAVA:
    case DNGN_DEEP_WATER:
    case DNGN_SHALLOW_WATER:
        showed = '{';
        break;

    case DNGN_FLOOR:
    case DNGN_UNDISCOVERED_TRAP:
        showed = ',';
        break;

    //case 68: showed = '>'; break; // < (60)


    case DNGN_OPEN_DOOR:
        showed = 39;
        break;                  // open door

    //case 72: showed = '<'; break;

    case DNGN_TRAP_MECHANICAL:
    case DNGN_TRAP_MAGICAL:
    case DNGN_TRAP_III:
        showed = '^';
        break;

    case DNGN_STONE_STAIRS_DOWN_I:
    case DNGN_STONE_STAIRS_DOWN_II:
    case DNGN_STONE_STAIRS_DOWN_III:
    case DNGN_ROCK_STAIRS_DOWN:
    case DNGN_ENTER_ORCISH_MINES :
    case DNGN_ENTER_HIVE:
    case DNGN_ENTER_LAIR_I:
    case DNGN_ENTER_SLIME_PITS:
    case DNGN_ENTER_VAULTS:
    case DNGN_ENTER_CRYPT_I:
    case DNGN_ENTER_HALL_OF_BLADES:
    case DNGN_ENTER_TEMPLE:
    case DNGN_ENTER_SNAKE_PIT:
    case DNGN_ENTER_ELVEN_HALLS:
    case DNGN_ENTER_TOMB:
    case DNGN_ENTER_SWAMP:
    case 123:
    case 124:
    case 125:
    case 126:
        showed = '>';
        break;

    case DNGN_STONE_STAIRS_UP_I:
    case DNGN_STONE_STAIRS_UP_II:
    case DNGN_STONE_STAIRS_UP_III:
    case DNGN_ROCK_STAIRS_UP:
    case DNGN_RETURN_DUNGEON_I:
    case DNGN_RETURN_DUNGEON_II:
    case DNGN_RETURN_DUNGEON_III:
    case DNGN_RETURN_LAIR_II:
    case DNGN_RETURN_DUNGEON_IV:
    case DNGN_RETURN_VAULTS:
    case DNGN_RETURN_CRYPT_II:
    case DNGN_RETURN_DUNGEON_V:
    case DNGN_RETURN_LAIR_III:
    case DNGN_RETURN_MINES:
    case DNGN_RETURN_CRYPT_III:
    case DNGN_RETURN_LAIR_IV:
    case 143:
    case 144:
    case 145:
    case 146:
        showed = '<';
        break;

    case DNGN_ENTER_HELL:
    case DNGN_ENTER_LABYRINTH:
    case DNGN_ENTER_SHOP:
    case DNGN_ENTER_DIS:
    case DNGN_ENTER_GEHENNA:
    case DNGN_ENTER_COCYTUS:
    case DNGN_ENTER_TARTARUS:
    case DNGN_ENTER_ABYSS:
    case DNGN_EXIT_ABYSS:
    case DNGN_STONE_ARCH:
    case DNGN_ENTER_PANDEMONIUM:
    case DNGN_EXIT_PANDEMONIUM:
    case DNGN_TRANSIT_PANDEMONIUM:
    case DNGN_ENTER_ZOT:
    case DNGN_EXIT_ZOT:
        showed = '\\';
        break;

    case DNGN_ALTAR_ZIN:
    case DNGN_ALTAR_SHINING_ONE:
    case DNGN_ALTAR_KIKUBAAQUDGHA:
    case DNGN_ALTAR_YREDELEMNUL:
    case DNGN_ALTAR_XOM:
    case DNGN_ALTAR_VEHUMET:
    case DNGN_ALTAR_OKAWARU:
    case DNGN_ALTAR_MAKHLEB:
    case DNGN_ALTAR_SIF_MUNA:
    case DNGN_ALTAR_TROG:
    case DNGN_ALTAR_NEMELEX_XOBEH:
    case DNGN_ALTAR_ELYVILON:
        showed = '_';
        break;

    case DNGN_BLUE_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_I:
    case DNGN_SPARKLING_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_II:
    case DNGN_DRY_FOUNTAIN_III:
    case DNGN_DRY_FOUNTAIN_IV:
    case DNGN_DRY_FOUNTAIN_V:
    case DNGN_DRY_FOUNTAIN_VI:
    case DNGN_DRY_FOUNTAIN_VII:
    case DNGN_DRY_FOUNTAIN_VIII:
    case DNGN_PERMADRY_FOUNTAIN:
        showed = '}';
        break;

    default:
        showed = 0;
        break;
    }

    return showed;

}




unsigned char mapchar4( unsigned char ldfk )
{
    unsigned char showed = 0;

    switch (ldfk)
    {
    case DNGN_UNSEEN:
        showed = 0;
        break;

    case DNGN_CLOSED_DOOR:
        showed = '+';
        break;

    case DNGN_SECRET_DOOR:
    case DNGN_ROCK_WALL:
    case DNGN_STONE_WALL:
    case DNGN_METAL_WALL:
    case DNGN_GREEN_CRYSTAL_WALL:
    case DNGN_WAX_WALL:
        showed = '#';
        break;

    case 20:                    // orcish idol
    case 24:                    // ???
    case 25:                    // ???
    case DNGN_SILVER_STATUE:
    case DNGN_GRANITE_STATUE:
    case DNGN_ORANGE_CRYSTAL_STATUE:
        showed = '8';
        break;

    case DNGN_LAVA_X:
    case DNGN_WATER_X:
    case DNGN_LAVA:
    case DNGN_DEEP_WATER:
    case DNGN_SHALLOW_WATER:
        showed = '{';
        break;

    case DNGN_FLOOR:
    case DNGN_UNDISCOVERED_TRAP:
        showed = '.';
        break;

    case 68:
        showed = '>';                  // <
        break;

    case DNGN_OPEN_DOOR:
        showed = 39;
        break;

    case 72:
        showed = '<';
        break;

    case DNGN_TRAP_MECHANICAL:
    case DNGN_TRAP_MAGICAL:
    case DNGN_TRAP_III:
        showed = '^';
        break;

    case DNGN_STONE_STAIRS_DOWN_I:
    case DNGN_STONE_STAIRS_DOWN_II:
    case DNGN_STONE_STAIRS_DOWN_III:
    case DNGN_ROCK_STAIRS_DOWN:
    case DNGN_ENTER_ORCISH_MINES:
    case DNGN_ENTER_HIVE:
    case DNGN_ENTER_LAIR_I:
    case DNGN_ENTER_SLIME_PITS:
    case DNGN_ENTER_VAULTS:
    case DNGN_ENTER_CRYPT_I:
    case DNGN_ENTER_HALL_OF_BLADES:
    case DNGN_ENTER_TEMPLE:
    case DNGN_ENTER_SNAKE_PIT:
    case DNGN_ENTER_ELVEN_HALLS:
    case DNGN_ENTER_TOMB:
    case DNGN_ENTER_SWAMP:
    case 123:
    case 124:
    case 125:
    case 126:
        showed = '>';
        break;

    case DNGN_STONE_STAIRS_UP_I:
    case DNGN_STONE_STAIRS_UP_II:
    case DNGN_STONE_STAIRS_UP_III:
    case DNGN_ROCK_STAIRS_UP:
    case DNGN_RETURN_DUNGEON_I:
    case DNGN_RETURN_DUNGEON_II:
    case DNGN_RETURN_DUNGEON_III:
    case DNGN_RETURN_LAIR_II:
    case DNGN_RETURN_DUNGEON_IV:
    case DNGN_RETURN_VAULTS:
    case DNGN_RETURN_CRYPT_II:
    case DNGN_RETURN_DUNGEON_V:
    case DNGN_RETURN_LAIR_III:
    case DNGN_RETURN_MINES:
    case DNGN_RETURN_CRYPT_III:
    case DNGN_RETURN_LAIR_IV:
    case 143:
    case 144:
    case 145:
    case 146:
        showed = '<';
        break;

    case DNGN_ENTER_HELL:
    case DNGN_ENTER_LABYRINTH:
    case DNGN_ENTER_SHOP:
    case DNGN_ENTER_DIS:
    case DNGN_ENTER_GEHENNA:
    case DNGN_ENTER_COCYTUS:
    case DNGN_ENTER_TARTARUS:
    case DNGN_ENTER_ABYSS:
    case DNGN_EXIT_ABYSS:
    case DNGN_STONE_ARCH:
    case DNGN_ENTER_PANDEMONIUM:
    case DNGN_EXIT_PANDEMONIUM:
    case DNGN_TRANSIT_PANDEMONIUM:
    case DNGN_ENTER_ZOT:
    case DNGN_EXIT_ZOT:
        showed = '\\';
        break;

    case DNGN_ALTAR_ZIN:
    case DNGN_ALTAR_SHINING_ONE:
    case DNGN_ALTAR_KIKUBAAQUDGHA:
    case DNGN_ALTAR_YREDELEMNUL:
    case DNGN_ALTAR_XOM:
    case DNGN_ALTAR_VEHUMET:
    case DNGN_ALTAR_OKAWARU:
    case DNGN_ALTAR_MAKHLEB:
    case DNGN_ALTAR_SIF_MUNA:
    case DNGN_ALTAR_TROG:
    case DNGN_ALTAR_NEMELEX_XOBEH:
    case DNGN_ALTAR_ELYVILON:
        showed = '_';
        break;

    case DNGN_BLUE_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_I:
    case DNGN_SPARKLING_FOUNTAIN:
    case DNGN_DRY_FOUNTAIN_II:
    case DNGN_DRY_FOUNTAIN_III:
    case DNGN_DRY_FOUNTAIN_IV:
    case DNGN_DRY_FOUNTAIN_V:
    case DNGN_DRY_FOUNTAIN_VI:
    case DNGN_DRY_FOUNTAIN_VII:
    case DNGN_DRY_FOUNTAIN_VIII:
    case DNGN_PERMADRY_FOUNTAIN:
        showed = '}';
        break;

    default:
        showed = 0;
        break;
    }

    return showed;

}
