/*
 *  File:       beam.cc
 *  Summary:    Functions related to ranged attacks.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *   <7>    21mar2001    GDL    Replaced all FP arithmetic with integer*100 math
 *   <6>    07jan2001    GDL    complete rewrite.
 *   <5>    22July2000   GDL    allowed 'dummy' missiles from monsters
 *   <4>    11/14/99     cdl    evade beams with random40(ev) vice random2(ev)
 *                              all armour now protects against shrapnel
 *   <3>     6/ 2/99     DML    Added enums
 *   <2>     5/20/99     BWR    Added refreshs for curses
 *   <1>     -/--/--     LRH    Created
 */

#include "AppHdr.h"
#include "beam.h"

#include <stdlib.h>
#include <string.h>

#ifdef DOS
#include <dos.h>
#include <conio.h>
#endif
#if DEBUG_DIAGNOSTICS
#include <stdio.h>
#endif

#include "externs.h"

#include "cloud.h"
#include "effects.h"
#include "enum.h"
#include "fight.h"
#include "it_use2.h"
#include "itemname.h"
#include "items.h"
#include "misc.h"
#include "monplace.h"
#include "monstuff.h"
#include "mon-util.h"
#include "mstuff2.h"
#include "ouch.h"
#include "player.h"
#include "religion.h"
#include "skills.h"
#include "spells1.h"
#include "spells3.h"
#include "spells4.h"
#include "stuff.h"
#include "view.h"

#define BEAM_STOP       1000        // all beams stopped by subtracting this
                                    // from remaining range
#define MON_RESIST      0           // monster resisted
#define MON_UNAFFECTED  1           // monster unaffected
#define MON_AFFECTED    2           // monster was unaffected

static int spreadx[] = { 0, 0, 1, -1 };
static int spready[] = { -1, 1, 0, 0 };
static int opdir[]   = { 2, 1, 4, 3 };
static FixedArray < bool, 19, 19 > explode_map;

// helper functions (some of these, esp. affect(),  should probably
// be public):
static void sticky_flame_monster(int mn, bool source, int power);
static bool affectsWalls(struct bolt &beam);
static int affect(struct bolt &beam, int x, int y);
static bool isBouncy(struct bolt &beam);
static void beam_drop_object(struct bolt &beam, int inv_number, int x, int y);
static bool beam_term_on_target(struct bolt &beam);
static void beam_explodes(struct bolt &beam, int x, int y);
static int bounce(int &step1, int &step2, int w1, int w2, int &n1, int &n2,
    int l1, int l2, int &t1, int &t2, bool topBlocked, bool sideBlocked);
static bool fuzzyLine(int nx, int ny, int &tx, int &ty, int lx, int ly,
    int stepx, int stepy, bool roundX, bool roundY);
static int  affect_wall(struct bolt &beam, int x, int y);
static int  affect_place_clouds(struct bolt &beam, int x, int y);
static void affect_place_explosion_clouds(struct bolt &beam, int x, int y);
static int  affect_player(struct bolt &beam);
static void affect_items(struct bolt &beam, int x, int y);
static int  affect_monster(struct bolt &beam, struct monsters *mon);
static int  affect_monster_enchantment(struct bolt &beam, struct monsters *mon);
static int  range_used_on_hit(struct bolt &beam);
static void explosion1(struct bolt &pbolt);
static void explosion_map(struct bolt &beam, int x, int y,
    int count, int dir, int r);
static void explosion_cell(struct bolt &beam, int x, int y, bool drawOnly);

static void zappy(char z_type, int power, struct bolt &pbolt);

void zapping(char ztype, int power, struct bolt &pbolt)
{
    // GDL: note that rangeMax is set to 0, which means that max range is
    // equal to range.  This is OK,  since rangeMax really only matters for
    // stuff monsters throw/zap.

    // all of the following might be changed by zappy():
    pbolt.range = 8 + random2(5);       // default for "0" beams (I think)
    pbolt.rangeMax = 0;
    pbolt.damage = power;
    pbolt.hit = 0;                      // default for "0" beams (I think)
    pbolt.type = 0;                     // default for "0" beams
    pbolt.flavour = BEAM_MAGIC;         // default for "0" beams
    pbolt.ench_power = power;
    pbolt.obviousEffect = false;
    pbolt.isBeam = false;               // default for all beams.
    pbolt.isTracer = false;             // default for all player beams
    pbolt.thrower = KILL_YOU_MISSILE;   // missile from player

    // fill in the bolt structure
    zappy(ztype, power, pbolt);

    if (ztype == ZAP_LIGHTNING && !silenced(you.x_pos, you.y_pos))
        // needs to check silenced at other location, too {dlb}
    {
        mpr("You hear a mighty clap of thunder!");
        noisy(25, you.x_pos, you.y_pos);
    }

    beam(pbolt);

    return;
}                               // end zapping()

// *do not* call this function directly (duh - it's static), need to
// see zapping() for default values not set within this function {dlb}
static void zappy(char z_type, int power, struct bolt &pbolt)
{
    int temp_rand = 0;          // probability determination {dlb}

    // Applying some power caps so we can have some idea of what the
    // spells are capable of on the high end. -- bwr

    // In general, it's better not to change the caps, but to adjust
    // the spells below.  If you blindly adjust the caps you can
    // easily warp the balance so that first level spells become
    // dragon-slaying weapons of choice... this is because the
    // caps on the low level spells (which are likely to come
    // into effect during the game) are used to prevent them from
    // getting overly powerful (this allows low level spells to
    // develop at a fast enough rate to mean something to low
    // level characters, rather than having them develop slower
    // than high level spells just to keep their damage down so
    // that they don't become the obvious best choice for 400+ power
    // mages (who would be getting as much as 70pts for a single
    // sting... pretty good for one mana, little food, and
    // an easy spell to cast)).
    switch (z_type)
    {
    // level 1
    // This cap can affect mid-level characters (and gives them a good
    // reason to move onto spells that can support more power).
    case ZAP_MAGIC_DARTS:
    case ZAP_PEBBLE:
    case ZAP_STING:
    case ZAP_ELECTRICITY:
    case ZAP_FLAME_TONGUE:
    case ZAP_SMALL_SANDBLAST:
    case ZAP_DISRUPTION:                // ench_power boosted below
    case ZAP_PAIN:                      // ench_power boosted below
        if (power > 30)
            power = 30;
        break;

    // level 2
    // This cap should only affect high level characters (it's actually
    // pretty hard to get this high without intentionally trying to play
    // a strong spellcaster and they should really want to get into the
    // powerful spells below).
    case ZAP_FLAME:             // also ability (pow = lev * 2)
    case ZAP_FROST:             // also ability (pow = lev * 2)
    case ZAP_SANDBLAST:
        if (power > 100)
            power = 100;
        break;

    // level 3/4
    // This cap might actually come into effect (we're talking 20's in
    // the skills involved (inc spellcasting) with INT 20 and a spell
    // magnifier (staff or ring) to start breaking this point).
    case ZAP_STONE_ARROW:
    case ZAP_ORB_OF_ENERGY:
    case ZAP_STICKY_FLAME:
    case ZAP_ICE_BOLT:
        if (power > 150)
            power = 150;
        break;

    // powerful bolts and balls (levels 5-7)
    // This cap will probably seldom get used unless the character is
    // playing a full-mage style.
    case ZAP_FIRE:
    case ZAP_COLD:
    case ZAP_VENOM_BOLT:
    case ZAP_MAGMA:
    case ZAP_AGONY:
    case ZAP_LIGHTNING:                 // also invoc * 6 or lev * 2 (abils)
    case ZAP_NEGATIVE_ENERGY:           // also ability (pow = lev * 6)
    case ZAP_IRON_BOLT:
    case ZAP_DISINTEGRATION:
    case ZAP_FIREBALL:
    case ZAP_ORB_OF_ELECTRICITY:
    case ZAP_ORB_OF_FRAGMENTATION:
        if (power > 200)
            power = 200;
        break;

    // extremely powerful spells (levels 8-9)
    // This cap will probably never really come into effect except for
    // very few characters who try for it (you need high skills, some
    // items to magnify your magic, and a high INT to get this type
    // of number).
    case ZAP_CRYSTAL_SPEAR:
    case ZAP_HELLFIRE:
    case ZAP_ICE_STORM:
    case ZAP_CLEANSING_FLAME:
        if (power > 300)
            power = 300;
        break;

    // unlimited power (needs a good reason)
    case ZAP_BONE_SHARDS:    // power limited to 5250 before getting here
    case ZAP_BEAM_OF_ENERGY: // inaccuracy (only on staff, hardly hits)
        break;

    // natural/mutant breath/spit powers (power ~= characer level)
    case ZAP_SPIT_POISON:               // lev + mut * 5
    case ZAP_BREATHE_FIRE:              // lev + mut * 4 + 12 (if dragonform)
    case ZAP_BREATHE_FROST:             // lev
    case ZAP_BREATHE_ACID:              // lev (or invoc * 3 from minor destr)
    case ZAP_BREATHE_POISON:            // lev
    case ZAP_BREATHE_POWER:             // lev
    case ZAP_BREATHE_STEAM:             // lev
        if (power > 50)
            power = 50;
        break;

    // enchantments and other resistable effects
    case ZAP_SLOWING:
    case ZAP_HASTING:
    case ZAP_PARALYSIS:
    case ZAP_BACKLIGHT:
    case ZAP_SLEEP:
    case ZAP_CONFUSION:
    case ZAP_INVISIBILITY:
    case ZAP_ENSLAVEMENT:
    case ZAP_TELEPORTATION:
    case ZAP_DIGGING:
    case ZAP_POLYMORPH_OTHER:
    case ZAP_DISPEL_UNDEAD:
    case ZAP_DEGENERATION:
    case ZAP_BANISHMENT:
        // This is the only power that matters.  We magnify it apparently
        // to get values that work better with magic resistance checks...
        // those checks will scale down this value and max it out at 120.
        pbolt.ench_power *= 3;
        pbolt.ench_power /= 2;
        break;

    // anything else we cap to 100
    default:
        if (power > 100)
            power = 100;
        break;
    }

    // Note:  I'm only displaying the top damage and such here, that's
    // because it's really not been known before (since the above caps
    // didn't exist), so they were all pretty much unlimited before.
    // Also note, that the high end damage occurs at the cap, only
    // players that are that powerful can get that damage... and
    // although these numbers might seem small, you should remember
    // that Dragons in this game are 60-90 hp monsters, and very
    // few monsters have more than 100 hp (and that 1d5 damage is
    // still capable of taking a good sized chunk (and possibly killing)
    // any monster you're likely to meet in the first three levels). -- bwr

    // Note: damage > 100 signals that "random2(damage - 100)" will be
    // applied three times, which not only ups the damage but gives
    // a more normal distribution.
    switch (z_type)
    {
    case ZAP_FLAME:
        strcpy(pbolt.beam_name, "puff of flame");
        pbolt.colour = RED;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 104 + (power / 20);              // max dam: 27
        pbolt.hit = 8 + (power / 10);                   // max hit: 18
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_FIRE;

        pbolt.obviousEffect = true;
        break;

    case ZAP_FROST:
        strcpy(pbolt.beam_name, "puff of frost");
        pbolt.colour = WHITE;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 104 + (power / 20);              // max dam: 27
        pbolt.hit = 8 + (power / 10);                   // max hit: 18
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_COLD;

        pbolt.obviousEffect = true;
        break;

    case ZAP_SLOWING:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_SLOW;
        pbolt.isBeam = true;
        break;

    case ZAP_HASTING:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_HASTE;
        pbolt.isBeam = true;
        break;

    case ZAP_MAGIC_DARTS:
        strcpy(pbolt.beam_name, "magic dart");
        pbolt.colour = LIGHTMAGENTA;
        pbolt.range = random2(5) + 8;
        pbolt.damage = 2 + (power / 4);                 // max dam: 9
        pbolt.hit = 1500;                               // hits always
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable
        pbolt.obviousEffect = true;
        break;

    case ZAP_HEALING:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_HEALING;
        pbolt.hit = 7 + (power / 3);
        pbolt.isBeam = true;
        break;

    case ZAP_PARALYSIS:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_PARALYSIS;
        pbolt.isBeam = true;
        break;

    case ZAP_FIRE:
        strcpy(pbolt.beam_name, "bolt of fire");
        pbolt.colour = RED;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 107 + (power / 11);              // max dam: 75
        pbolt.hit = 10 + (random2(power) / 25);         // max hit: 17
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_FIRE;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_COLD:
        strcpy(pbolt.beam_name, "bolt of cold");
        pbolt.colour = WHITE;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 107 + (power / 11);              // max dam: 75
        pbolt.hit = 10 + (random2(power) / 25);         // max hit: 17
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_COLD;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_MAGMA:
        strcpy(pbolt.beam_name, "bolt of magma");
        pbolt.colour = RED;
        pbolt.range = 5 + random2(4);
        pbolt.damage = 106 + (power / 12);              // max dam: 66
        pbolt.hit = 8 + (random2(power) / 25);          // max hit: 15
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_LAVA;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_CONFUSION:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_CONFUSION;
        pbolt.isBeam = true;
        break;

    case ZAP_INVISIBILITY:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_INVISIBILITY;
        pbolt.isBeam = true;
        break;

    case ZAP_DIGGING:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_DIGGING;
        // not ordinary "0" beam range {dlb}
        pbolt.range = 3 + random2(power) + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_FIREBALL:
        strcpy(pbolt.beam_name, "fireball");
        pbolt.colour = RED;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 105 + (power / 7);               // max dam: 99
        pbolt.hit = 40;                                 // hit: 40
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_EXPLOSION;                 // fire

        break;

    case ZAP_TELEPORTATION:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_TELEPORT;
        pbolt.range = 9 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_LIGHTNING: // also for breath (at pow = lev * 2; max dam: 30)
        strcpy(pbolt.beam_name, "bolt of lightning");
        pbolt.colour = LIGHTCYAN;
        pbolt.range = 8 + random2(10);                  // extended in beam
        pbolt.damage = 106 + (power / 12);              // max dam: 66
        pbolt.hit = 7 + (random2(power) / 20);          // max hit: 16
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_ELECTRICITY;               // beams & reflects

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_POLYMORPH_OTHER:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_POLYMORPH;
        pbolt.range = 9 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_VENOM_BOLT:
        strcpy(pbolt.beam_name, "bolt of poison");
        pbolt.colour = LIGHTGREEN;
        pbolt.range = 8 + random2(10);
        pbolt.damage = 105 + (power / 14);              // max dam: 57
        pbolt.hit = 8 + (random2(power) / 20);          // max hit: 17
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_POISON;                    // extra damage

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_NEGATIVE_ENERGY:
        strcpy(pbolt.beam_name, "bolt of negative energy");
        pbolt.colour = DARKGREY;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 105 + (power / 14);              // max dam: 57
        pbolt.hit = 8 + (random2(power) / 20);          // max hit: 17
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_NEG;                       // drains levels

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_BEAM_OF_ENERGY:    // bolt of innacuracy
        strcpy(pbolt.beam_name, "narrow beam of energy");
        pbolt.colour = YELLOW;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 110 + (power / 5);               // max dam: unlimited
        pbolt.hit = 2;                                  // hit: 2 (very hard)
        pbolt.type = SYM_ZAP;
        pbolt.flavour = 17;    // whatever

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_ORB_OF_ENERGY:     // Mystic Bolt
        strcpy(pbolt.beam_name, "orb of energy");
        pbolt.colour = LIGHTMAGENTA;
        pbolt.range = 7 + random2(5);
        pbolt.damage = 105 + (power / 20);              // max dam: 36
        pbolt.hit = 10 + (power / 7);                   // max hit: 31
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable

        pbolt.obviousEffect = true;
        break;

    case ZAP_ENSLAVEMENT:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_CHARM;
        pbolt.range = 7 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_PAIN:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_PAIN;
        pbolt.range = 7 + random2(8);

        // is this right? other "0" beams do not define ->hit {dlb}
        // actually, it's used for damage (the spell always hits)
        pbolt.hit = 6 + (power / 3);                   // max dam: 16

        pbolt.ench_power *= 7;
        pbolt.ench_power /= 2;
        pbolt.isBeam = true;
        break;

    case ZAP_STICKY_FLAME:
        strcpy(pbolt.beam_name, "sticky flame");        // extra damage
        pbolt.colour = RED;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 102 + (power / 30);              // max dam: 21
        pbolt.hit = 11 + (power / 10);                  // max hit: 26
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_FIRE;

        pbolt.obviousEffect = true;
        break;

    case ZAP_DISPEL_UNDEAD:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_DISPEL_UNDEAD;
        pbolt.range = 7 + random2(8);
        // is this right? other "0" beams do not define ->hit {dlb}
        pbolt.hit = 9 + (power / 5);
        pbolt.isBeam = true;
        break;

    case ZAP_CLEANSING_FLAME:
        strcpy(pbolt.beam_name, "golden flame");
        pbolt.colour = YELLOW;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 110 + (power / 7);               // max dam: 156
        pbolt.hit = 20 + (random2(power) / 80);         // max hit: 23
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_HOLY;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_BONE_SHARDS:
        strcpy(pbolt.beam_name, "spray of bone shards");
        pbolt.colour = LIGHTGREY;
        pbolt.range = 7 + random2(10);

        // note that f_p[2] has a high value for this spell
        // The power here is quite large because it contains a
        // factor based on the size of the skeleton used... therefore
        // damage is highly dependant on material.
        pbolt.damage = 102 + (power / 250);             // max dam: 69
        pbolt.hit = 8 + (power / 100);                  // max hit: 60 (high)
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_MAGIC;                     // unresisted

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_BANISHMENT:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_BANISH;
        pbolt.range = 7 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_DEGENERATION:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_DEGENERATE;
        pbolt.range = 7 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_STING:
        strcpy(pbolt.beam_name, "sting");
        pbolt.colour = GREEN;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 6 + (power / 5);                 // max dam: 12
        pbolt.hit = 8 + (power / 5);                    // max hit: 14
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_POISON;                    // extra damage

        pbolt.obviousEffect = true;
        break;

    case ZAP_HELLFIRE:
        strcpy(pbolt.beam_name, "hellfire");
        pbolt.colour = RED;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 110 + (power / 10);              // max dam: 123
        pbolt.hit = 20 + (power / 10);                  // max hit: 50
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_EXPLOSION;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_PEBBLE:
        strcpy(pbolt.beam_name, "pebble");
        pbolt.colour = BROWN;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 5;                               // dam: 5
        pbolt.hit = 8 + (power / 10);                   // max hit: 11
        pbolt.type = SYM_MISSILE;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable

        pbolt.obviousEffect = true;
        break;

    case ZAP_STONE_ARROW:
        strcpy(pbolt.beam_name, "stone arrow");
        pbolt.colour = LIGHTGREY;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 105 + (power / 20);              // max dam: 36
        pbolt.hit = 7 + (power / 20);                   // max hit: 14
        pbolt.type = SYM_MISSILE;                       // unresistable
        pbolt.flavour = BEAM_MMISSILE;

        pbolt.obviousEffect = true;
        break;

    case ZAP_IRON_BOLT:
        strcpy(pbolt.beam_name, "iron bolt");
        pbolt.colour = LIGHTCYAN;
        pbolt.range = 4 + random2(5);
        pbolt.damage = 108 + (power / 12);              // max dam: 72
        pbolt.hit = 5 + (random2(power) / 25);          // max hit: 13
        pbolt.type = SYM_MISSILE;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable

        pbolt.obviousEffect = true;
        break;

    case ZAP_CRYSTAL_SPEAR:
        strcpy(pbolt.beam_name, "crystal spear");
        pbolt.colour = WHITE;
        pbolt.range = 7 + random2(10);
        pbolt.damage = 112 + (power / 9);               // max dam: 135
        pbolt.hit = 4 + random2(power) / 30;            // max hit: 14
        pbolt.type = SYM_MISSILE;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable

        pbolt.obviousEffect = true;
        break;

    case ZAP_ELECTRICITY:
        strcpy(pbolt.beam_name, "zap");
        pbolt.colour = LIGHTCYAN;
        pbolt.range = 6 + random2(8);                   // extended in beam
        pbolt.damage = 5 + random2(power) / 4;          // max dam: 12
        pbolt.hit = 8 + power / 7;                      // max hit: 12
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_ELECTRICITY;               // beams & reflects

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_ORB_OF_ELECTRICITY:
        strcpy(pbolt.beam_name, "orb of electricity");
        pbolt.colour = LIGHTBLUE;
        pbolt.range = 9 + random2(12);
        pbolt.damage = 105 + (power / 6);               // max dam: 114
        pbolt.hit = 40;                                 // hit: 40
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_ELECTRICITY;
        break;

    case ZAP_SPIT_POISON:
        // max pow = lev + mut * 5 = 42
        strcpy(pbolt.beam_name, "splash of poison");
        pbolt.colour = GREEN;
        pbolt.range = 3 + random2(1 + (power / 2));
        pbolt.damage = 4 + (power / 2);                 // max dam: 25
        pbolt.hit = 5 + random2(1 + (power / 3));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_POISON;
        pbolt.obviousEffect = true;
        break;

    case ZAP_DEBUGGING_RAY:
        strcpy(pbolt.beam_name, "debugging ray");
        pbolt.colour = random_colour();
        pbolt.range = 7 + random2(10);
        pbolt.damage = 150;                             // dam: 150
        pbolt.hit = 60;                                 // hit: 60
        pbolt.type = SYM_DEBUG;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable

        pbolt.obviousEffect = true;
        break;

    case ZAP_BREATHE_FIRE:
        // max pow = lev + mut * 4 + 12 = 51 (capped to 50)
        strcpy(pbolt.beam_name, "fiery breath");
        pbolt.colour = RED;
        pbolt.range = 3 + random2(1 + (power / 2));
        pbolt.damage = 104 + (power / 3);               // max dam: 60
        pbolt.hit = 8 + random2(1 + (power / 3));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_FIRE;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_BREATHE_FROST:
        // max power = lev = 27
        strcpy(pbolt.beam_name, "freezing breath");
        pbolt.colour = WHITE;
        pbolt.range = 3 + random2(1 + (power / 2));
        pbolt.damage = 104 + (power / 3);               // max dam: 39
        pbolt.hit = 8 + random2(1 + (power / 3));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_COLD;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_BREATHE_ACID:
        // max power = lev for ability, 50 for minor destruction (max dam: 57)
        strcpy(pbolt.beam_name, "acid");
        pbolt.colour = YELLOW;
        pbolt.range = 3 + random2(1 + (power / 2));
        pbolt.damage = 103 + (power / 3);               // max dam: 36
        pbolt.hit = 5 + random2(1 + (power / 3));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_ACID;

        pbolt.obviousEffect = true;
        break;

    case ZAP_BREATHE_POISON:    // leaves clouds of gas
        // max power = lev = 27
        strcpy(pbolt.beam_name, "poison gas");
        pbolt.colour = GREEN;
        pbolt.range = 3 + random2(1 + (power / 2));
        pbolt.damage = 102 + (power / 6);               // max dam: 18
        pbolt.hit = 5 + random2(1 + (power / 3));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_POISON;

        pbolt.obviousEffect = true;
        break;

    case ZAP_BREATHE_POWER:
        strcpy(pbolt.beam_name, "bolt of energy");
        // max power = lev = 27

        pbolt.colour = BLUE;
        if (random2(power) >= 8)
            pbolt.colour = LIGHTBLUE;
        if (random2(power) >= 12)
            pbolt.colour = MAGENTA;
        if (random2(power) >= 17)
            pbolt.colour = LIGHTMAGENTA;

        pbolt.range = 6 + random2(1 + (power / 2));
        pbolt.damage = 103 + (power / 3);               // max dam: 36
        pbolt.hit = 11 + random2(1 + (power / 3));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_MMISSILE;                  // unresistable

        pbolt.obviousEffect = true;
        break;

    case ZAP_ENSLAVE_UNDEAD:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_ENSLAVE_UNDEAD;
        pbolt.range = 7 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_AGONY:
        strcpy(pbolt.beam_name, "0agony");
        pbolt.colour = BCOL_PAIN;
        pbolt.range = 7 + random2(8);
        // is this right? other "0" beams do not define ->hit {dlb}
        pbolt.hit = 6 + (power / 50);
        pbolt.ench_power *= 5;
        pbolt.isBeam = true;
        break;

    case ZAP_DISRUPTION:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_DISINTEGRATION;
        pbolt.range = 7 + random2(8);
        // is this right? other "0" beams do not define ->hit {dlb}
        // actually, it's used for damage (the spell always hits)
        pbolt.hit = 6 + (power / 3);                   // max dam: 16
        pbolt.ench_power *= 3;
        pbolt.isBeam = true;
        break;

    case ZAP_DISINTEGRATION:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_DISINTEGRATION;
        pbolt.range = 7 + random2(8);
        // is this right? other "0" beams do not define ->hit {dlb}
        // actually, it's used for damage (the spell always hits)
        pbolt.hit = 15 + (power / 3);                   // max dam: 80
        pbolt.ench_power *= 5;
        pbolt.ench_power /= 2;
        pbolt.isBeam = true;
        break;

    case ZAP_BREATHE_STEAM:
        // max power = lev = 27
        strcpy(pbolt.beam_name, "ball of steam");
        pbolt.colour = LIGHTGREY;
        pbolt.range = 6 + random2(5);
        pbolt.damage = 104 + (power / 5);               // max dam: 27
        pbolt.hit = 10 + random2(1 + (power / 5));
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_FIRE;
        pbolt.obviousEffect = true;
        break;

    case ZAP_CONTROL_DEMON:
        strcpy(pbolt.beam_name, "0");
        pbolt.colour = BCOL_ENSLAVE_DEMON;
        pbolt.range = 7 + random2(5);
        pbolt.ench_power *= 17;
        pbolt.ench_power /= 10;
        pbolt.isBeam = true;
        break;

    case ZAP_ORB_OF_FRAGMENTATION:
        strcpy(pbolt.beam_name, "metal orb");
        pbolt.colour = CYAN;
        pbolt.range = 9 + random2(7);
        pbolt.damage = 110 + (power / 7);               // max dam: 114
        pbolt.hit = 20;                                 // hit: 20
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_MMISSILE;
        break;

    case ZAP_ICE_BOLT:
        strcpy(pbolt.beam_name, "bolt of ice");
        pbolt.colour = WHITE;
        pbolt.range = 8 + random2(5);
        pbolt.damage = 105 + (power / 10);              // max dam: 60
        pbolt.hit = 9 + (power / 12);                   // max hit: 21
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_ICE;
        break;

    case ZAP_ICE_STORM:
        strcpy(pbolt.beam_name, "great blast of cold");
        pbolt.colour = BLUE;
        pbolt.range = 9 + random2(5);
        pbolt.damage = 105 + (power / 7);               // max dam: 141
        pbolt.hit = 20 + (power / 10);                  // max hit: 50
        pbolt.ench_power = power;                       // used for radius
        pbolt.type = SYM_ZAP;
        /* ice */// <- changed from BEAM_COLD b/c of comment 13jan2000 {dlb}
        pbolt.flavour = BEAM_ICE;
        break;

    case ZAP_SLEEP:             //jmf: added
        strcpy(pbolt.beam_name, "0");
        pbolt.flavour = BEAM_SLEEP;
        pbolt.range = 7 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_BACKLIGHT: //jmf: added
        strcpy(pbolt.beam_name, "0");
        pbolt.flavour = BEAM_BACKLIGHT;
        pbolt.colour = BLUE;
        pbolt.range = 7 + random2(5);
        pbolt.isBeam = true;
        break;

    case ZAP_FLAME_TONGUE:      //jmf: ought to be a weak, short-range missile
        strcpy(pbolt.beam_name, "flame");
        pbolt.colour = RED;

        pbolt.range = 1 + random2(2) + random2(power) / 10;
        if (pbolt.range > 4)
            pbolt.range = 4;

        pbolt.damage = 8 + (power / 3);               // max dam: 18
        pbolt.hit = 8 + (power / 5);                  // max hit: 15
        pbolt.type = SYM_BOLT;
        pbolt.flavour = BEAM_FIRE;

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_SANDBLAST: //jmf: ought to be a weak, short-range missile
        strcpy(pbolt.beam_name, coinflip() ? "blast of rock" : "rocky blast");
        pbolt.colour = BROWN;

        pbolt.range = 2 + random2(power) / 20;
        if (pbolt.range > 4)
            pbolt.range = 4;

        // remember -- this one has max power of 100
        pbolt.damage = 103 + (power / 25);              // max dam: 21
        pbolt.hit = 13 + (power / 10);                  // max hit: 20
        pbolt.type = SYM_BOLT;
        pbolt.flavour = BEAM_FRAG;                      // extra AC resist

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;

    case ZAP_SMALL_SANDBLAST:   //jmf: ought to be a weak, short-range missile
        strcpy(pbolt.beam_name, "blast of ");

        temp_rand = random2(4);

        strcat(pbolt.beam_name, (temp_rand == 0) ? "dust" :
                                (temp_rand == 1) ? "dirt" :
                                (temp_rand == 2) ? "grit" : "sand");

        pbolt.colour = BROWN;
        pbolt.range = (random2(power) > random2(30)) ? 2 : 1; // 1 or 2 squares
        pbolt.damage = 6 + power / 5;                   // max dam: 12
        pbolt.hit = 10 + (power / 5);                   // max hit: 16
        pbolt.type = SYM_BOLT;
        pbolt.flavour = BEAM_FRAG;                      // extra AC resist

        pbolt.obviousEffect = true;
        pbolt.isBeam = true;
        break;
    }                           // end of switch
}                               // end zappy()

/*  NEW (GDL):
 *  Now handles all beamed/thrown items and spells,  tracers, and their effects.
 *  inv_number is used to determine what, if anything, was actually 'thrown',
 *  either from monster or player inventory.
 *
 *  if inv_number is -1,  there is no physical object being thrown that could
 *  land on the ground.
 */


/*
 * Beam pseudo code:
 *
 * 1. Calculate stepx and stepy - algorithms depend on finding a step axis
 *    which results in a line of rise 1 or less (ie 45 degrees or less)
 * 2. Calculate range.  Tracers always have max range, otherwise the beam
 *    will have somewhere between range and rangeMax
 * 3. Loop tracing out the line:
 *      3a. Check for walls and wall affecting beams
 *      3b. If no valid move is found, try a fuzzy move
 *      3c. If no valid move is yet found, try bouncing
 *      3d. If no valid move or bounce is found, break
 *      4. Check for beam termination on target
 *      5. Affect the cell which the beam just moved into -> affect()
 *      6. Decrease remaining range appropriately
 *      7. Check for early out due to aimedAtFeet
 *      8. Draw the beam
 * 9. Drop an object where the beam 'landed'
 *10. Beams explode where the beam 'landed'
 *11. If no message generated yet, send "nothing happens" (enchantments only)
 *
 */


void beam(struct bolt &pbolt, int inv_number)
{
    int dx, dy;             // total delta between source & target
    int lx, ly;             // last affected x,y
    int stepx, stepy;       // x,y increment - FP
    int wx, wy;             // 'working' x,y - FP
    bool beamTerminate;     // has beam been 'stopped' by something?
    int nx, ny;             // test(new) x,y - FP
    int tx, ty;             // test(new) x,y - integer
    bool roundX, roundY;    // which to round?
    int rangeRemaining;
    bool fuzzyOK;           // fuzzification resulted in OK move
    bool sideBlocked, topBlocked, random_beam;

#if DEBUG_DIAGNOSTICS
    snprintf( info, INFO_SIZE, "%s %s from %d, %d to %d, %d : t=%d c=%d f=%d hit=%d dam=%d",
             (pbolt.isBeam) ? "beam" : "missile",
             (pbolt.isTracer) ? "tracer" : "",
             pbolt.source_x, pbolt.source_y,
             pbolt.target_x, pbolt.target_y,
             pbolt.type, pbolt.colour, pbolt.flavour,
             pbolt.hit, pbolt.damage );

    mpr(info);
#endif

    // init
    pbolt.aimedAtFeet = false;
    pbolt.msgGenerated = false;
    pbolt.isExplosion = false;
    roundY = false;
    roundX = false;

    // first, calculate beam step
    dx = pbolt.target_x - pbolt.source_x;
    dy = pbolt.target_y - pbolt.source_y;

    // check for aim at feet
    if (dx == 0 && dy == 0)
    {
        pbolt.aimedAtFeet = true;
        stepx = 0;
        stepy = 0;
        tx = pbolt.source_x;
        ty = pbolt.source_y;
    }
    else
    {
        if (abs(dx) >= abs(dy))
        {
            stepx = (dx>0)?100:-100;
            stepy = 100 * dy/(abs(dx));
            roundY = true;
        }
        else
        {
            stepy = (dy>0)?100:-100;
            stepx = 100 * dx/(abs(dy));
            roundX = true;
        }
    }

    // give chance for beam to affect one cell even if aimedAtFeet.
    beamTerminate = false;
    // setup working coords
    lx = pbolt.source_x;
    wx = 100 * lx;
    ly = pbolt.source_y;
    wy = 100 * ly;
    // setup range
    rangeRemaining = pbolt.range;
    if (pbolt.rangeMax > pbolt.range)
    {
        if (pbolt.isTracer)
            rangeRemaining = pbolt.rangeMax;
        else
            rangeRemaining += random2((pbolt.rangeMax - pbolt.range) + 1);
    }

    // before we start drawing the beam, turn buffering off
#ifdef WIN32CONSOLE
    bool oldValue = true;
    if (!pbolt.isTracer)
        oldValue = setBuffering(false);
#endif

    // cannot use source_x, source_y, target_x, target_y during
    // step algorithm due to bouncing.

    // now, one step at a time, try to move towards target.
    while(!beamTerminate)
    {
        nx = wx + stepx;
        ny = wy + stepy;

        if (roundY)
        {
            tx = nx / 100;
            ty = (ny + 50) / 100;
        }
        if (roundX)
        {
            ty = ny / 100;
            tx = (nx + 50) / 100;
        }

        // check that tx, ty are valid.  If not,  set to last
        // x,y and break.
        if (tx < 0 || tx >= GXM || ty < 0 || ty >= GYM)
        {
            tx = lx;
            ty = ly;
            break;
        }

        // see if tx, ty is blocked by something
        if (grd[tx][ty] < MINMOVE)
        {
            // first, check to see if this beam affects walls.
            if (affectsWalls(pbolt))
            {
                // should we ever get a tracer with a wall-affecting
                // beam (possible I suppose),  we'll quit tracing now.
                if (!pbolt.isTracer)
                    rangeRemaining -= affect(pbolt, tx, ty);

                // if it's still a wall, quit.
                if (grd[tx][ty] < MINMOVE)
                {
                    break;      // breaks from line tracing
                }
            }
            else
            {
                // BEGIN fuzzy line algorithm
                fuzzyOK = fuzzyLine(nx,ny,tx,ty,lx,ly,stepx,stepy,roundX,roundY);
                if (!fuzzyOK)
                {
                    // BEGIN bounce case
                    if (!isBouncy(pbolt))
                    {
                        tx = lx;
                        ty = ly;
                        break;          // breaks from line tracing
                    }

                    sideBlocked = false;
                    topBlocked = false;
                    // BOUNCE -- guaranteed to return reasonable tx, ty.
                    // if it doesn't, we'll quit in the next if stmt anyway.
                    if (roundY)
                    {
                        if ( grd[lx + stepx / 100][ly] < MINMOVE)
                            sideBlocked = true;

                        if (dy != 0)
                        {
                            if ( grd[lx][ly + (stepy>0?1:-1)] < MINMOVE)
                                topBlocked = true;
                        }

                        rangeRemaining -= bounce(stepx, stepy, wx, wy, nx, ny,
                            lx, ly, tx, ty, topBlocked, sideBlocked);
                    }
                    else
                    {
                        if ( grd[lx][ly + stepy / 100] < MINMOVE)
                            sideBlocked = true;

                        if (dx != 0)
                        {
                            if ( grd[lx + (stepx>0?1:-1)][ly] < MINMOVE)
                                topBlocked = true;
                        }

                        rangeRemaining -= bounce(stepy, stepx, wy, wx, ny, nx,
                            ly, lx, ty, tx, topBlocked, sideBlocked);
                    }
                    // END bounce case - range check
                    if (rangeRemaining < 1)
                    {
                        tx = lx;
                        ty = ly;
                        break;
                    }
                }
            } // end else - beam doesn't affect walls
        } // endif - is tx, ty wall?

        // at this point, if grd[tx][ty] is still a wall, we
        // couldn't find any path: bouncy, fuzzy, or not - so break.
        if (grd[tx][ty] < MINMOVE)
        {
            tx = lx;
            ty = ly;
            break;
        }

        // check for "target termination"
        // occurs when beam can be targetted at empty
        // cell (e.g. a mage wants an explosion to happen
        // between two monsters)

        // in this case,  don't affect the cell - players
        // /monsters have no chance to dodge or block such
        // a beam,  and we want to avoid silly messages.
        if (tx == pbolt.target_x && ty == pbolt.target_y)
            beamTerminate = beam_term_on_target(pbolt);

        // affect the cell,  except in the special case noted
        // above -- affect() will early out if something gets
        // hit and the beam is type 'term on target'.
        if (!beamTerminate)
        {
            // random beams: randomize before affect
            random_beam = false;
            if (pbolt.flavour == BEAM_RANDOM)
            {
                random_beam = true;
                pbolt.flavour = BEAM_FIRE + random2(7);
            }

            rangeRemaining -= affect(pbolt, tx, ty);

            if (random_beam)
                pbolt.flavour = BEAM_RANDOM;
        }

        // always decrease range by 1
        rangeRemaining -= 1;

        // check for range termination
        if (rangeRemaining <= 0)
            beamTerminate = true;

        // special case - beam was aimed at feet
        if (pbolt.aimedAtFeet)
            beamTerminate = true;

        // actually draw the beam/missile/whatever,
        // if the player can see the cell.
        if (!pbolt.isTracer && pbolt.beam_name[0] != '0' && see_grid(tx,ty))
        {
            // we don't clean up the old position.
            // first, most people like seeing the full path,
            // and second, it is hard to do it right with
            // respect to killed monsters, cloud trails, etc.

            // draw new position
            int drawx = tx - you.x_pos + 18;
            int drawy = ty - you.y_pos + 9;
            // bounds check
            if (drawx > 8 && drawx < 26 && drawy > 0 && drawy < 18)
            {
                if (pbolt.colour == BLACK)
                    textcolor(random_colour());
                else
                    textcolor(pbolt.colour);

                gotoxy(drawx, drawy);
                putch(pbolt.type);

#ifdef LINUX
                // get curses to update the screen so we can see the beam
                update_screen();
#endif

                delay(15);

#ifdef MISSILE_TRAILS_OFF
                if (!pbolt.isBeam || pbolt.beam_name[0] == '0')
                    viewwindow(1,false); // mv: added. It's not optimal but
                                         // is usually enough
#endif
            }

        }

        // set some stuff up for the next iteration
        lx = tx;
        ly = ty;

        wx = nx;
        wy = ny;

    } // end- while !beamTerminate

    // the beam has finished,  and terminated at tx, ty

    // leave an object, if applicable
    beam_drop_object(pbolt, inv_number, tx, ty);

    // check for explosion.  NOTE that for tracers, we have to make a copy
    // of target co'ords and then reset after calling this -- tracers should
    // never change any non-tracers fields in the beam structure. -- GDL
    int ox = pbolt.target_x;
    int oy = pbolt.target_y;

    beam_explodes(pbolt, tx, ty);

    if (pbolt.isTracer)
    {
        pbolt.target_x = ox;
        pbolt.target_y = oy;
    }

    // canned msg for enchantments that affected no-one
    if (pbolt.beam_name[0] == '0' && pbolt.colour != BCOL_DIGGING)
    {
        if (!pbolt.isTracer && !pbolt.msgGenerated && !pbolt.obviousEffect)
            canned_msg(MSG_NOTHING_HAPPENS);
    }

    // that's it!
#ifdef WIN32CONSOLE
    if (!pbolt.isTracer)
        setBuffering(oldValue);
#endif
}                               // end beam();


// returns damage taken by a monster from a "flavoured" (fire, ice, etc.)
// attack -- damage from clouds and branded weapons handled elsewhere.
int mons_adjust_flavoured(struct monsters *monster, struct bolt &pbolt,
                       int hurted, bool doFlavouredEffects)
{
    // if we're not doing flavored effects,  must be preliminary
    // damage check only;  do not print messages or apply any side
    // effects!

    switch (pbolt.flavour)
    {
    case BEAM_FIRE:
        if (mons_res_fire(monster->type) > 0)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }
        else if (monster->inv[MSLOT_ARMOUR] != NON_ITEM
                 && mitm[monster->inv[MSLOT_ARMOUR]].special
                                                 == SPARM_FIRE_RESISTANCE)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " resists.");
            hurted /= 3;
        }

        if (mons_res_fire(monster->type) == -1
            && (monster->inv[MSLOT_ARMOUR] == NON_ITEM
                || mitm[monster->inv[MSLOT_ARMOUR]].special
                                                != SPARM_FIRE_RESISTANCE))
        {
            if (monster->type == MONS_ICE_BEAST
                || monster->type == MONS_SIMULACRUM_SMALL
                || monster->type == MONS_SIMULACRUM_LARGE)
            {
                if (doFlavouredEffects)
                    simple_monster_message(monster, " melts!");
            }
            else
            {
                if (doFlavouredEffects)
                    simple_monster_message(monster, " is burned terribly!");
            }
            hurted *= 15;
            hurted /= 10;
        }
        break;


    case BEAM_COLD:
        if (mons_res_cold(monster->type) > 0)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }
        else if (monster->inv[MSLOT_ARMOUR] != NON_ITEM
                 && mitm[monster->inv[MSLOT_ARMOUR]].special
                                                 == SPARM_COLD_RESISTANCE)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " resists.");
            hurted /= 3;
        }

        if (mons_res_cold(monster->type) == -1
            && (monster->inv[MSLOT_ARMOUR] == NON_ITEM
                || mitm[monster->inv[MSLOT_ARMOUR]].special
                                                != SPARM_COLD_RESISTANCE))
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " is frozen!");
            hurted *= 15;
            hurted /= 10;
        }
        break;

    case BEAM_ELECTRICITY:
        if (mons_res_elec(monster->type) > 0)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }
        break;


    case BEAM_POISON:
        if (doFlavouredEffects && !one_chance_in(3))
            poison_monster(monster, YOU_KILL(pbolt.thrower));

        if (mons_res_poison(monster->type) > 0)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }
        break;

    case BEAM_NEG:
        if (mons_holiness(monster->type) == MH_UNDEAD
            || mons_holiness(monster->type) == MH_DEMONIC)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }
        else
        {
            // early out for tracer/no side effects
            if (!doFlavouredEffects)
                return hurted;

            simple_monster_message(monster, " is drained.");

            if (one_chance_in(5))
                monster->hit_dice--;

            monster->max_hit_points -= 2 + random2(3);
            monster->hit_points -= 2 + random2(3);

            if (monster->hit_points >= monster->max_hit_points)
                monster->hit_points = monster->max_hit_points;

            if (monster->hit_dice < 1)
                monster->hit_points = 0;
        }                       // end else
        break;

    case BEAM_HOLY:             // flame of cleansing
        if (mons_holiness(monster->type) == MH_NATURAL
            || mons_holiness(monster->type) == MH_HOLY)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }
        break;

    case BEAM_ICE:
        /* ice - about 50% of damage is cold, other 50% is impact and
           can't be resisted (except by AC, of course) */
        if (mons_res_cold(monster->type) > 0)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " partially resists.");
            hurted /= 2;
        }
        else if (monster->inv[MSLOT_ARMOUR] != NON_ITEM
                 && mitm[monster->inv[MSLOT_ARMOUR]].special ==
                 SPARM_COLD_RESISTANCE)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " partially resists.");
            hurted /= 2;
        }

        if (mons_res_cold(monster->type) == -1
            && (monster->inv[MSLOT_ARMOUR] == NON_ITEM
                || mitm[monster->inv[MSLOT_ARMOUR]].special
                                                != SPARM_COLD_RESISTANCE))
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " is frozen!");
            hurted *= 13;
            hurted /= 10;
        }
        break;
    }                           /* end of switch */

    if (pbolt.flavour == BEAM_LAVA)    //jmf: lava != hellfire
    {
        if (mons_res_fire(monster->type) > 0
            || (monster->inv[MSLOT_ARMOUR] != NON_ITEM
                && mitm[monster->inv[MSLOT_ARMOUR]].special
                                                == SPARM_FIRE_RESISTANCE))
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " partially resists.");
            hurted /= 2;
        }

        if (mons_res_fire(monster->type) == -1
            && (monster->inv[MSLOT_ARMOUR] == NON_ITEM
                || mitm[monster->inv[MSLOT_ARMOUR]].special
                                                != SPARM_FIRE_RESISTANCE))
        {
            if (monster->type == MONS_ICE_BEAST
                || monster->type == MONS_SIMULACRUM_SMALL
                || monster->type == MONS_SIMULACRUM_LARGE)
            {
                if (doFlavouredEffects)
                    simple_monster_message(monster, " melts!");
            }
            else
            {
                if (doFlavouredEffects)
                    simple_monster_message(monster, " is burned terribly!");
            }

            hurted *= 12;
            hurted /= 10;
        }
    }
    else if (stricmp(pbolt.beam_name, "hellfire") == 0)
    {
        if (mons_res_fire(monster->type) >= 2)
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " appears unharmed.");
            hurted = 0;
        }

        if (mons_res_fire(monster->type) == 1
            || (monster->inv[MSLOT_ARMOUR] != NON_ITEM
                && mitm[monster->inv[MSLOT_ARMOUR]].special
                                                == SPARM_FIRE_RESISTANCE))
        {
            if (doFlavouredEffects)
                simple_monster_message(monster, " partially resists.");
            hurted /= 2;
        }

        if (mons_res_fire(monster->type) == -1
            && (monster->inv[MSLOT_ARMOUR] == NON_ITEM
                || mitm[monster->inv[MSLOT_ARMOUR]].special
                                                != SPARM_FIRE_RESISTANCE))
        {
            if (monster->type == MONS_ICE_BEAST
                || monster->type == MONS_SIMULACRUM_SMALL
                || monster->type == MONS_SIMULACRUM_LARGE)
            {
                if (doFlavouredEffects)
                    simple_monster_message(monster, " melts!");
            }
            else
            {
                if (doFlavouredEffects)
                    simple_monster_message(monster, " is burned terribly!");
            }

            hurted *= 12;       /* hellfire */
            hurted /= 10;
        }
    }

    return hurted;
}                               // end mons_adjust_flavoured()

// Returns true if the monster made its save against hostile
// enchantments/some other magics.
bool check_mons_magres(struct monsters * monster, int pow)
{
    int mrs = mon_resist_mag(monster->type, monster->hit_dice);

    if (mrs == 5000)
        return true;

    // Evil, evil hack to make weak one hd monsters easier for first
    // level characters who have resistable 1st level spells. Six is
    // a very special value because mrs = hd * 2 * 3 for most monsters,
    // and the weak, low level monsters have been adjusted so that the
    // "3" is typically a 1.  There are some notable one hd monsters
    // that shouldn't fall under this, so we do < 6, instead of <= 6...
    // or checking monster->hit_dice.  The goal here is to make the
    // first level easier for these classes and give them a better
    // shot at getting to level two or three and spells that can help
    // them out (or building a level or two of their base skill so they
    // aren't resisted as often). -- bwr
    if (mrs < 6 && coinflip())
        return (false);

    pow = stepdown_value( pow, 30, 40, 100, 120 );

    const int mrchance = (100 + mrs) - pow;
    const int mrch2 = random2(100) + random2(101);

#if DEBUG_DIAGNOSTICS
    snprintf( info, INFO_SIZE, "Power: %d, monster's MR: %d, target: %d, roll: %d",
             pow, mrs, mrchance, mrch2 );

    mpr(info);
#endif

    return ((mrch2 < mrchance) ? true : false);
}                               // end check_mons_magres()

// Enchants all monsters in player's sight.
bool mass_enchantment( int wh_enchant, int pow, int origin )
{
    int i;                      // loop variable {dlb}
    bool msgGenerated = false;
    struct monsters *monster;

    viewwindow(0, false);

    if (pow > 200)
        pow = 200;

    for (i = 0; i < MAX_MONSTERS; i++)
    {
        monster = &menv[i];

        if (monster->type == -1 || !mons_near(monster))
            continue;

        // assuming that the only mass charm is control undead:
        if (wh_enchant == ENCH_CHARM && !mons_friendly(monster))
        {
            if (mons_holiness(monster->type) != MH_UNDEAD)
                continue;

            if (monster->hit_dice * 4 >= random2(pow))
            {
                simple_monster_message(monster, " resists.");
                continue;
            }
        }

        if (wh_enchant != ENCH_CHARM
            || mons_holiness(monster->type) != MH_UNDEAD)
        {
            if (check_mons_magres(monster, pow))
            {
                simple_monster_message(monster, " resists.");
                continue;
            }
        }

        if (mons_near(monster))
        {
            if (mons_has_ench(monster, wh_enchant))
                break;

            if (mons_add_ench(monster, wh_enchant))
            {
                if (player_monster_visible( monster ))
                {
                    // turn message on
                    msgGenerated = true;
                    switch (wh_enchant)
                    {
                    case ENCH_FEAR:
                        simple_monster_message(monster,
                                               " looks frightened!");
                        break;
                    case ENCH_CONFUSION:
                        simple_monster_message(monster,
                                               " looks rather confused.");
                        break;
                    case ENCH_CHARM:
                        simple_monster_message(monster,
                                               " submits to your will.");
                        break;
                    default:
                        // oops, I guess not!
                        msgGenerated = false;
                    }
                }

                // extra check for fear (monster needs to reevaluate behavior)
                if (wh_enchant == ENCH_FEAR)
                    behavior_event( monster, ME_SCARE, origin );
            }
        }                       // end "if mons_near(monster)"
    }                           // end "for i"

    if (!msgGenerated)
        canned_msg(MSG_NOTHING_HAPPENS);

    return msgGenerated;
}                               // end mass_enchantmenet()

/*
   Monster has probably failed save, now it gets enchanted somehow.

   returns MON_RESIST if monster is unaffected due to magic resist.
   returns MON_UNAFFECTED if monster is immune to enchantment
   returns MON_AFFECTED in all other cases (already enchanted, etc)
 */
int mons_ench_f2(struct monsters *monster, struct bolt &pbolt)
{
    bool is_near = mons_near(monster);  // single caluclation permissible {dlb}

    switch (pbolt.colour)      /* put in magic resistance */
    {
    case BCOL_SLOW:         /* 0 = slow monster */
        // try to remove haste,  if monster is hasted
        if (mons_del_ench(monster, ENCH_HASTE))
        {
            if (simple_monster_message(monster, " is no longer moving quickly."))
                pbolt.obviousEffect = true;
            return MON_AFFECTED;
        }

        // not hasted,  slow it
        if (mons_add_ench(monster, ENCH_SLOW))
        {
            // put in an exception for fungi, plants and other things you won't
            // notice slow down.
            if (simple_monster_message(monster, " seems to slow down."))
                pbolt.obviousEffect = true;
        }

        return MON_AFFECTED;

    case BCOL_HASTE:                  // 1 = haste
        if (mons_del_ench(monster, ENCH_SLOW))
        {
            if (simple_monster_message(monster, " is no longer moving slowly."))
                pbolt.obviousEffect = true;
            return MON_AFFECTED;
        }

        // not slowed, haste it
        if (mons_add_ench(monster, ENCH_HASTE))
        {
            // put in an exception for fungi, plants and other things you won't
            // notice speed up.
            if (simple_monster_message(monster, " seems to speed up."))
                pbolt.obviousEffect = true;
        }
        return MON_AFFECTED;

    case BCOL_HEALING:         /* 2 = healing */
        if (heal_monster(monster, 5 + random2( pbolt.hit + 1 ), false))
        {
            if (monster->hit_points == monster->max_hit_points)
            {
                if (simple_monster_message(monster,
                                        "'s wounds heal themselves!"))
                    pbolt.obviousEffect = true;
            }
            else
            {
                if (simple_monster_message(monster, " is healed somewhat."))
                    pbolt.obviousEffect = true;
            }
        }
        return MON_AFFECTED;

    case BCOL_PARALYSIS:                  /* 3 = paralysis */
        monster->speed_increment = 0;

        if (simple_monster_message(monster, " suddenly stops moving!"))
            pbolt.obviousEffect = true;

        if (grd[monster->x][monster->y] == DNGN_LAVA_X
            || grd[monster->x][monster->y] == DNGN_WATER_X)
        {
            if (mons_flies(monster->type) == 1)
            {
                // don't worry about invisibility - you should be able to
                // see if something has fallen into the lava
                if (is_near)
                {
                    strcpy(info, ptr_monam(monster, DESC_CAP_THE));
                    strcat(info, " falls into the ");
                    strcat(info, (grd[monster->x][monster->y] == DNGN_WATER_X)
                                ? "water" : "lava");
                    strcat(info, "!");
                    mpr(info);
                }

                switch (pbolt.thrower)
                {
                case KILL_YOU:
                case KILL_YOU_MISSILE:
                    monster_die(monster, KILL_YOU, pbolt.beam_source);
                    break;      /*  "    " */

                case KILL_MON:
                case KILL_MON_MISSILE:
                    monster_die(monster, KILL_MON_MISSILE,
                                pbolt.beam_source);
                    break;      /* dragon breath &c */
                }
            }
        }
        return MON_AFFECTED;

    case BCOL_CONFUSION:                   /* 4 = confusion */
        if (mons_add_ench(monster, ENCH_CONFUSION))
        {
            // put in an exception for fungi, plants and other things you won't
            // notice becoming confused.
            if (simple_monster_message(monster, " appears confused."))
                pbolt.obviousEffect = true;
        }

        return MON_AFFECTED;

    case BCOL_INVISIBILITY:               /* 5 = invisibility */
        if (mons_add_ench(monster, ENCH_INVIS))
        {
            if (player_see_invis())
                simple_monster_message(monster, " flickers for a moment.");
            else
                simple_monster_message(monster, " flickers and vanishes!");

            pbolt.obviousEffect = true;
        }
        return MON_AFFECTED;

    case BCOL_CHARM:             /* 9 = charm */
        if (mons_add_ench(monster, ENCH_CHARM))
        {
            // put in an exception for fungi, plants and other things you won't
            // notice becoming charmed.
            if (simple_monster_message(monster, " is charmed."))
                pbolt.obviousEffect = true;
        }
        return MON_AFFECTED;

    default:
        break;
    }                           /* end of switch (beam_colour) */

    return MON_AFFECTED;
}                               // end mons_ench_f2()

// actually poisons a monster (w/ message)
void poison_monster(struct monsters *monster, bool fromPlayer, int levels)
{
    bool yourPoison = false;
    int currentPoison;
    int currentStrength = 0;

    if (monster->type == -1)
        return;

    if (mons_res_poison(monster->type) > 0)
        return;

    if (monster->inv[MSLOT_ARMOUR] != NON_ITEM
        && mitm[monster->inv[MSLOT_ARMOUR]].special == SPARM_POISON_RESISTANCE)
    {
        return;
    }

    // who gets the credit if monster dies of poison?
    currentPoison = mons_has_ench(monster, ENCH_POISON_I, ENCH_POISON_IV);
    if (currentPoison != ENCH_NONE)
    {
        currentStrength = currentPoison - ENCH_POISON_I + 1;
    }
    else
    {
        currentPoison = mons_has_ench(monster, ENCH_YOUR_POISON_I, ENCH_YOUR_POISON_IV);
        if (currentPoison != ENCH_NONE)
        {
            currentStrength = currentPoison - ENCH_YOUR_POISON_I + 1;
            yourPoison = true;
        }
    }

    // delete old poison
    mons_del_ench(monster, ENCH_POISON_I, ENCH_POISON_IV);
    mons_del_ench(monster, ENCH_YOUR_POISON_I, ENCH_YOUR_POISON_IV);

    // now, if player poisons the monster at ANY TIME, they should
    // get credit for the kill if the monster dies from poison.  This
    // really isn't that abusable -- GDL.
    if (fromPlayer || yourPoison)
    {
        currentPoison = ENCH_YOUR_POISON_I + currentStrength + levels - 1;

        if (currentPoison > ENCH_YOUR_POISON_IV)
            currentPoison = ENCH_YOUR_POISON_IV;
    }
    else
    {
        currentPoison = ENCH_POISON_I + currentStrength + levels - 1;

        if (currentPoison > ENCH_POISON_IV)
            currentPoison = ENCH_POISON_IV;
    }

    // actually do the poisoning
    if (mons_add_ench(monster, currentPoison))
    {
        simple_monster_message(monster,
            (currentStrength == 0) ?  " looks ill." : " looks even sicker.");
    }

    // finally, take care of deity preferences
    if (fromPlayer)
    {
        naughty(NAUGHTY_POISON, 5 + random2(3)); //jmf: TSO now hates poison
        done_good(GOOD_POISON, 5);      //jmf: had test god who liked poison
    }
}                               // end poison_monster()

// actually napalms a monster (w/ message)
void sticky_flame_monster(int mn, bool fromPlayer, int power)
{
    bool yourFlame = fromPlayer;
    int currentFlame;
    int currentStrength = 0;

    struct monsters *monster = &menv[mn];

    if (monster->type == -1)
        return;

    if (mons_res_fire(monster->type) > 0)
        return;

    if (monster->inv[MSLOT_ARMOUR] != NON_ITEM
        && mitm[monster->inv[MSLOT_ARMOUR]].special == SPARM_FIRE_RESISTANCE)
    {
        return;
    }

    int long_last = 1 + (random2(power) / 2);
    if (long_last > 4)
        long_last = 4;

    // who gets the credit if monster dies of napalm?
    currentFlame = mons_has_ench(monster, ENCH_STICKY_FLAME_I, ENCH_STICKY_FLAME_IV);

    if (currentFlame != ENCH_NONE)
    {
        currentStrength = currentFlame - ENCH_STICKY_FLAME_I;
        yourFlame = false;
    }
    else
    {
        currentFlame = mons_has_ench(monster, ENCH_YOUR_STICKY_FLAME_I,
                                     ENCH_YOUR_STICKY_FLAME_IV);

        if (currentFlame != ENCH_NONE)
        {
            currentStrength = currentFlame - ENCH_YOUR_STICKY_FLAME_I;
            yourFlame = true;
        }
        else
            currentStrength = -1;           // no flame yet!
    }

    // delete old flame
    mons_del_ench(monster, ENCH_STICKY_FLAME_I, ENCH_STICKY_FLAME_IV);
    mons_del_ench(monster, ENCH_YOUR_STICKY_FLAME_I, ENCH_YOUR_STICKY_FLAME_IV);

    // increase poison strength,  cap at 3 (level is 0..3)
    currentStrength += long_last;

    if (currentStrength > 3)
        currentStrength = 3;

    // now, if player flames the monster at ANY TIME, they should
    // get credit for the kill if the monster dies from napalm.  This
    // really isn't that abusable -- GDL.
    if (fromPlayer || yourFlame)
        currentStrength += ENCH_YOUR_STICKY_FLAME_I;
    else
        currentStrength += ENCH_STICKY_FLAME_I;

    // actually do flame
    if (mons_add_ench(monster, currentStrength))
        simple_monster_message(monster, " is covered in liquid fire!");
}                               // end sticky_flame_monster

/*
 * Used by monsters in "planning" which spell to cast. Fires off a "tracer"
 * which tells the monster what it'll hit if it breathes/casts etc.
 *
 * The output from this tracer function is four variables in the beam struct:
 * fr_count, foe_count: a count of how many friends and foes will (probably)
 * be hit by this beam
 * fr_power, foe_power: a measure of how many 'friendly' hit dice it will
 *   affect,  and how many 'unfriendly' hit dice.
 *
 * note that beam properties must be set,  as the tracer will take them
 * into account,  as well as the monster's intelligence.
 *
 */
void fire_tracer(struct monsters *monster, struct bolt &pbolt)
{
    // don't fiddle with any input parameters other than tracer stuff!
    pbolt.isTracer = true;
    pbolt.source_x = monster->x;    // always safe to do.
    pbolt.source_y = monster->y;
    pbolt.beam_source = monster_index(monster);
    pbolt.canSeeInvis = (mons_see_invis(monster->type) != 0);
    pbolt.smartMonster = (mons_intel(monster->type) == I_HIGH ||
                          mons_intel(monster->type) == I_NORMAL);
    pbolt.isFriendly = mons_friendly(monster);

    // init tracer variables
    pbolt.foe_count = pbolt.fr_count = 0;
    pbolt.foe_power = pbolt.fr_power = 0;
    pbolt.foeRatio = 80;        // default - see mons_should_fire()

    // foe ratio for summon gtr. demons & undead -- they may be
    // summoned, but they're hostile and would love nothing better
    // than to nuke the player and his minions
    if (monster->attitude != ATT_FRIENDLY)
        pbolt.foeRatio = 25;

    // fire!
    beam(pbolt);

    // unset tracer flag (convenience)
    pbolt.isTracer = false;
}                               // end tracer_f()


/*
   When a mimic is hit by a ranged attack, it teleports away (the slow way)
   and changes its appearance - the appearance change is in monster_teleport
   in mstuff2.
 */
void mimic_alert(struct monsters *mimic)
{
    if (mons_has_ench(mimic, ENCH_TP_I, ENCH_TP_IV))
        return;

    monster_teleport(mimic, false);
}                               // end mimic_alert()

static bool isBouncy(struct bolt &beam)
{
    // at present, only non-enchantment eletrcical beams bounce.
    if (beam.beam_name[0] != '0' && beam.flavour == BEAM_ELECTRICITY)
        return true;

    return false;
}

static void beam_explodes(struct bolt &beam, int x, int y)
{
    int cloud_type;

    // this will be the last thing this beam does.. set target_x
    // and target_y to hold explosion co'ords.

    beam.target_x = x;
    beam.target_y = y;

    // generic explosion
    if (beam.flavour == BEAM_EXPLOSION || beam.flavour == BEAM_HOLY)
    {
        explosion1(beam);
        return;
    }

    // cloud producer -- POISON BLAST
    if (strcmp(beam.beam_name, "blast of poison") == 0)
    {
        cloud_type = YOU_KILL(beam.thrower) ? CLOUD_POISON : CLOUD_POISON_MON;
        big_cloud( cloud_type, x, y, 0, 7 + random2(5) );
        return;
    }

    // cloud producer -- FOUL VAPOR (SWAMP DRAKE?)
    if (strcmp(beam.beam_name, "foul vapour") == 0)
    {
        cloud_type = YOU_KILL(beam.thrower) ? CLOUD_STINK : CLOUD_STINK_MON;
        big_cloud( cloud_type, x, y, 0, 9 );
        return;
    }

    // special cases - orbs & blasts of cold
    if (strcmp(beam.beam_name, "orb of electricity") == 0
        || strcmp(beam.beam_name, "metal orb") == 0
        || strcmp(beam.beam_name, "great blast of cold") == 0)
    {
        explosion1( beam );
        return;
    }

    // cloud producer only -- stinking cloud
    if (strcmp(beam.beam_name, "ball of vapour") == 0)
    {
        explosion1( beam );
        return;
    }
}

static bool beam_term_on_target(struct bolt &beam)
{

    // generic - all explosion-type beams can be targetted at empty space,
    // and will explode there.  This semantic also means that a creature
    // in the target cell will have no chance to dodge or block,  so we
    // DON'T affect() the cell if this function returns true!

    if (beam.flavour == BEAM_EXPLOSION
        || beam.flavour == BEAM_HOLY)
        return true;

    // POISON BLAST
    if (strcmp(beam.beam_name, "blast of poison") == 0)
        return true;

    // FOUL VAPOR (SWAMP DRAKE)
    if (strcmp(beam.beam_name, "foul vapour") == 0)
        return true;

    // STINKING CLOUD
    if (strcmp(beam.beam_name, "ball of vapour") == 0)
        return true;

    return false;
}

static void beam_drop_object(struct bolt &beam, int inv_number, int x, int y)
{
    // conditions: inv_number >= 0, beam is missile and not tracer.
    if (inv_number < 0 || beam.isTracer || beam.flavour != BEAM_MISSILE)
        return;

    if (YOU_KILL(beam.thrower) // ie if you threw it.
        && (grd[x][y] != DNGN_LAVA && grd[x][y] != DNGN_DEEP_WATER))
    {
        int chance;

        // This represents "fletching" type skills... the values are
        // more based for game balance than reality, though. -- bwr
        switch (you.inv[inv_number].sub_type)
        {
        case MI_NEEDLE: chance = 6 + you.skills[SK_DARTS]     / 6; break;
        case MI_STONE:  chance = 3 + you.skills[SK_SLINGS]    / 4; break;
        case MI_DART:   chance = 2 + you.skills[SK_DARTS]     / 6; break;
        case MI_ARROW:  chance = 2 + you.skills[SK_BOWS]      / 4; break;
        case MI_BOLT:   chance = 2 + you.skills[SK_CROSSBOWS] / 5; break;

        case MI_LARGE_ROCK:
        default:
            chance = 20;
            break;
        }

        if (you.inv[inv_number].base_type != OBJ_MISSILES
            || !one_chance_in(chance))
        {
            copy_item_to_grid( you.inv[inv_number], x, y, 1 );
        }
    }
    else if (MON_KILL(beam.thrower) // monster threw it.
            && (grd[x][y] != DNGN_LAVA && grd[x][y] != DNGN_DEEP_WATER)
            && coinflip())
    {
        copy_item_to_grid( mitm[inv_number], x, y, 1 );
    }                           // if (thing_throw == 2) ...
}

// somewhat complicated BOUNCE function
// returns # of times beam bounces during routine (usually 1)
//
// step 1 is always the step value from the stepping direction.
#define B_HORZ      1
#define B_VERT      2
#define B_BOTH      3

static int bounce(int &step1, int &step2, int w1, int w2, int &n1, int &n2,
    int l1, int l2, int &t1, int &t2, bool topBlocked, bool sideBlocked)
{
    int bounceType = 0;
    int bounceCount = 1;

    if (topBlocked) bounceType = B_HORZ;
    if (sideBlocked) bounceType = B_VERT;
    if (topBlocked && sideBlocked)
    {
        // check for veritcal bounce only
        if ((w2 + step2 - 50)/100 == (w2 - 50)/100)
            bounceType = B_VERT;
        else
            bounceType = B_BOTH;
    }

    switch (bounceType)
    {
        case B_VERT:            // easiest
            n1 = w1;
            n2 = w2 + step2;
            step1 = -step1;
            t1 = n1 / 100;
            t2 = (n2 + 50)/100;
            // check top
            if (t2 != n2/100 && topBlocked)
                t2 = n2/100;
            break;
        case B_HORZ:            // a little tricky
            if (step2 > 0)
                n2 = (100 + 200*(w2/100)) - (w2 + step2);
            else
                n2 = (100 + 200*((w2 - 50)/100)) - (w2 + step2);
            n1 = w1 + step1;
            t1 = n1 /100;
            t2 = (n2 + 50) / 100;
            step2 = -step2;
            break;
        case B_BOTH:
            // vertical:
            n1 = w1;
            t1 = l1;
            t2 = l2;
            // horizontal:
            if (step2 > 0)
                n2 = (100 + 200*(w2/100)) - (w2 + step2);
            else
                n2 = (100 + 200*((w2 - 50)/100)) - (w2 + step2);
            // reverse both directions
            step1 =- step1;
            step2 =- step2;
            bounceCount = 2;
            break;
        default:
            bounceCount = 0;
            break;
    }

    return bounceCount;
}

static bool fuzzyLine(int nx, int ny, int &tx, int &ty, int lx, int ly,
    int stepx, int stepy, bool roundX, bool roundY)
{
    bool fuzzyOK = false;
    int fx, fy;                 // fuzzy x,y

    // BEGIN fuzzy line algorithm
    fx = tx;
    fy = ty;
    if (roundY)
    {
        // try up
        fy = (ny + 100) / 100;
        // check for monotonic
        if (fy != ty && ((stepy>0 && fy >= ly)
            || (stepy<0 && fy <= ly)))
            fuzzyOK = true;
        // see if up try is blocked
        if (fuzzyOK && grd[tx][fy] < MINMOVE)
            fuzzyOK = false;

        // try down
        if (!fuzzyOK)
            fy = ny / 100;
        // check for monotonic
        if (fy != ty && ((stepy>0 && fy >= ly)
            || (stepy<0 && fy <= ly)))
            fuzzyOK = true;
        if (fuzzyOK && grd[tx][fy] < MINMOVE)
            fuzzyOK = false;
    }
    if (roundX)
    {
        // try up
        fx = (nx + 100) / 100;
        // check for monotonic
        if (fx != tx && ((stepx>0 && fx >= lx)
            || (stepx<0 && fx <= lx)))
            fuzzyOK = true;
        // see if up try is blocked
        if (fuzzyOK && grd[fx][ty] < MINMOVE)
            fuzzyOK = false;

        // try down
        if (!fuzzyOK)
            fx = nx / 100;
        // check for monotonic
        if (fx != tx && ((stepx>0 && fx >= lx)
            || (stepx<0 && fx <= lx)))
            fuzzyOK = true;
        if (fuzzyOK && grd[fx][ty] < MINMOVE)
            fuzzyOK = false;
    }
    // END fuzzy line algorithm

    if (fuzzyOK)
    {
        tx = fx;
        ty = fy;
    }

    return fuzzyOK;
}

// affects a single cell.
// returns the amount of extra range 'used up' by this beam
// during the affectation.
//
// pseudo-code:
//
// 1. If wall, and wall affecting non-tracer, affect the wall.
//  1b.  If for some reason the wall-affect didn't make it into
//      a non-wall, return                      affect_wall()
// 2. for non-tracers, produce cloud effects    affect_place_clouds()
// 3. if cell holds player, affect player       affect_player()
// 4. if cell holds monster, affect monster     affect_monster()
// 5. return range used affectation.

static int affect(struct bolt &beam, int x, int y)
{
    // extra range used by hitting something
    int rangeUsed = 0;

    if (grd[x][y] < MINMOVE)
    {
        if (beam.isTracer)          // tracers always stop on walls.
            return BEAM_STOP;

        if (affectsWalls(beam))
        {
            rangeUsed += affect_wall(beam, x, y);
        }
        // if it's still a wall,  quit - we can't do anything else to
        // a wall.  Otherwise effects (like clouds, etc) are still possible.
        if (grd[x][y] < MINMOVE)
            return rangeUsed;
    }

    // grd[x][y] will NOT be a wall for the remainder of this function.

    // if not a tracer, place clouds
    if (!beam.isTracer)
        rangeUsed += affect_place_clouds(beam, x, y);

    // if player is at this location,  try to affect unless term_on_target
    if (x == you.x_pos && y == you.y_pos)
    {
        if (beam_term_on_target(beam) && !beam.isExplosion)
            return BEAM_STOP;

        rangeUsed += affect_player(beam);
    }

    // if there is a monster at this location,  affect it
    // submerged monsters aren't really there -- bwr
    int mid = mgrd[x][y];
    if (mid != NON_MONSTER && !mons_has_ench( &menv[mid], ENCH_SUBMERGED ))
    {
        if (beam_term_on_target(beam) && !beam.isExplosion)
            return BEAM_STOP;

        struct monsters* monster = &menv[mid];
        rangeUsed += affect_monster(beam, monster);
    }

    return rangeUsed;
}

static bool affectsWalls(struct bolt &beam)
{
    // don't know of any explosion that affects walls.  But change it here
    // if there is.
    if (beam.isExplosion)
        return false;

    // digging
    if (beam.colour == BCOL_DIGGING && beam.beam_name[0] == '0')
        return true;

    // disintegration (or powerful disruption)
    // but not needles (which happen to be white.  Argh!  What a hack.
    if (beam.colour == BCOL_DISINTEGRATION && beam.flavour != BEAM_COLD
        && beam.hit >= 20 && beam.type != SYM_MISSILE)
    {
        return true;
    }

    // eye of devestation?
    if (beam.flavour == BEAM_NUKE)
        return true;

    return false;
}

// return amount of extra range used up by affectation of this wall.
static int  affect_wall(struct bolt &beam, int x, int y)
{
    int rangeUsed = 0;

    // DIGGING
    if (beam.colour == BCOL_DIGGING && beam.beam_name[0] == '0')
    {
        if (grd[x][y] == DNGN_STONE_WALL
            || grd[x][y] == DNGN_METAL_WALL
            || x <= 5 || x >= (GXM - 5)
            || y <= 5 || y >= (GYM - 5))
            return 0;

        if (grd[x][y] == DNGN_ROCK_WALL)
        {
            grd[x][y] = DNGN_FLOOR;

            if (!beam.msgGenerated)
            {
                if (!silenced(you.x_pos, you.y_pos))
                {
                    mpr("You hear a grinding noise.");
                    beam.obviousEffect = true;
                }
                beam.msgGenerated = true;
            }
        }
        return rangeUsed;
    }
    // END DIGGING EFFECT

    // NUKE / DISRUPT
    if ((beam.colour == BCOL_DISINTEGRATION
            && beam.flavour != BEAM_COLD
            && beam.hit >= 20)
        || beam.flavour == BEAM_NUKE)
    {
        int targ_grid = grd[ x ][ y ];

        if ((targ_grid == DNGN_ROCK_WALL || targ_grid == DNGN_WAX_WALL)
             && !(x <= 6 || y <= 6 || x >= (GXM - 6) || y >= (GYM - 6)))
        {
            grd[ x ][ y ] = DNGN_FLOOR;
            if (!silenced(you.x_pos, you.y_pos))
            {
                mpr("You hear a grinding noise.");
                beam.obviousEffect = true;
            }
        }

        if (targ_grid == DNGN_ORCISH_IDOL || (targ_grid >= DNGN_SILVER_STATUE
                && targ_grid <= DNGN_STATUE_39))
        {
            grd[ x ][ y ] = DNGN_FLOOR;

            if (!silenced(you.x_pos, you.y_pos))
            {
                if (!see_grid( x, y ))
                    mpr("You hear a hideous screaming!");
                else
                    mpr("The statue screams as its substance crumbles away!");
            }
            else
            {
                if (see_grid(x,y))
                    mpr("The statue twists and shakes as its substance crumbles away!");
            }

            beam.obviousEffect = 1;
        }

        return BEAM_STOP;
    }

    return rangeUsed;
}

static int affect_place_clouds(struct bolt &beam, int x, int y)
{
    int cloud_type;

    if (beam.isExplosion)
    {
        affect_place_explosion_clouds( beam, x, y );
        return 0;       // return value irrelevant for explosions
    }

    // check for CLOUD HITS
    if (env.cgrid[x][y] != EMPTY_CLOUD)     // hit a cloud
    {
        // polymorph randomly changes clouds in its path
        if (beam.beam_name[0] == '0' && beam.colour == BCOL_POLYMORPH)
            env.cloud[ env.cgrid[x][y] ].type = 1 + random2(8);

        // now exit (all enchantments)
        if (beam.beam_name[0] == '0')
            return 0;

        int clouty = env.cgrid[x][y];

        // fire cancelling cold & vice versa
        if (((env.cloud[clouty].type == CLOUD_COLD
                || env.cloud[clouty].type == CLOUD_COLD_MON)
            && (beam.flavour == BEAM_FIRE
                || beam.flavour == BEAM_LAVA))
            || ((env.cloud[clouty].type == CLOUD_FIRE
                || env.cloud[clouty].type == CLOUD_FIRE_MON)
            && beam.flavour == BEAM_COLD))
        {
            if (!silenced(x, y)
                && !silenced(you.x_pos, you.y_pos))
            {
                mpr("You hear a sizzling sound!");
            }

            delete_cloud( clouty );
            return 5;
        }
    }

    // POISON BLAST
    if (strcmp(beam.beam_name, "blast of poison") == 0)
    {
        cloud_type = YOU_KILL(beam.thrower) ? CLOUD_POISON : CLOUD_POISON_MON;

        place_cloud( cloud_type, x, y, random2(4) + 2 );
    }

    // FIRE/COLD over water/lava
    if ( (grd[x][y] == DNGN_LAVA && beam.flavour == BEAM_COLD)
        || ((grd[x][y] == DNGN_DEEP_WATER || grd[x][y] == DNGN_SHALLOW_WATER)
              && beam.flavour == BEAM_FIRE) )
    {
        place_cloud( CLOUD_STEAM, x, y, 2 + random2(5) );
    }

    // ORB OF ENERGY
    if (strcmp(beam.beam_name, "orb of energy") == 0)
        place_cloud( CLOUD_PURP_SMOKE, x, y, random2(5) + 1 );

    // GREAT BLAST OF COLD
    if (strcmp(beam.beam_name, "great blast of cold") == 0)
        place_cloud( CLOUD_COLD, x, y, random2(5) + 3 );


    // BALL OF STEAM
    if (strcmp(beam.beam_name, "ball of steam") == 0)
    {
        place_cloud( CLOUD_STEAM, x, y, random2(5) + 1 );
    }

    // STICKY FLAME
    if (strcmp(beam.beam_name, "sticky flame") == 0)
    {
        place_cloud( CLOUD_BLACK_SMOKE, x, y, random2(4) + 1 );
    }

    // POISON GAS
    if (strcmp(beam.beam_name, "poison gas") == 0)
    {
        cloud_type = YOU_KILL(beam.thrower) ? CLOUD_POISON : CLOUD_POISON_MON;
        place_cloud( cloud_type, x, y, random2(4) + 2 );
    }

    return 0;
}

// following two functions used with explosions:

static void affect_place_explosion_clouds(struct bolt &beam, int x, int y)
{
    int duration;

    // first check: FIRE/COLD over water/lava
    if ( (grd[x][y] == DNGN_LAVA && beam.flavour == BEAM_COLD)
        || ((grd[x][y] == DNGN_DEEP_WATER || grd[x][y] == DNGN_SHALLOW_WATER)
              && beam.flavour == BEAM_FIRE) )
    {
        place_cloud(CLOUD_STEAM, x, y, 2 + random2(5));
        return;
    }

    // then check for more specific explosion cloud types.
    if (stricmp(beam.beam_name, "ice storm") == 0)
    {
        place_cloud(CLOUD_COLD, x, y, 2 + random2avg(5, 2));
    }

    if (stricmp(beam.beam_name, "stinking cloud") == 0)
    {
        duration =  1 + random2(4) + random2((beam.ench_power / 50) + 1);
        place_cloud(CLOUD_STINK, x, y, duration);
    }

    if (strcmp(beam.beam_name, "great blast of fire") == 0)
    {
        duration = 1 + random2(5) + (random2( beam.ench_power ) / 5)
                                  + (random2( beam.ench_power ) / 5);

        if (duration > 20)
            duration = 20 + random2(4);

        place_cloud( CLOUD_FIRE, x, y, duration );

        if (grd[x][y] == DNGN_FLOOR && mgrd[x][y] == NON_MONSTER
            && one_chance_in(4))
        {
            mons_place( MONS_FIRE_VORTEX, BEH_HOSTILE, MHITNOT, true, x, y );
        }
    }
}

static void affect_items(struct bolt &beam, int x, int y)
{
    char objs_vulnerable = -1;

    switch (beam.flavour)
    {
    case BEAM_FIRE:
    case BEAM_LAVA:
        objs_vulnerable = OBJ_SCROLLS;
        break;
    case BEAM_COLD:
        objs_vulnerable = OBJ_POTIONS;
        break;
    case BEAM_SPORE:
        objs_vulnerable = OBJ_FOOD;
        break;
    }

    if (stricmp(beam.beam_name, "hellfire") == 0)
        objs_vulnerable = OBJ_SCROLLS;

    if (igrd[x][y] != NON_ITEM)
    {
        if (objs_vulnerable != -1 &&
            mitm[igrd[x][y]].base_type == objs_vulnerable)
        {
            destroy_item( igrd[ x ][ y ] );

            if (objs_vulnerable == OBJ_SCROLLS && see_grid(x,y))
            {
                mpr("You see a puff of smoke.");
            }

            if (objs_vulnerable == OBJ_POTIONS && !silenced(x,y)
                && !silenced(you.x_pos, you.y_pos))
            {
                mpr("You hear glass shatter.");
            }
        }
    }
}

// return amount of extra range used up by affectation of the player
static int affect_player(struct bolt &beam)
{
    int beamHit;

    // digging -- don't care.
    if (beam.beam_name[0] == '0' && beam.colour == BCOL_DIGGING)
        return 0;

    // check for tracer
    if (beam.isTracer)
    {
        // check can see player
        if (beam.canSeeInvis || !you.invis)
        {
            if (beam.isFriendly)
            {
                beam.fr_count += 1;
                beam.fr_power += you.experience_level;
            }
            else
            {
                beam.foe_count += 1;
                beam.foe_power += you.experience_level;
            }
        }
        return (range_used_on_hit(beam));
    }

    // BEGIN real beam code
    beam.msgGenerated = true;

    // use beamHit,  NOT beam.hit,  for modification of tohit.. geez!
    beamHit = beam.hit;

    if (beam.beam_name[0] != '0')
    {
        // BEGIN BEAM/MISSILE
        int dodge = random2limit(player_evasion(),40) + random2(you.dex)/3 - 2;

        // only beams can be dodged
        if (beam.isBeam && !beam.isExplosion && !beam.aimedAtFeet)
        {
            if (player_light_armour()
                && !beam.aimedAtFeet && coinflip())
            {
                exercise(SK_DODGING, 1);
            }

            if (you.duration[DUR_REPEL_MISSILES]
                || you.mutation[MUT_REPULSION_FIELD] == 3)
            {
                beamHit -= random2(beamHit / 2);
            }

            if (you.duration[DUR_DEFLECT_MISSILES])
                beamHit = random2(beamHit / 3);

            if (beamHit < dodge)
            {
                strcpy(info, "The ");
                strcat(info, beam.beam_name);
                strcat(info, " misses you.");
                mpr(info);
                return 0;           // no extra used by miss!
            }
        }

        if (!beam.isBeam && !beam.isExplosion)
        {
            // non-beams can be blocked or dodged
            if (you.equip[EQ_SHIELD] != -1)
            {
                if (!beam.aimedAtFeet)
                {
                    if (coinflip())
                        exercise(SK_SHIELDS, 1);
                }
            }

            int hit = random2( beam.hit * 5
                                + 10 * you.shield_blocks * you.shield_blocks );

            int block = random2(player_shield_class()) + (random2(you.dex) / 5) - 1;

            if (player_shield_class() > 0 && hit < block)
            {
                you.shield_blocks++;
                strcpy(info, "You block the ");
                strcat(info, beam.beam_name);
                strcat(info, ".");
                mpr(info);
                return BEAM_STOP;
            }

            if (player_light_armour() && !beam.aimedAtFeet
                && coinflip())
                exercise(SK_DODGING, 1);

            if (you.duration[DUR_REPEL_MISSILES]
                || you.mutation[MUT_REPULSION_FIELD] == 3)
            {
                beamHit = random2(beamHit);
            }


            // miss message
            if (beamHit < dodge || you.duration[DUR_DEFLECT_MISSILES])
            {
                strcpy(info, "The ");
                strcat(info, beam.beam_name);
                strcat(info, " misses you.");
                return 0;
            }
        }
    }
    else
    {
        // BEGIN enchantment beam
        if (beam.colour != BCOL_HASTE && beam.colour != BCOL_INVISIBILITY
            && beam.colour != BCOL_HEALING
            && ((beam.colour != BCOL_TELEPORT
                    && beam.colour != BCOL_BANISH)
                || !beam.aimedAtFeet))
        {
            if (you_resist_magic(beam.ench_power))
            {
                canned_msg(MSG_YOU_RESIST);
                return range_used_on_hit(beam);
            }
        }

        // these colors are misapplied - see mons_ench_f2() {dlb}
        switch (beam.colour)
        {
        case BCOL_SLOW:
            potion_effect(POT_SLOWING, beam.ench_power);
            beam.obviousEffect = true;
            break;     // slow

        case BCOL_HASTE:
            potion_effect(POT_SPEED, beam.ench_power);
            beam.obviousEffect = true;
            break;     // haste

        case BCOL_HEALING:
            potion_effect(POT_HEAL_WOUNDS, beam.ench_power);
            beam.obviousEffect = true;
            break;     // heal (heal wounds potion eff)

        case BCOL_PARALYSIS:
            potion_effect(POT_PARALYSIS, beam.ench_power);
            beam.obviousEffect = true;
            break;     // paralysis

        case BCOL_CONFUSION:
            potion_effect(POT_CONFUSION, beam.ench_power);
            beam.obviousEffect = true;
            break;     // confusion

        case BCOL_INVISIBILITY:
            potion_effect(POT_INVISIBILITY, beam.ench_power);
            beam.obviousEffect = true;
            break;     // invisibility

            // 6 is used by digging

        case BCOL_TELEPORT:
            you_teleport();
            beam.obviousEffect = true;
            break;

        case BCOL_POLYMORPH:
            mpr("This is polymorph other only!");
            beam.obviousEffect = true;
            break;

        case BCOL_CHARM:
            potion_effect(POT_CONFUSION, beam.ench_power);
            beam.obviousEffect = true;
            break;     // enslavement - confusion?

        case BCOL_BANISH:
            if (you.level_type == LEVEL_ABYSS)
            {
                mpr("You feel trapped.");
                break;
            }
            mpr("You are cast into the Abyss!");
            more();
            banished(DNGN_ENTER_ABYSS);
            beam.obviousEffect = true;
            break;     // banishment to the abyss

        case BCOL_PAIN:      // pain
            if (you.is_undead || you.mutation[MUT_TORMENT_RESISTANCE])
            {
                mpr("You are unaffected.");
                break;
            }

            mpr("Pain shoots through your body!");

            if (YOU_KILL(beam.thrower))
                ouch(1 + random2(beam.hit), 0, KILLED_BY_TARGETTING);
            else
            {
                ouch(1 + random2(beam.hit), beam.beam_source,
                     KILLED_BY_BEAM);
            }
            beam.obviousEffect = true;
            break;

        case BCOL_DISINTEGRATION:
            mpr("You are blasted!");
            if (YOU_KILL(beam.thrower))
                ouch(random2(beam.hit), 0, KILLED_BY_TARGETTING);
            else
            {
                ouch(random2(beam.hit), beam.beam_source,
                     KILLED_BY_BEAM);
            }
            beam.obviousEffect = true;
            break;

        default:
            // _all_ enchantments should be enumerated here!
            mpr("Software bugs nibble your toes!");
            break;
        }               // end of switch (beam.colour)

        // regardless of affect, we need to know if this is a stopper
        // or not - it seems all of the above are.
        return range_used_on_hit(beam);

        // END enchantment beam
    }

    // THE BEAM IS NOW GUARANTEED TO BE A NON-ENCHANTMENT WHICH HIT

    strcpy( info, "The " );
    strcat( info, beam.beam_name );
    strcat( info, beam.isExplosion ? " engulfs you!" : " hits you!" );
    mpr(info);

    int hurted = 0;
    int burn_power = (beam.isExplosion) ? 5 : ((beam.isBeam) ? 3 : 2);

    // bwross: Note the conversion here to regular dice (minimum of 1, not 0)
    if (beam.damage > 100)
    {
        hurted += 1 + random2(beam.damage - 100);
        hurted += 1 + random2(beam.damage - 100);
        hurted += 1 + random2(beam.damage - 100);
    }
    else
        hurted += 1 + random2(beam.damage);

    hurted -= random2(1 + player_AC());

    // shrapnel
    if (beam.flavour == BEAM_FRAG && !player_light_armour())
    {
        hurted -= random2(1 + player_AC());
        hurted -= random2(1 + player_AC());
    }

    // shield exercise for non-beams taken care of already..
    // no shield exercise for explosions (awww..)
    if (!beam.isExplosion && beam.isBeam && you.equip[EQ_SHIELD] != -1)
    {
        if (!beam.aimedAtFeet)
            exercise(SK_SHIELDS, (random2(3)) / 2);
    }

    if (you.equip[EQ_BODY_ARMOUR] != -1)
    {
        if (!player_light_armour() && one_chance_in(4)
            && random2(1000) <= mass_item( you.inv[you.equip[EQ_BODY_ARMOUR]] ))
        {
            exercise(SK_ARMOUR, 1);
        }
    }

    if (hurted < 0)
        hurted = 0;

    hurted = check_your_resists(hurted, beam.flavour);

    // poisoning
    if (strstr(beam.beam_name, "poison") != NULL
        && beam.flavour != BEAM_POISON && !player_res_poison())
    {
        if (hurted || (strstr(beam.beam_name, "needle") != NULL
                        && random2(100) < 90 - (3 * player_AC())))
        {
            mpr("You are poisoned.");
            you.poison += 1 + random2(3);
        }
    }

    // sticky flame
    if (strcmp(beam.beam_name, "sticky flame") == 0
        && (you.species != SP_MOTTLED_DRACONIAN
        || you.experience_level < 6))
    {
        if (!player_equip( EQ_BODY_ARMOUR, ARM_MOTTLED_DRAGON_ARMOUR ))
            you.duration[DUR_LIQUID_FLAMES] += random2avg(7, 3) + 1;
    }

    // simple cases for scroll burns
    if (beam.flavour == BEAM_LAVA || stricmp(beam.beam_name, "hellfire") == 0)
        scrolls_burn(burn_power, OBJ_SCROLLS);

    // more complex (geez..)
    if (beam.flavour == BEAM_FIRE && strcmp(beam.beam_name, "ball of steam") != 0)
        scrolls_burn(burn_power, OBJ_SCROLLS);

    // potions exploding
    if (beam.flavour == BEAM_COLD)
        scrolls_burn(burn_power, OBJ_POTIONS);

    // spore pops
    if (beam.isExplosion && beam.flavour == BEAM_SPORE)
        scrolls_burn(2, OBJ_FOOD);

#if DEBUG_DIAGNOSTICS
    snprintf( info, INFO_SIZE, "Damage: %d", hurted );
    mpr( info );
#endif

    if (YOU_KILL(beam.thrower))
        ouch(hurted, 0, KILLED_BY_TARGETTING);
    else
    {
        char killer = (beam.flavour == BEAM_SPORE)?KILLED_BY_SPORE:
            KILLED_BY_BEAM;
        ouch(hurted, beam.beam_source, killer);
    }

    return range_used_on_hit(beam);
}

// return amount of range used up by affectation of this monster
static int  affect_monster(struct bolt &beam, struct monsters *mon)
{
    int tid = mgrd[mon->x][mon->y];
    int hurt;
    int hurt_final;

    // digging -- don't care.
    if (beam.beam_name[0] == '0' && beam.colour == BCOL_DIGGING)
        return (0);

    // fire storm creates these, so we'll avoid affecting them
    if (strcmp(beam.beam_name, "great blast of fire") == 0
        && mon->type == MONS_FIRE_VORTEX)
    {
        return (0);
    }

    // check for tracer
    if (beam.isTracer)
    {
        // check can see other monster
        if (!beam.canSeeInvis && mons_has_ench(&menv[tid], ENCH_INVIS))
        {
            // can't see this monster, ignore it
            return 0;
        }
    }

    if (beam.beam_name[0] == '0')
    {
        if (beam.isTracer)
        {
            // enchant case -- enchantments always hit, so update target immed.
            if (beam.isFriendly ^ mons_friendly(mon))
            {
                beam.foe_count += 1;
                beam.foe_power += mons_power(tid);
            }
            else
            {
                beam.fr_count += 1;
                beam.fr_power += mons_power(tid);
            }

            return range_used_on_hit(beam);
        }

        // BEGIN non-tracer enchantment

        // nasty enchantments will annoy the monster, and are considered
        // naughty (even if a monster might resist)
        if (nasty_beam(mon, beam))
        {
            if (mons_friendly(mon) &&  YOU_KILL(beam.thrower))
                naughty(NAUGHTY_ATTACK_FRIEND, 5);

            behavior_event( mon, ME_ANNOY,
                        YOU_KILL(beam.thrower) ? MHITYOU : beam.beam_source );
        }
        else
        {
            behavior_event( mon, ME_ALERT,
                        YOU_KILL(beam.thrower) ? MHITYOU : beam.beam_source );
        }

        // !@#*( affect_monster_enchantment() has side-effects on
        // the beam structure which screw up range_used_on_hit(),
        // so call it now and store.
        int rangeUsed = range_used_on_hit(beam);

        // now do enchantment affect
        int ench_result = affect_monster_enchantment(beam, mon);
        switch(ench_result)
        {
            case MON_RESIST:
                if (simple_monster_message(mon, " resists."))
                    beam.msgGenerated = true;
                break;
            case MON_UNAFFECTED:
                if (simple_monster_message(mon, " is unaffected."))
                    beam.msgGenerated = true;
                break;
            default:
                break;
        }
        return rangeUsed;

        // END non-tracer enchantment
    }


    // BEGIN non-enchantment (could still be tracer)
    if (mons_has_ench( mon, ENCH_SUBMERGED ) && !beam.aimedAtFeet)
        return 0;                   // missed me!

    // we need to know how much the monster _would_ be hurt by this,  before
    // we decide if it actually hits.

    hurt = 0;

    // bwross: Note the conversion here to regular dice (minimum of 1, not 0)
    if (beam.damage > 100)
    {
        hurt += 1 + random2(beam.damage - 100);
        hurt += 1 + random2(beam.damage - 100);
        hurt += 1 + random2(beam.damage - 100);
    }
    else
    {
        hurt += 1 + random2(beam.damage);
    }

    hurt_final = hurt;

    if (beam.isTracer)
        hurt_final -= mon->armour_class / 2;
    else
        hurt_final -= random2(1 + mon->armour_class);

    if (beam.flavour == BEAM_FRAG)
    {
        hurt_final -= random2(1 + mon->armour_class);
        hurt_final -= random2(1 + mon->armour_class);
    }

    if (hurt_final < 1)
    {
        hurt_final = 0;
    }

    // check monster resists,  _without_ side effects (since the
    // beam/missile might yet miss!)
    hurt_final = mons_adjust_flavoured(mon, beam, hurt_final, false);

#if DEBUG_DIAGNOSTICS
    if (!beam.isTracer)
    {
        snprintf( info, INFO_SIZE, "Monster: %s; Damage: %d",
                 ptr_monam( mon, DESC_PLAIN ), hurt_final );
        mpr( info );
    }
#endif

    // now,  we know how much this monster would (probably) be
    // hurt by this beam.
    if (beam.isTracer)
    {
        if (hurt_final != 0)
        {
            // monster could be hurt somewhat,  but only apply the
            // monster's power based on how badly it is affected.
            // For example,  if a fire giant (power 16) threw a
            // fireball at another fire giant,  and it only took
            // 1/3 damage,  then power of 5 would be applied to
            // foe_power or fr_power.
            if (beam.isFriendly ^ mons_friendly(mon))
            {
                beam.foe_count += 1;
                beam.foe_power += hurt_final * mons_power(tid) / hurt;
            }
            else
            {
                beam.fr_count += 1;
                beam.fr_power += hurt_final * mons_power(tid) / hurt;
            }
        }
        // either way, we could hit this monster, so return range used
        return range_used_on_hit(beam);
    }
    // END non-enchantment (could still be tracer)

    // BEGIN real non-enchantment beam

    // player beams which hit friendly MIGHT annoy them and be considered
    // naughty if they do much damage (this is so as not to penalize
    // players that fling fireballs into a melee with fire elementals
    // on their side - the elementals won't give a sh*t,  after all)

    if (nasty_beam(mon, beam))
    {
        // could be naughty if it's your beam & the montster is friendly
        if (mons_friendly(mon) && YOU_KILL(beam.thrower))
        {
            // but did you do enough damage to piss them off?
            if (hurt_final > mon->hit_dice / 3)
            {
                naughty(NAUGHTY_ATTACK_FRIEND, 5);
                behavior_event( mon, ME_ANNOY,
                        YOU_KILL(beam.thrower) ? MHITYOU : beam.beam_source );
            }
        }
        else
        {
            behavior_event(mon, ME_ANNOY,
                    YOU_KILL(beam.thrower) ? MHITYOU : beam.beam_source);
        }
    }

    // explosions always 'hit'
    if (!beam.isExplosion && beam.hit < random2(mon->evasion))
    {
        // if the PLAYER cannot see the monster, don't tell them anything!
        if (player_monster_visible( &menv[tid] ) && mons_near(mon))
        {
            strcpy(info, "The ");
            strcat(info, beam.beam_name);
            strcat(info, " misses ");
            strcat(info, ptr_monam(mon, DESC_NOCAP_THE));
            strcat(info, ".");
            mpr(info);
        }
        return 0;
    }

    // the beam hit.
    if (mons_near(mon))
    {
        strcpy(info, "The ");
        strcat(info, beam.beam_name);
        strcat(info, beam.isExplosion?" engulfs ":" hits ");

        if (player_monster_visible( &menv[tid] ))
            strcat(info, ptr_monam(mon, DESC_NOCAP_THE));
        else
            strcat(info, "something");

        strcat(info, ".");
        mpr(info);
    }
    else
    {
        // the player might hear something,
        // if _they_ fired a missile (not beam)
        if (!silenced(you.x_pos, you.y_pos) && beam.flavour == BEAM_MISSILE
                && YOU_KILL(beam.thrower))
        {
            strcpy(info, "The ");
            strcat(info, beam.beam_name);
            strcat(info, " hits something.");
            mpr(info);
        }
    }

    // note that hurt_final was calculated above, so we don't need it again.
    // just need to apply flavoured specials (since we called with
    // doFlavouredEffects = false above)
    mons_adjust_flavoured(mon, beam, hurt_final);

    // now hurt monster
    hurt_monster(mon, hurt_final);

    int thrower = YOU_KILL(beam.thrower) ? KILL_YOU_MISSILE : KILL_MON_MISSILE;

    if (mon->hit_points < 1)
    {
        monster_die(mon, thrower, beam.beam_source);
    }
    else
    {
        if (thrower == KILL_YOU_MISSILE && mons_near(mon))
            print_wounds(mon);

        // sticky flame
        if (strcmp(beam.beam_name, "sticky flame") == 0)
            sticky_flame_monster(tid, YOU_KILL(beam.thrower), hurt_final);


        /* looks for missiles which aren't poison but
           are poison*ed* */
        if (strstr(beam.beam_name, "poison") != NULL
            && beam.flavour != BEAM_POISON)
        {
            if (strstr(beam.beam_name, "needle") != NULL
                && random2(100) < 90 - (3 * mon->armour_class))
            {
                poison_monster( mon, YOU_KILL(beam.thrower), 2 );
            }
            else if (random2(hurt_final) - random2(mon->armour_class) > 0)
            {
                poison_monster( mon, YOU_KILL(beam.thrower) );
            }
        }

        if (mons_charclass(mon->type) == MONS_GOLD_MIMIC)
            mimic_alert(mon);
    }

    return range_used_on_hit(beam);
}

static int affect_monster_enchantment(struct bolt &beam, struct monsters *mon)
{
    if (beam.colour == BCOL_TELEPORT) // teleportation
    {
        if (check_mons_magres(mon, beam.ench_power)
            && !beam.aimedAtFeet)
        {
            return MON_RESIST;
        }

        if (simple_monster_message(mon, " looks slightly unstable."))
            beam.obviousEffect = true;

        monster_teleport(mon, false);

        return MON_AFFECTED;
    }

    if (beam.colour == BCOL_POLYMORPH)
    {
        if (mons_holiness( mon->type ) != MH_NATURAL)
            return MON_UNAFFECTED;

        if (check_mons_magres(mon, beam.ench_power))
            return MON_RESIST;

        if (monster_polymorph(mon, RANDOM_MONSTER, 100))
            beam.obviousEffect = true;

        return MON_AFFECTED;
    }

    if (beam.colour == BCOL_BANISH)
    {
        if (check_mons_magres(mon, beam.ench_power))
            return MON_RESIST;

        if (you.level_type == LEVEL_ABYSS)
        {
            simple_monster_message(mon, " wobbles for a moment.");
        }
        else
            monster_die(mon, KILL_RESET, beam.beam_source);

        beam.obviousEffect = true;
        return MON_AFFECTED;
    }

    if (beam.colour == BCOL_DEGENERATE)
    {
        if (mons_holiness(mon->type) != MH_NATURAL
            || mon->type == MONS_PULSATING_LUMP)
        {
            return MON_UNAFFECTED;
        }

        if (check_mons_magres(mon, beam.ench_power))
            return MON_RESIST;

        if (monster_polymorph(mon, MONS_PULSATING_LUMP, 100))
            beam.obviousEffect = true;

        return MON_AFFECTED;
    }

    if (beam.colour == BCOL_DISPEL_UNDEAD)
    {
        if (mons_holiness(mon->type) != MH_UNDEAD)
            return MON_UNAFFECTED;

        if (simple_monster_message(mon, " convulses!"))
            beam.obviousEffect = true;

        hurt_monster(mon, random2(beam.hit) + random2(beam.hit) +
                     random2(beam.hit));

        goto deathCheck;
    }

    if ((beam.colour == BCOL_ENSLAVE_UNDEAD
            && mons_holiness(mon->type) == MH_UNDEAD)
        || (beam.colour == BCOL_ENSLAVE_DEMON
            && mons_holiness(mon->type) == MH_DEMONIC))
    {
#if DEBUG_DIAGNOSTICS
        snprintf( info, INFO_SIZE, "HD: %d; pow: %d", mon->hit_dice, beam.ench_power );
        mpr( info );
#endif

        if (mon->hit_dice * 4 >= random2(beam.ench_power))
            return MON_RESIST;

        simple_monster_message(mon, " is enslaved.");
        beam.obviousEffect = true;

        // wow, permanent enslaving
        mon->attitude = ATT_FRIENDLY;
        return MON_AFFECTED;
    }

    //
    // Everything past this point must pass this magic resistance test.
    //
    if (check_mons_magres(mon, beam.ench_power)
        && beam.colour != BCOL_HASTE && beam.colour != BCOL_HEALING
        && beam.colour != BCOL_INVISIBILITY)
    {
        return MON_RESIST;
    }

    if (beam.colour == BCOL_PAIN)      /* pain/agony */
    {
        if (mons_holiness(mon->type) == MH_UNDEAD
            || mons_holiness(mon->type) == MH_DEMONIC)
        {
            return MON_UNAFFECTED;
        }

        if (simple_monster_message(mon, " convulses in agony!"))
            beam.obviousEffect = true;

        if (strstr(beam.beam_name, "agony") != NULL)
        {
            // AGONY
            mon->hit_points = mon->hit_points / 2;

            if (mon->hit_points < 1)
                mon->hit_points = 1;
        }
        else
        {
            // PAIN
            hurt_monster(mon, 1 + random2(beam.hit));
        }

        goto deathCheck;
    }

    if (beam.colour == BCOL_DISINTEGRATION)     /* disrupt/disintegrate */
    {
        if (simple_monster_message(mon, " is blasted."))
            beam.obviousEffect = true;

        hurt_monster(mon, 1 + random2(beam.hit));

        goto deathCheck;
    }


    if (beam.flavour == BEAM_SLEEP)
    {
        //jmf: FIXME: ought to do a better "affected by sleep" check
        if (mons_holiness(mon->type) == MH_NATURAL)
        {
            if (simple_monster_message(mon, " looks drowsy..."))
                beam.obviousEffect = true;

            mon->behavior = BEH_SLEEP;
            return MON_AFFECTED;
        }
        else
            return MON_UNAFFECTED;
    }

    if (beam.flavour == BEAM_BACKLIGHT)
    {
        if (backlight_monsters(mon->x, mon->y, beam.hit, 0))
        {
            beam.obviousEffect = true;
            return MON_AFFECTED;
        }
        return MON_UNAFFECTED;
    }

    // everything else?
    return mons_ench_f2(mon, beam);

deathCheck:

    int thrower = KILL_YOU_MISSILE;
    if (MON_KILL(beam.thrower))
        thrower = KILL_MON_MISSILE;

    if (mon->hit_points < 1)
        monster_die(mon, thrower, beam.beam_source);
    else
    {
        print_wounds(mon);

        if (mons_charclass(mon->type) == MONS_GOLD_MIMIC)
            mimic_alert(mon);
    }

    return MON_AFFECTED;
}


// extra range used on hit
static int  range_used_on_hit(struct bolt &beam)
{
    // non-beams can only affect one thing (player/monster)
    if (!beam.isBeam)
        return BEAM_STOP;

    // CHECK ENCHANTMENTS
    if (beam.beam_name[0] == '0')
    {
        switch(beam.colour)
        {
        case BCOL_SLOW:
        case BCOL_HASTE:
        case BCOL_HEALING:
        case BCOL_PARALYSIS:
        case BCOL_CONFUSION:
        case BCOL_INVISIBILITY:
        case BCOL_TELEPORT:
        case BCOL_POLYMORPH:
        case BCOL_CHARM:
        case BCOL_BANISH:
        case BCOL_PAIN:
        case BCOL_DISINTEGRATION:
        case BCOL_DEGENERATE:
        case BCOL_DISPEL_UNDEAD:
        case BCOL_ENSLAVE_UNDEAD:
        case BCOL_ENSLAVE_DEMON:
            return BEAM_STOP;
        default:
            break;
        }

        switch(beam.flavour)
        {
        case BEAM_SLEEP:
        case BEAM_BACKLIGHT:
            return BEAM_STOP;
        default:
            break;
        }
        return 0;
    }

    // hellfire stops for nobody!
    if (strcmp(beam.beam_name, "hellfire") == 0)
        return 0;

    // generic explosion
    if (beam.flavour == BEAM_EXPLOSION)
        return BEAM_STOP;

    // stinking cloud, plant spit
    if (beam.flavour == BEAM_ACID)
        return BEAM_STOP;

    // lava doesn't go far, but it goes through most stuff
    if (beam.flavour == BEAM_LAVA)
        return 1;

    // If it isn't lightning, reduce range by a lot
    if (beam.flavour != BEAM_ELECTRICITY)
        return random2(4) + 2;

    return 0;
}

/*
   Takes a bolt struct and refines it for use in the explosion function. Called
   from missile() and beam() in beam.cc. Explosions which do not follow from
   beams (eg scrolls of immolation) bypass this function.
 */
static void explosion1(struct bolt &pbolt)
{
    int ex_size = 1;
    // convenience
    int x = pbolt.target_x;
    int y = pbolt.target_y;
    char *seeMsg = NULL;
    char *hearMsg = NULL;

    // assume that the player can see/hear the explosion, or
    // gets burned by it anyway.  :)
    pbolt.msgGenerated = true;

    if (stricmp(pbolt.beam_name, "hellfire") == 0)
    {
        seeMsg = "The hellfire explodes!";
        hearMsg = "You hear a strangely unpleasant explosion.";

        pbolt.type = SYM_BURST;
        pbolt.flavour = BEAM_HELLFIRE;
    }

    if (stricmp(pbolt.beam_name, "golden flame") == 0)
    {
        seeMsg = "The flame explodes!";
        hearMsg = "You hear a strange explosion.";

        pbolt.type = SYM_BURST;
        pbolt.flavour = BEAM_HOLY;     // same as golden flame? [dlb]
    }

    if (stricmp(pbolt.beam_name, "fireball") == 0)
    {
        seeMsg = "The fireball explodes!";
        hearMsg = "You hear an explosion.";

        pbolt.type = SYM_BURST;
        pbolt.flavour = BEAM_FIRE;
        ex_size = 1;
    }

    if (stricmp(pbolt.beam_name, "orb of electricity") == 0)
    {
        seeMsg = "The orb of electricity explodes!";
        hearMsg = "You hear a clap of thunder!";

        pbolt.type = SYM_BURST;
        pbolt.flavour = BEAM_ELECTRICITY;
        pbolt.colour = LIGHTCYAN;
        ex_size = 2;
    }

    if (stricmp(pbolt.beam_name, "orb of energy") == 0)
    {
        seeMsg = "The orb of energy explodes.";
        hearMsg = "You hear an explosion.";
    }

    if (stricmp(pbolt.beam_name, "metal orb") == 0)
    {
        seeMsg = "The orb explodes into a blast of deadly shrapnel!";
        hearMsg = "You hear an explosion!";

        strcpy(pbolt.beam_name, "blast of shrapnel");
        pbolt.type = SYM_ZAP;
        pbolt.flavour = BEAM_FRAG;     // sets it from pure damage to shrapnel (which is absorbed extra by armour)
    }

    if (stricmp(pbolt.beam_name, "great blast of cold") == 0)
    {
        seeMsg = "The blast explodes into a great storm of ice!";
        hearMsg = "You hear a raging storm!";

        strcpy(pbolt.beam_name, "ice storm");
        pbolt.type = SYM_ZAP;
        pbolt.colour = WHITE;
        ex_size = 2 + pbolt.ench_power / 150;
    }

    if (stricmp(pbolt.beam_name, "ball of vapour") == 0)
    {
        seeMsg = "The ball expands into a vile cloud!";
        hearMsg = "You hear a gentle \'poof\'.";
        strcpy(pbolt.beam_name, "stinking cloud");
    }

    if (seeMsg == NULL)
    {
        seeMsg = "The beam explodes into a cloud of software bugs!";
        hearMsg = "You hear the sound of one hand clapping!";
    }


    if (!pbolt.isTracer)
    {
        // check for see/hear/no msg
        if (see_grid(x,y) || (x == you.x_pos && y == you.y_pos))
            mpr(seeMsg);
        else
        {
            if (!(silenced(x,y) || silenced(you.x_pos, you.y_pos)))
                mpr(hearMsg);
            else
                pbolt.msgGenerated = false;
        }
    }

    pbolt.ex_size = ex_size;
    explosion( pbolt );
}                               // end explosion1()


#define MAX_EXPLOSION_RADIUS 9

// explosion is considered to emanate from beam->target_x, target_y
// and has a radius equal to ex_size.  The explosion will respect
// boundaries like walls,  but go through/around statues/idols/etc.

// for each cell affected by the explosion, affect() is called.

void explosion(struct bolt &beam)
{
    int r = beam.ex_size;

    // beam is now an explosion;  set isExplosion.
    beam.isExplosion = true;

    // for now, we don't support explosions greater than 9 radius
    if (r > MAX_EXPLOSION_RADIUS)
        r = MAX_EXPLOSION_RADIUS;

    // make a noise
    noisy(10 + 5*r, beam.target_x, beam.target_y);

    // set map to false
    for (int i=0; i<19; i++)
    {
        for (int j=0; j<19; j++)
            explode_map[i][j] = false;
    }

    // discover affected cells - recursion is your friend!
    // this is done to model an explosion's behavior around
    // corners where a simple 'line of sight' isn't quite
    // enough.   This might be slow for really big explosions,
    // as the recursion runs approximately as R^2
    explosion_map(beam, 0, 0, 0, 0, r);

    // go through affected cells,  drawing effect and
    // calling affect() and affect_items() for each.
    // now, we get a bit fancy,  drawing all radius 0
    // effects, then radius 1, radius 2, etc.  It looks
    // a bit better that way.

    // turn buffering off
#ifdef WIN32CONSOLE
    bool oldValue;
    if (!beam.isTracer)
        oldValue = setBuffering(false);
#endif

    // --------------------- begin boom ---------------

    bool drawing = true;
    for(int i=0; i<2; i++)
    {
        // do center
        explosion_cell(beam, 0, 0, drawing);

        // do the rest of it
        for(int rad = 1; rad <= r; rad ++)
        {
            // do sides
            for (int ay = 1 - rad; ay <= rad - 1; ay += 1)
            {
                if (explode_map[-rad+9][ay+9])
                    explosion_cell(beam, -rad, ay, drawing);

                if (explode_map[rad+9][ay+9])
                    explosion_cell(beam, rad, ay, drawing);
            }

            // do top & bottom
            for (int ax = -rad; ax <= rad; ax += 1)
            {
                if (explode_map[ax+9][-rad+9])
                    explosion_cell(beam, ax, -rad, drawing);

                if (explode_map[ax+9][rad+9])
                    explosion_cell(beam, ax, rad, drawing);
            }

            // new-- delay after every 'ring' {gdl}
#ifdef LINUX
            // If we don't refresh curses we won't
            // guarantee that the explosion is visible
            if (drawing)
                update_screen();
#endif
            // only delay on real explosion
            if (!beam.isTracer && drawing)
                delay(50);
        }

        drawing = false;
    }

    // ---------------- end boom --------------------------

#ifdef WIN32CONSOLE
    if (!beam.isTracer)
        setBuffering(oldValue);
#endif

    // duplicate old behavior - pause after entire explosion
    // has been drawn.
    if (!beam.isTracer)
        more();
}

static void explosion_cell(struct bolt &beam, int x, int y, bool drawOnly)
{
    bool random_beam = false;
    int realx = beam.target_x + x;
    int realy = beam.target_y + y;

    if (!drawOnly)
    {
        // random beams: randomize before affect
        if (beam.flavour == BEAM_RANDOM)
        {
            random_beam = true;
            beam.flavour = BEAM_FIRE + random2(7);
        }

        affect(beam, realx, realy);

        if (random_beam)
            beam.flavour = BEAM_RANDOM;
    }

    // early out for tracer
    if (beam.isTracer)
        return;

    // now affect items
    if (!drawOnly)
        affect_items(beam, realx, realy);

    if (drawOnly)
    {
        int drawx = realx - you.x_pos + 18;
        int drawy = realy - you.y_pos + 9;

        if (see_grid(realx, realy) || (realx == you.x_pos && realy == you.y_pos))
        {
            // bounds check
            if (drawx > 8 && drawx < 26 && drawy > 0 && drawy < 18)
            {
                if (beam.colour == BLACK)
                    textcolor(random_colour());
                else
                    textcolor(beam.colour);

                gotoxy(drawx, drawy);
                putch('#');
            }
        }
    }
}

static void explosion_map( struct bolt &beam, int x, int y,
                           int count, int dir, int r )
{
    // 1. check to see out of range
    if (x * x + y * y > r * r + r)
        return;

    // 2. check count
    if (count > 10*r)
        return;

    // 3. check to see if we're blocked by something
    //    specifically,  we're blocked by WALLS.  Not
    //    statues, idols, etc.
    int dngn_feat = grd[beam.target_x + x][beam.target_y + y];

    // special case: explosion originates from rock/statue
    // (e.g. Lee's rapid deconstruction) - in this case, ignore
    // solid cells at the center of the explosion.
    if (dngn_feat < DNGN_GREEN_CRYSTAL_WALL || dngn_feat == DNGN_WAX_WALL)
    {
        if (!(x==0 && y==0))
            return;
    }

    // hmm, I think we're ok
    explode_map[x+9][y+9] = true;

    // now recurse in every direction except the one we
    // came from
    for(int i=0; i<4; i++)
    {
        if (i+1 != dir)
        {
            int cadd = 5;
            if (x * spreadx[i] < 0 || y * spready[i] < 0)
                cadd = 17;

            explosion_map( beam, x + spreadx[i], y + spready[i],
                           count + cadd, opdir[i], r );
        }
    }
}

// returns true if the beam is harmful (ignoring monster
// resists) -- mon is given for 'special' cases where,
// for example,  "Heal" might actually hurt undead, or
// "Holy Word" being ignored by holy monsters,  etc.
//
// only enchantments should need the actual monster type
// to determine this;  non-enchantments are pretty
// straightforward.
bool nasty_beam(struct monsters *mon, struct bolt &beam)
{
    // take care of non-enchantments
    if (beam.beam_name[0] != '0')
        return true;

    // now for some non-hurtful enchantments

    // degeneration / sleep
    if (beam.colour == BCOL_DEGENERATE || beam.flavour == BEAM_SLEEP)
        return (mons_holiness(mon->type) == MH_NATURAL);

    // dispel undead / control undead
    if (beam.colour == BCOL_DISPEL_UNDEAD || beam.colour == BCOL_ENSLAVE_UNDEAD)
        return (mons_holiness(mon->type) == MH_UNDEAD);

    // pain/agony
    if (beam.colour == BCOL_PAIN)
    {
        return !(mons_holiness(mon->type) == MH_UNDEAD
                    || mons_holiness(mon->type) == MH_DEMONIC);
    }

    // control demon
    if (beam.colour == BCOL_ENSLAVE_DEMON)
        return (mons_holiness(mon->type) == MH_DEMONIC);

    // haste
    if (beam.colour == BCOL_HASTE && beam.flavour != BEAM_BACKLIGHT)
        return false;

    // healing
    if (beam.colour == BCOL_HEALING || beam.colour == BCOL_INVISIBILITY)
        return false;

    // everything else is considered nasty by everyone
    return true;
}
