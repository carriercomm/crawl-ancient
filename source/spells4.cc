/*
 *  File:       spells4.cc
 *  Summary:    new spells, focusing on transmigration, divination and
 *              other neglected areas of Crawl magic ;^)
 *  Written by: Copyleft Josh Fishman 1999-2000, All Rights Preserved
 *
 *  Change History (most recent first):
 *
 *   <2> 29jul2000  jdj  Made a zillion functions static.
 *   <1> 06jan2000  jmf  Created
 */

#include "AppHdr.h"

#include <string>
#include <stdio.h>

#include "externs.h"

#include "abyss.h"
#include "beam.h"
#include "cloud.h"
#include "debug.h"
#include "delay.h"
#include "describe.h"
#include "direct.h"
#include "dungeon.h"
#include "effects.h"
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
#include "randart.h"
#include "religion.h"
#include "spells1.h"
#include "spells4.h"
#include "spl-cast.h"
#include "spl-util.h"
#include "stuff.h"
#include "view.h"

enum DEBRIS                 // jmf: add for shatter, dig, and Giants to throw
{
    DEBRIS_METAL,           //    0
    DEBRIS_ROCK,
    DEBRIS_STONE,
    DEBRIS_WOOD,
    DEBRIS_CRYSTAL,
    NUM_DEBRIS
};          // jmf: ...and I'll actually implement the items Real Soon Now...

static bool mons_can_host_shuggoth(int type);
static int make_a_random_cloud(int x, int y, int pow, int ctype);
static int make_a_rot_cloud(int x, int y, int pow, int ctype);
static int quadrant_blink(int x, int y, int pow, int garbage);

//void cast_animate_golem(int pow); // see actual function for reasoning {dlb}
//void cast_detect_magic(int pow);  //jmf: as above...
//void cast_eringyas_surprising_bouquet(int powc);
void do_monster_rot(int mon);

//jmf: FIXME: put somewhere else (misc.cc?)
// A feeble attempt at Nethack-like completeness for cute messages.
const char *your_hand(int plural)
{
    static char buffer[80];

    switch (you.attribute[ATTR_TRANSFORMATION])
    {
    default:
        mpr("ERROR: unknown transformation in your_hand() (spells4.cc)");
    case TRAN_NONE:
    case TRAN_STATUE:
        if (you.species == SP_TROLL || you.species == SP_GHOUL)
        {
            strcpy(buffer, "claw");
            break;
        }
        // or fall-through
    case TRAN_ICE_BEAST:
    case TRAN_LICH:
        strcpy(buffer, "hand");
        break;
    case TRAN_SPIDER:
        strcpy(buffer, "front leg");
        break;
    case TRAN_SERPENT_OF_HELL:
    case TRAN_DRAGON:
        strcpy(buffer, "foreclaw");
        break;
    case TRAN_BLADE_HANDS:
        strcpy(buffer, "scythe-like blade");
        break;
    case TRAN_AIR:
        strcpy(buffer, "misty tendril");
        break;
    }

    if (plural)
        strcat(buffer, "s");

    return buffer;
}

// I need to make some debris for metal, crystal and stone.
// They could go in OBJ_MISSILES, but I think I'd rather move
// MI_LARGE_ROCK into OBJ_DEBRIS and code giants to throw any
// OBJ_DEBRIS they get their meaty mits on.
static void place_debris(int x, int y, int debris_type)
{
#ifdef USE_DEBRIS_CODE
    switch (debris_type)
    {
    // hate to say this, but the first parameter only allows specific quantity
    // for *food* and nothing else -- and I would hate to see that parameter
    // (force_unique) abused any more than it already has been ... {dlb}:
    case DEBRIS_STONE:
        large = items( random2(3), OBJ_MISSILES, MI_LARGE_ROCK, true, 1, 250 );
        small = items( 3 + random2(6) + random2(6) + random2(6),
                       OBJ_MISSILES, MI_STONE, true, 1, 250 );
        break;
    case DEBRIS_METAL:
    case DEBRIS_WOOD:
    case DEBRIS_CRYSTAL:
        break;
    }

    if (small != NON_ITEM)
        move_item_to_grid( &small, x, y );

    if (large != NON_ITEM)
        move_item_to_grid( &large, x, y );

#else
    return;
#endif
}                               // end place_debris()

// just to avoid typing this over and over
inline void player_hurt_monster(int monster, int damage)
{
    ASSERT( monster != NON_MONSTER );

    if (damage > 0)
    {
        hurt_monster( &menv[monster], damage );

        if (menv[monster].hit_points < 1)
            monster_die( &menv[monster], KILL_YOU, 0 );
        else
            print_wounds( &menv[monster] );
    }

    return;
}                               // end player_hurt_monster()


// Here begin the actual spells:
static int shatter_monsters(int x, int y, int pow, int garbage)
{
    int damage, monster = mgrd[x][y];

    if (monster == NON_MONSTER)
        return 0;

    // Lowered this -- it was much too powerful with the multipliers
    // applied below. It's still capable of a 75 base (150 when doubled),
    // although it's much more likely to be around 52/104 given the
    // distribution... that's extremely good when you consider the
    // area of effect involved (radius 4). -- bwr
    damage = 15 + random2avg( (pow / 5), 4 );

    // Removed a lot of silly monsters down here... people, just because
    // it says ice, rock, or iron in the name doesn't mean it's actually
    // made out of the substance. -- bwr
    switch (menv[monster].type)
    {
    case MONS_ICE_BEAST:        // 3/2 damage
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
    case MONS_DANCING_WEAPON:
    // case MONS_MOLTEN_GARGOYLE:    // fairly fluid -- should absorb damage
    // case MONS_QUICKSILVER_DRAGON: // quicksilver is liquid mercury
        damage *= 15;
        damage /= 10;
        break;

    case MONS_SKELETON_SMALL: // double damage
    case MONS_SKELETON_LARGE:
    case MONS_CURSE_SKULL:
    case MONS_CLAY_GOLEM:
    case MONS_STONE_GOLEM:
    case MONS_IRON_GOLEM:
    case MONS_CRYSTAL_GOLEM:
    case MONS_EARTH_ELEMENTAL:
    case MONS_GARGOYLE:
    case MONS_SKELETAL_DRAGON:
    case MONS_SKELETAL_WARRIOR:
        damage *= 2;
        break;

    case MONS_VAPOUR:
    case MONS_INSUBSTANTIAL_WISP:
    case MONS_AIR_ELEMENTAL:
    case MONS_FIRE_ELEMENTAL:
    case MONS_WATER_ELEMENTAL:
    case MONS_SPECTRAL_WARRIOR:
    case MONS_FREEZING_WRAITH:
    case MONS_WRAITH:
    case MONS_PHANTOM:
    case MONS_PLAYER_GHOST:
    case MONS_SHADOW:
    case MONS_HUNGRY_GHOST:
    case MONS_FLAYED_GHOST:
    case MONS_SMOKE_DEMON:      //jmf: I hate these bastards...
        damage = 0;
        break;

    case MONS_PULSATING_LUMP:
    case MONS_JELLY:
    case MONS_SLIME_CREATURE:
    case MONS_BROWN_OOZE:
    case MONS_AZURE_JELLY:
    case MONS_DEATH_OOZE:
    case MONS_ACID_BLOB:
    case MONS_ROYAL_JELLY:
    case MONS_OOZE:
    case MONS_SPECTRAL_THING:
    case MONS_JELLYFISH:
        damage = 1 + (damage / 4);
        break;

    default:                    // normal damage
        break;
    }

    //FIXME: resist maybe? maybe not...
    player_hurt_monster(monster, random2avg( damage, 2 ));
    return damage;
}                               // end shatter_monsters()

static int shatter_items(int x, int y, int pow, int garbage)
{
    int broke_stuff = 0, next, obj = igrd[x][y];

    if (obj == NON_ITEM)
        return 0;

    while (obj != NON_ITEM)
    {
        next = mitm[obj].link;

        switch (mitm[obj].base_type)
        {
        case OBJ_POTIONS:
            if (!one_chance_in(10))
            {
                broke_stuff++;
                destroy_item(obj);
            }
            break;

        default:
            break;
        }

        obj = next;
    }

    if (broke_stuff)
    {
        if (!silenced(x, y) && !silenced(you.x_pos, you.y_pos))
            mpr("You hear glass break.");

        return 1;
    }

    return 0;
}                               // end shatter_items()

static int shatter_walls(int x, int y, int pow, int garbage)
{
    int chance = pow / 4;  // 75% at power == 300
    int stuff = 0;

    switch (grd[x][y])
    {
    case DNGN_SECRET_DOOR:
        if (see_grid(x, y))
            mpr("A secret door shatters!");
        grd[x][y] = DNGN_FLOOR;
        stuff = DEBRIS_WOOD;
        break;

    case DNGN_CLOSED_DOOR:
        if (see_grid(x, y))
            mpr("A door shatters!");
        grd[x][y] = DNGN_FLOOR;
        stuff = DEBRIS_WOOD;
        break;

    case DNGN_METAL_WALL:
        stuff = DEBRIS_METAL;
        chance /= 4;
        break;

    case DNGN_GRANITE_STATUE:
    case DNGN_STONE_WALL:
        chance /= 2;
        // fall-through
    case DNGN_ORCISH_IDOL:
        stuff = DEBRIS_STONE;
        break;

    case DNGN_ROCK_WALL:
        stuff = DEBRIS_ROCK;
        break;

    case DNGN_GREEN_CRYSTAL_WALL:
        stuff = DEBRIS_CRYSTAL;
        break;

    default:
        break;
    }

    if (stuff && random2(100) < chance)
    {
        if (!silenced(x, y) && !silenced(you.x_pos, you.y_pos))
            mpr("Ka-crash!");

        grd[x][y] = DNGN_FLOOR;
        place_debris(x, y, stuff);
        return 1;
    }

    return 0;
}                               // end shatter_walls()

void cast_shatter(int pow)
{
    int damage = 0;

    // level 9 spell == 300 cap.
    if (pow > 300)
        pow = 300;

    strcpy(info, "The dungeon ");
    strcat(info, (silenced(you.x_pos, you.y_pos)) ? "shakes!" : "rumbles!");
    mpr(info);

    if (!silenced(you.x_pos, you.y_pos))
        noisy(you.x_pos, you.y_pos, 30);

    switch (you.attribute[ATTR_TRANSFORMATION])
    {
    case TRAN_NONE:
    case TRAN_SPIDER:
    case TRAN_LICH:
    case TRAN_DRAGON:
    case TRAN_AIR:
    case TRAN_SERPENT_OF_HELL:
        break;

    case TRAN_STATUE:           // full damage
        damage = 15 + random2avg( (pow / 5), 4 );
        break;

    case TRAN_ICE_BEAST:        // 1/2 damage
        damage = 10 + random2avg( (pow / 5), 4 ) / 2;
        break;

    case TRAN_BLADE_HANDS:      // 2d3 damage
        mpr("Your scythe-like blades vibrate painfully!");
        damage = 2 + random2avg(5, 2);
        break;

    default:
        mpr("cast_shatter(): unknown transformation in spells4.cc");
    }

    if (damage)
        ouch(damage, 0, KILLED_BY_TARGETTING);

    int rad = 3 + (you.skills[SK_EARTH_MAGIC]/9);

    apply_area_within_radius(shatter_items, you.x_pos, you.y_pos, pow, rad, 0);
    apply_area_within_radius(shatter_monsters, you.x_pos, you.y_pos, pow, rad, 0);
    apply_area_within_radius(shatter_walls, you.x_pos, you.y_pos, pow, rad, 0);
}                               // end cast_shatter()

// cast_forescry: raises evasion (by 8 currently) via divination
void cast_forescry(int pow)
{
    if (!you.duration[DUR_FORESCRY])
        mpr("You begin to receive glimpses of the immediate future...");

    you.duration[DUR_FORESCRY] += 5 + random2(pow);

    if (you.duration[DUR_FORESCRY] > 50)
        you.duration[DUR_FORESCRY] = 50;

    you.redraw_evasion = 1;
}                               // end cast_forescry()

void cast_see_invisible(int pow)
{
    if (player_see_invis() || !you.duration[DUR_SEE_INVISIBLE])
        mpr("Nothing seems to happen.");
    else
        mpr("Your vision seems to sharpen.");

    // no message if you already are under the spell
    you.duration[DUR_SEE_INVISIBLE] += 10 + random2(2 + (pow / 2));

    if (you.duration[DUR_SEE_INVISIBLE] > 100)
        you.duration[DUR_SEE_INVISIBLE] = 100;
}                               // end cast_see_invisible()

#if 0
// FIXME: This would be kinda cool if implemented right.
//        The idea is that, like detect_secret_doors, the spell gathers all
//        sorts of information about a thing and then tells the caster a few
//        cryptic hints. So for a (+3,+5) Mace of Flaming, one might detect
//        "enchantment and heat", but for a cursed ring of hunger, one might
//        detect "enchantment and ice" (since it gives you a 'deathly cold'
//        feeling when you put it on) or "necromancy" (since it's evil).
//        A weapon of Divine Wrath and a randart that makes you angry might
//        both give similar messages. The key would be to not tell more than
//        hints about whether an item is benign or cursed, but give info
//        on how strong its enchantment is (and therefore how valuable it
//        probably is).
static void cast_detect_magic(int pow)
{
    struct dist bmove;
    int x, y;
    int monster = 0, item = 0, next;    //int max;
    FixedVector < int, NUM_SPELL_TYPES > found;
    int strong = 0;             // int curse = 0;

    for (next = 0; next < NUM_SPELL_TYPES; next++)
    {
        found[next] = 0;
    }

    mpr("Which direction?", MSGCH_PROMPT);
    direction(bmove, DIR_DIR);

    if (!bmove.isValid)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        return;
    }

    if (bmove.dx == 0 && bmove.dy == 0)
    {
        mpr("You detect a divination in progress.");
        return;
    }

    x = you.x_pos + bmove.dx;
    y = you.y_pos + bmove.dy;

    monster = mgrd[x][y];
    if (monster == NON_MONSTER)
        goto do_items;
    else
        goto all_done;

  do_items:
    item = igrd[x][y];

    if (item == NON_ITEM)
        goto all_done;

    while (item != NON_ITEM)
    {
        next = mitm[item].link;
        if (is_dumpable_artifact
            (mitm[item].base_type, mitm[item].sub_type, mitm[item].plus,
             mitm[item].plus2, mitm[item].special, 0, 0))
        {
            strong++;
            //FIXME: do checks for randart properties
        }
        else
        {
            switch (mitm[item].base_type)
            {
            case OBJ_WEAPONS:
                found[SPTYP_ENCHANTMENT] += (mitm[item].plus > 50);
                found[SPTYP_ENCHANTMENT] += (mitm[item].plus2 > 50);
                break;

            case OBJ_MISSILES:
                found[SPTYP_ENCHANTMENT] += (mitm[item].plus > 50);
                found[SPTYP_ENCHANTMENT] += (mitm[item].plus2 > 50);
                break;

            case OBJ_ARMOUR:
                found[SPTYP_ENCHANTMENT] += mitm[item].plus;
            }
        }
    }

  all_done:
    if (monster)
    {
        mpr("You detect a morphogenic field, such as a monster might have.");
    }
    if (strong)
    {
        mpr("You detect very strong enchantments.");
        return;
    }
    else
    {
        //FIXME:
    }
    return;
}
#endif

// The description idea was okay, but this spell just isn't that exciting.
// So I'm converting it to the more practical expose secret doors. -- bwr
void cast_detect_secret_doors(int pow)
{
    int found = 0;

    for (int x = you.x_pos - 8; x <= you.x_pos + 8; x++)
    {
        for (int y = you.y_pos - 8; y <= you.y_pos + 8; y++)
        {
            if (x < 5 || x > GXM - 5 || y < 5 || y > GYM - 5)
                continue;

            if (!see_grid(x, y))
                continue;

            if (grd[x][y] == DNGN_SECRET_DOOR && random2(pow) > random2(15))
            {
                grd[x][y] = DNGN_CLOSED_DOOR;
                found++;
            }
        }
    }

    if (found)
    {
        redraw_screen();

        snprintf( info, INFO_SIZE, "You detect %s sercet door%s.",
                 (found > 1) ? "some" : "a", (found > 1) ? "s" : "" );
        mpr( info );
    }
}                               // end cast_detect_secret_doors()

void cast_summon_butterflies(int pow)
{
    // explicitly limiting the number
    int num = 1 + random2(3) + random2( pow ) / 10;
    if (num > 12)
        num = 12;

    for (int scount = 1; scount < num; scount++)
    {
        create_monster( MONS_BUTTERFLY, ENCH_ABJ_III, BEH_FRIENDLY,
                        you.x_pos, you.y_pos, MHITNOT, 250 );
    }
}

void cast_summon_large_mammal(int pow)
{
    int mon;
    int temp_rand = random2(pow);

    if (temp_rand < 10)
        mon = MONS_JACKAL;
    else if (temp_rand < 15)
        mon = MONS_HOUND;
    else
    {
        switch (temp_rand % 7)
        {
        case 0:
            if (you.species == SP_HILL_ORC && one_chance_in(3))
                mon = MONS_WARG;
            else
                mon = MONS_WOLF;
            break;
        case 1:
        case 2:
            mon = MONS_WAR_DOG;
            break;
        case 3:
        case 4:
            mon = MONS_HOUND;
            break;
        default:
            mon = MONS_JACKAL;
            break;
        }
    }

    create_monster( mon, ENCH_ABJ_III, BEH_FRIENDLY, you.x_pos,
                    you.y_pos, MHITNOT, 250 );
}

void cast_sticks_to_snakes(int pow)
{
    int mon, i, how_many = 0, max, behaviour;
    bool nice = true;           // delete all ammo?

    // Toned this down... players should have to earn the second snake.
    // This is nicer is that it allows for a more gradual progression
    // (ie. 7th level gives a greater chance of a second snake over 6th...
    // the previous one did the division before calling random2() so
    // the change was only every fifth level) -- bwr
    max = 1 + random2( 1 + you.skills[SK_TRANSMIGRATION] ) / 5;

    if (max > 6)
        max = 6;

    const int weapon = you.equip[EQ_WEAPON];
    // how_many = you.inv[you.equip[EQ_WEAPON]].quantity;
    if (weapon == -1)
    {
        snprintf( info, INFO_SIZE, "Your %s feel slithery!", your_hand(1));
        mpr(info);
        return;
    }

    behaviour = item_cursed( you.inv[ weapon ] ) ? BEH_HOSTILE
                                                    : BEH_FRIENDLY;

    if ((you.inv[ weapon ].base_type == OBJ_MISSILES
         && (you.inv[ weapon ].sub_type == MI_ARROW)))
    {
        if (you.inv[ weapon ].quantity < max)
            max = you.inv[ weapon ].quantity;

        for (i = 0; i <= max; i++)
        {
            //jmf: perhaps also check for poison ammo?
            if (pow > 40 && one_chance_in(3))
                mon = MONS_SNAKE;
            else
                mon = MONS_SMALL_SNAKE;

            if (create_monster(mon, ENCH_ABJ_III, behaviour, you.x_pos,
                you.y_pos, MHITNOT, 250) != -1 || !nice)
            {
                how_many++;
            }
        }
    }

    if (you.inv[ weapon ].base_type == OBJ_WEAPONS
        && (you.inv[ weapon ].sub_type == WPN_CLUB
            || you.inv[ weapon ].sub_type == WPN_SPEAR
            || you.inv[ weapon ].sub_type == WPN_QUARTERSTAFF
            || you.inv[ weapon ].sub_type == WPN_SCYTHE
            || you.inv[ weapon ].sub_type == WPN_GIANT_CLUB
            || you.inv[ weapon ].sub_type == WPN_GIANT_SPIKED_CLUB
            || you.inv[ weapon ].sub_type == WPN_BOW
            || you.inv[ weapon ].sub_type == WPN_ANCUS
            || you.inv[ weapon ].sub_type == WPN_HALBERD
            || you.inv[ weapon ].sub_type == WPN_GLAIVE
            || you.inv[ weapon ].sub_type == WPN_BLOWGUN))
    {
        how_many = 1;

        // Upsizing Snakes to Brown Snakes as the base class for using
        // the really big sticks (so bonus applies really only to trolls,
        // ogres, and most importantly ogre magi).  Still it's unlikely
        // any character is strong enough to bother lugging a few of
        // these around.  -- bwr
        if (mass_item( you.inv[ weapon ] ) < 500)
            mon = MONS_SNAKE;
        else
            mon = MONS_BROWN_SNAKE;

        if (pow > 80 && one_chance_in(3))
            mon = MONS_BLACK_SNAKE;

        if (pow > 40 && one_chance_in(3))
            mon = MONS_YELLOW_SNAKE;

        if (pow > 20 && one_chance_in(3))
            mon = MONS_BROWN_SNAKE;

        create_monster(mon, ENCH_ABJ_III, behaviour, you.x_pos,
            you.y_pos, MHITNOT, 250);
    }

#ifdef USE_DEBRIS_CODE
    if (you.inv[ weapon ].base_type == OBJ_DEBRIS
        && (you.inv[ weapon ].sub_type == DEBRIS_WOOD))
    {
        // this is how you get multiple big snakes
        how_many = 1;
        mpr("FIXME: implement OBJ_DEBRIS conversion! (spells4.cc)");
    }
#endif // USE_DEBRIS_CODE

    if (how_many > you.inv[you.equip[EQ_WEAPON]].quantity)
        how_many = you.inv[you.equip[EQ_WEAPON]].quantity;

    if (how_many)
    {
        dec_inv_item_quantity( you.equip[EQ_WEAPON], how_many );

        snprintf( info, INFO_SIZE, "You create %s snake%s!",
                how_many > 1 ? "some" : "a", how_many > 1 ? "s" : "");
    }
    else
    {
        snprintf( info, INFO_SIZE, "Your %s feel slithery!", your_hand(1));
    }

    mpr(info);
    return;
}                               // end cast_sticks_to_snakes()

void cast_summon_dragon(int pow)
{
    int happy;

    // Removed the chance of multiple dragons... one should be more
    // than enough, and if it isn't, the player can cast again...
    // especially since these aren't on the Abjuration plan... they'll
    // last until they die (maybe that should be changed, but this is
    // a very high level spell so it might be okay).  -- bwr
    happy = random2(pow) > 10;

    if (create_monster(MONS_DRAGON, ENCH_ABJ_III,
                        (happy ? BEH_FRIENDLY : BEH_HOSTILE),
                        you.x_pos, you.y_pos, MHITNOT, 250) != -1)
    {
        strcpy(info, "A dragon appears.");

        if (!happy)
            strcat(info, " It doesn't look very happy.");
    }
    else
        strcpy(info, "Nothing happens.");

    mpr(info);
}                               // end cast_summon_dragon()

void cast_conjure_ball_lightning(int pow)
{
    int num = 3 + random2( 2 + pow / 50 );

    // but restricted so that the situation doesn't get too gross.
    // Each of these will explode for 3d20 damage. -- bwr
    if (num > 8)
        num = 8;

    bool summoned = false;

    for (int i = 0; i < num; i++)
    {
        int tx = -1, ty = -1;

        for (int j = 0; j < 10; j++)
        {
            if (!random_near_space( you.x_pos, you.y_pos, tx, ty, true, true)
                && distance( you.x_pos, you.y_pos, tx, ty ) <= 5)
            {
                break;
            }
        }

        // if we fail, we'll try the ol' summon next to player trick.
        if (tx == -1 || ty == -1)
        {
            tx = you.x_pos;
            ty = you.y_pos;
        }

        int mon = mons_place( MONS_BALL_LIGHTNING, BEH_FRIENDLY, MHITNOT,
                              true, tx, ty );

        // int mon = create_monster( MONS_BALL_LIGHTNING, 0, BEH_FRIENDLY,
        //                           tx, ty, MHITNOT, 250 );

        if (mon != -1)
        {
            mons_add_ench( &menv[mon], ENCH_SHORT_LIVED );
            summoned = true;
        }
    }

    if (summoned)
        mpr( "You create some ball lightning!" );
    else
        canned_msg( MSG_NOTHING_HAPPENS );
}

static int sleep_monsters(int x, int y, int pow, int garbage)
{
    int mnstr = mgrd[x][y];

    if (mnstr == NON_MONSTER)                                   return 0;
    if (mons_holiness(menv[mnstr].type) != MH_NATURAL)          return 0;
    if (check_mons_magres(&menv[mnstr], pow))                   return 0;
    if (mons_friendly(&menv[mnstr]))                            return 0;
    //jmf: now that sleep == hibernation:
    if ((mons_res_cold(menv[mnstr].type) > 0) && coinflip())    return 0;
    if (mons_has_ench(&menv[mnstr], ENCH_SLEEP_WARY))           return 0;

    if (mons_flag(menv[mnstr].type, M_COLD_BLOOD))
        mons_add_ench(&menv[mnstr], ENCH_SLOW);

    menv[mnstr].behavior = BEH_SLEEP;
    mons_add_ench(&menv[mnstr], ENCH_SLEEP_WARY);

    return 1;
}                               // end sleep_monsters()

void cast_mass_sleep(int pow)
{
    apply_area_visible(sleep_monsters, pow);
}                               // end cast_mass_sleep()

static int tame_beast_monsters(int x, int y, int pow, int garbage)
{
    int which_mons = mgrd[x][y];
    struct monsters *monster = &menv[which_mons];

    if (which_mons == NON_MONSTER)                             return 0;
    if (mons_holiness(monster->type) != MH_NATURAL)            return 0;
    if (mons_intel_type(monster->type) != I_ANIMAL)            return 0;
    if (mons_friendly(monster))                                return 0;

    // 50% bonus for dogs, add cats if they get implemented
    if (monster->type == MONS_HOUND || monster->type == MONS_WAR_DOG
                 || monster->type == MONS_BLACK_BEAR)
        pow += (pow / 2);

    if (you.species == SP_HILL_ORC && monster->type == MONS_WARG)
        pow += (pow / 2);

    if (check_mons_magres(monster, pow))
        return 0;

    // I'd like to make the monsters affected permanently, but that's
    // pretty powerful. Maybe a small (pow/10) chance of being permanently
    // tamed, large chance of just being enslaved.
    simple_monster_message(monster, " is tamed!");

    if (random2(100) < random2(pow / 10))
        monster->attitude = ATT_FRIENDLY;       // permanent, right?
    else
        mons_add_ench(monster, ENCH_CHARM);

    return 1;
}                               // end tame_beast_monsters()

void cast_tame_beasts(int pow)
{
    apply_area_visible(tame_beast_monsters, pow);
}                               // end cast_tame_beasts()

static int ignite_poison_objects(int x, int y, int pow, int garbage)
{
    int obj = igrd[x][y], next, strength = 0;

    if (obj == NON_ITEM)
        return 0;

    while (obj != NON_ITEM)
    {
        next = mitm[obj].link;
        if (mitm[obj].base_type == OBJ_POTIONS)
        {
            switch (mitm[obj].sub_type)
            {
                // intentional fall-through all the way down
            case POT_STRONG_POISON:
                strength += 20;
            case POT_DEGENERATION:
                strength += 10;
            case POT_POISON:
                strength += 10;
                destroy_item(obj);
            default:
                break;
            }
        }

        // FIXME: impliment burning poisoned ammo
        // else if ( it's ammo that's poisoned) {
        //   strength += number_of_ammo;
        //   destroy_item(ammo);
        //  }
        obj = next;
    }

    if (strength > 0)
    {
        place_cloud( CLOUD_FIRE, x, y, strength + random2(1 + strength / 4)
                                                + random2(1 + strength / 4)
                                                + random2(1 + strength / 4));
    }

    return strength;
}                               // end ignite_poison_objects()

static int ignite_poison_clouds( int x, int y, int pow, int garbage )
{
    bool did_anything = false;

    const int cloud = env.cgrid[x][y];

    if (cloud != EMPTY_CLOUD)
    {
        if (env.cloud[ cloud ].type == CLOUD_STINK
            || env.cloud[ cloud ].type == CLOUD_STINK_MON)
        {
            did_anything = true;
            env.cloud[ cloud ].type = CLOUD_FIRE;

            env.cloud[ cloud ].decay /= 2;

            if (env.cloud[ cloud ].decay < 1)
                env.cloud[ cloud ].decay = 1;
        }
        else if (env.cloud[ cloud ].type == CLOUD_POISON
                 || env.cloud[ cloud ].type == CLOUD_POISON_MON)
        {
            did_anything = true;
            env.cloud[ cloud ].type = CLOUD_FIRE;
        }
    }

    return ((int) did_anything);
}                               // end ignite_poison_clouds()

static int ignite_poison_monsters(int x, int y, int pow, int garbage)
{
    struct bolt beam;

    //FIXME: go through monster inventories for potions + poison weapons
    //FIXME: hurt monsters that have been poisoned
    int monster = mgrd[x][y];

    if (monster == NON_MONSTER)
        return 0;

    int damage = 1 + random2(8) + random2(8) + random2(pow) / 10;

    //FIXME: apply to monsters with poison attack but not poison corpses?
    if (mons_corpse_thingy(menv[monster].type) == CE_POISONOUS)
    {
        beam.flavour = BEAM_FIRE;
        damage = mons_adjust_flavoured(&menv[monster], beam, damage);
        player_hurt_monster(monster, damage);
        return 1;
    }
    return 0;
}

void cast_ignite_poison(int pow)
{
    int damage = 0, strength = 0, pcount = 0, acount = 0, totalstrength = 0;
    char item;
    bool wasWielding = false;

    // another power cap (level 5-7 spell)
    if (pow > 200)
        pow = 200;

    // temp weapon of venom => temp fire brand
    if (you.duration[DUR_WEAPON_BRAND])
    {
        if (you.inv[you.equip[EQ_WEAPON]].special == SPWPN_VENOM)
        {
            in_name(you.equip[EQ_WEAPON], DESC_CAP_YOUR, str_pass);
            strcpy(info, str_pass);
            strcat(info, " bursts into flame!");
            mpr(info);

            you.inv[you.equip[EQ_WEAPON]].special += (SPWPN_FLAMING - SPWPN_VENOM);
            you.wield_change = true;
            you.duration[DUR_WEAPON_BRAND] += 1 + you.duration[DUR_WEAPON_BRAND] / 2;
            if (you.duration[DUR_WEAPON_BRAND] > 80)
                you.duration[DUR_WEAPON_BRAND] = 80;
        }
    }

    totalstrength = 0;

    for (item = 0; item < ENDOFPACK; item++)
    {
        if (!you.inv[item].quantity)
            continue;

        strength = 0;

        if (you.inv[item].base_type == OBJ_MISSILES)
        {
            if (you.inv[item].special == 3)
            {                   // burn poison ammo
                strength = you.inv[item].quantity;
                acount += you.inv[item].quantity;
            }
        }

        if (you.inv[item].base_type == OBJ_POTIONS)
        {
            switch (you.inv[item].sub_type)
            {
            case POT_STRONG_POISON:
                strength += 20 * you.inv[item].quantity;
                break;
            case POT_DEGENERATION:
            case POT_POISON:
                strength += 10 * you.inv[item].quantity;
                break;
            default:
                break;
            } // end switch

            if (strength)
                pcount += you.inv[item].quantity;
        }

        if (strength)
        {
            you.inv[item].quantity = 0;
            if (item == you.equip[EQ_WEAPON])
            {
                you.equip[EQ_WEAPON] = -1;
                wasWielding = true;
            }
        }

        totalstrength += strength;
    }

    if (acount > 0)
        mpr("Some ammo you are carrying burns!");

    if (pcount > 0)
    {
        snprintf( info, INFO_SIZE, "%s potion%s you are carrying explode%s!",
            pcount > 1 ? "Some" : "A",
            pcount > 1 ? "s" : "",
            pcount > 1 ? "" : "s");
        mpr(info);
    }

    if (wasWielding == true)
        canned_msg( MSG_EMPTY_HANDED );

    if (totalstrength)
    {
        place_cloud(CLOUD_FIRE, you.x_pos, you.y_pos,
                    random2(totalstrength / 4 + 1) + random2(totalstrength / 4 + 1) +
                    random2(totalstrength / 4 + 1) + random2(totalstrength / 4 + 1) + 1);
    }

    // player is poisonous
    if (you.mutation[MUT_SPIT_POISON] || you.mutation[MUT_STINGER]
        || you.attribute[ATTR_TRANSFORMATION] == TRAN_SPIDER
        || (!player_is_shapechanged()
            && (you.species == SP_GREEN_DRACONIAN
                || you.species == SP_KOBOLD
                || you.species == SP_NAGA)))
    {
        damage = 1 + random2(8) + random2(8) + random2(pow) / 10;
    }

    // player is poisoned
    for (int i = 0; i < you.poison; i++)
    {
        damage += 1 + random2(6);
    }

    if (damage)
    {
        if (player_res_fire() > 100)
        {
            mpr("You feel like your blood is boiling!");
            damage = damage / 3;
        }
        else if (player_res_fire() < 100)
        {
            damage *= 3;
            mpr("The poison in your system burns terribly!");
        }
        else
            mpr("The poison in your system burns!");

        ouch(damage, 0, KILLED_BY_TARGETTING);
    }

    apply_area_visible(ignite_poison_clouds, pow);
    apply_area_visible(ignite_poison_objects, pow);
    apply_area_visible(ignite_poison_monsters, pow);
}                               // end cast_ignite_poison()

void cast_silence(int pow)
{
    if (!you.attribute[ATTR_WAS_SILENCED])
        mpr("A profound silence engulfs you.");

    you.attribute[ATTR_WAS_SILENCED] = 1;

    you.duration[DUR_SILENCE] += 20 + random2avg( pow, 2 );

    if (you.duration[DUR_SILENCE] > 100)
        you.duration[DUR_SILENCE] = 100;
}                               // end cast_silence()


/* ******************************************************************
// no hooks for this anywhere {dlb}:

void cast_animate_golem(int pow)
{
  // must have more than 20 max_hitpoints

  // must be wielding a Scroll of Paper (for chem)

  // must be standing on a pile of <foo> (for foo in: wood, metal, rock, stone)

  // Will cost you 5-10% of max_hitpoints, or 20 + some, whichever is more
  mpr("You imbue the inanimate form with a portion of your life force.");

  naughty(NAUGHTY_CREATED_LIFE, 10);
}

****************************************************************** */

static int discharge_monsters( int x, int y, int pow, int garbage )
{
    const int mon = mgrd[x][y];
    int damage = 0;

    struct bolt beam;
    beam.flavour = BEAM_ELECTRICITY; // used for mons_adjust_flavoured

    if (x == you.x_pos && y == you.y_pos)
    {
        mpr( "You are struck by lightning." );
        damage = 3 + random2( 5 + pow / 20 );
        damage = check_your_resists( damage, BEAM_ELECTRICITY );
        ouch( damage, 0, KILLED_BY_WILD_MAGIC );
    }
    else if (mon == NON_MONSTER)
        return (0);
    else if (mons_res_elec(menv[mon].type) || mons_flies(menv[mon].type))
        return (0);
    else
    {
        damage = 3 + random2( 5 + pow / 20 );
        damage = mons_adjust_flavoured( &menv[mon], beam, damage );

        if (damage)
        {
            strcpy( info, ptr_monam( &(menv[mon]), DESC_CAP_THE ) );
            strcat( info, " is struck by lightning." );
            mpr( info );

            player_hurt_monster( mon, damage );
        }
    }

    // Recursion to give us chain-lightning -- bwr
    // Low power slight chance added for low power characters -- bwr
    if ((pow >= 20 && !one_chance_in(3)) || (pow >= 3 && one_chance_in(10)))
    {
        mpr( "The lightning arcs!" );
        pow /= (coinflip() ? 2 : 3);
        damage += apply_random_around_square( discharge_monsters, x, y,
                                              true, pow, 1 );
    }
    else if (damage > 0)
    {
        // Only printed if we did damage, so that the messages in
        // cast_discharge() are clean. -- bwr
        mpr( "The lightning grounds out." );
    }

    return (damage);
}                               // end discharge_monsters()

void cast_discharge( int pow )
{
    if (pow > 150)
        pow = 150;

    int num_targs = 1 + random2( 1 + pow / 50 );
    int dam;

    dam = apply_random_around_square( discharge_monsters, you.x_pos, you.y_pos,
                                      true, pow, num_targs );

#if DEBUG_DIAGNOSTICS
    snprintf( info, INFO_SIZE, "Arcs: %d Damage: %d", num_targs, dam );
    mpr( info );
#endif

    if (dam == 0)
    {
        if (coinflip())
            mpr("The air around you crackles with electrical energy.");
        else
        {
            bool plural = coinflip();
            snprintf( info, INFO_SIZE, "%s blue arc%s ground%s harmlessly %s you.",
                plural ? "Some" : "A",
                plural ? "s" : "",
                plural ? " themselves" : "s itself",
                plural ? "around" : (coinflip() ? "beside" :
                                     coinflip() ? "behind" : "before")
                );

            mpr(info);
        }
    }
}                               // end cast_discharge()

// NB: this must be checked against the same effects
// in fight.cc for all forms of attack !!! {dlb}
// This function should be currently unused (the effect is too powerful)
static int distortion_monsters(int x, int y, int pow, int message)
{
    int specdam = 0;
    int monster_attacked = mgrd[x][y];

    if (monster_attacked == NON_MONSTER)
        return 0;

    struct monsters *defender = &menv[monster_attacked];

    if (pow > 100)
        pow = 100;

    if (x == you.x_pos && y == you.y_pos)
    {
        if (you.skills[SK_TRANSLOCATIONS] < random2(8))
            miscast_effect(SPTYP_TRANSLOCATION, pow / 9 + 1, pow, 100);
        else
            miscast_effect(SPTYP_TRANSLOCATION, 1, 1, 100);

        return 1;
    }

    if (defender->type == MONS_BLINK_FROG)      // any others resist?
    {
        int hp = defender->hit_points;
        int max_hp = defender->max_hit_points;

        mpr("The blink frog basks in the translocular energy.");

        if (hp < max_hp)
            hp += 1 + random2(1 + pow / 4) + random2(1 + pow / 7);

        if (hp > max_hp)
            hp = max_hp;

        defender->hit_points = hp;
        return 1;
    }
    else if (coinflip())
    {
        strcpy(info, "Space bends around ");
        strcat(info, ptr_monam(defender, DESC_NOCAP_THE));
        strcat(info, ".");
        mpr(info);
        specdam += 1 + random2avg( 7, 2 ) + random2(pow) / 40;
    }
    else if (coinflip())
    {
        strcpy(info, "Space warps horribly around ");
        strcat(info, ptr_monam( defender, DESC_NOCAP_THE ));
        strcat(info, "!");
        mpr(info);

        specdam += 3 + random2avg( 12, 2 ) + random2(pow) / 25;
    }
    else if (one_chance_in(3))
    {
        monster_blink(defender);
        return 1;
    }
    else if (one_chance_in(3))
    {
        monster_teleport(defender, coinflip());
        return 1;
    }
    else if (one_chance_in(3))
    {
        monster_die(defender, KILL_RESET, 0);
        return 1;
    }
    else if (message)
    {
        mpr("Nothing seems to happen.");
        return 1;
    }

    player_hurt_monster(monster_attacked, specdam);
    return specdam;
}                               // end distortion_monsters()

void cast_bend(int pow)
{
    apply_one_neighbouring_square( distortion_monsters, pow );
}                               // end cast_bend()

// Really this is just applying the best of Band/Warp weapon/Warp field
// into a spell that gives the "make monsters go away" benefit without
// the insane damage potential.  -- bwr
static int disperse_monsters(int x, int y, int pow, int message)
{
    int monster_attacked = mgrd[x][y];

    if (monster_attacked == NON_MONSTER)
        return 0;

    struct monsters *defender = &menv[monster_attacked];

    if (defender->type == MONS_BLINK_FROG)
    {
        simple_monster_message(defender, " resists.");
        return 1;
    }
    else if (check_mons_magres(defender, pow))
    {
        if (coinflip())
        {
            simple_monster_message(defender, " partially resists.");
            monster_blink(defender);
        }
        else
            simple_monster_message(defender, " resists.");

        return 1;
    }
    else
    {
        simple_monster_message(defender, " looks slightly unstable.");
        monster_teleport(defender, false);
        return 1;
    }

    return 0;
}

void cast_dispersal(int pow)
{
    if (apply_area_around_square( disperse_monsters,
                                  you.x_pos, you.y_pos, pow ) == 0)
    {
        mpr("There is a brief shimmering around you.");
    }
}

static int spell_swap_func(int x, int y, int pow, int message)
{
    int monster_attacked = mgrd[x][y];

    if (monster_attacked == NON_MONSTER)
        return 0;

    struct monsters *defender = &menv[monster_attacked];

    if (defender->type == MONS_BLINK_FROG
        || check_mons_magres( defender, pow ))
    {
        simple_monster_message( defender, " resists." );
    }
    else
    {
        // Swap doesn't seem to actually swap, but just sets the
        // monster's location equal to the players... this being because
        // the acr.cc call is going to move the player afterwards (for
        // the regular friendly monster swap).  So we'll go through
        // standard swap procedure here... since we really want to apply
        // the same swap_places function as with friendly monsters...
        // see note over there. -- bwr
        int old_x = defender->x;
        int old_y = defender->y;

        if (swap_places( defender ))
        {
            you.x_pos = old_x;
            you.y_pos = old_y;
        }
    }

    return 1;
}

void cast_swap(int pow)
{
    apply_one_neighbouring_square( spell_swap_func, pow );
}

static int make_a_rot_cloud(int x, int y, int pow, int ctype)
{
    int next = 0, obj = mgrd[x][y];

    if (obj == NON_MONSTER)
        return 0;

    while (obj != NON_ITEM)
    {
        next = mitm[obj].link;

        if (mitm[obj].base_type == OBJ_CORPSES
            && mitm[obj].sub_type == CORPSE_BODY)
        {
            if (!mons_skeleton(mitm[obj].plus))
                destroy_item(obj);
            else
            {
                mitm[obj].sub_type = CORPSE_SKELETON;
                mitm[obj].special = 200;
                mitm[obj].colour = LIGHTGREY;
            }

            place_cloud(ctype, x, y,
                        (3 + random2(pow / 4) + random2(pow / 4) +
                         random2(pow / 4)));
            return 1;
        }

        obj = next;
    }

    return 0;
}                               // end make_a_rot_cloud()

int make_a_normal_cloud(int x, int y, int pow, int ctype)
{
    place_cloud(ctype, x, y,
                (3 + random2(pow / 4) + random2(pow / 4) + random2(pow / 4)));

    return 1;
}                               // end make_a_normal_cloud()

static int make_a_random_cloud(int x, int y, int pow, int ctype)
{

    if (ctype == CLOUD_NONE)
        ctype = CLOUD_BLACK_SMOKE;

    unsigned char cloud_material;

    switch (random2(9))
    {
    case 0:
        cloud_material = CLOUD_FIRE;
        break;
    case 1:
        cloud_material = CLOUD_STINK;
        break;
    case 2:
        cloud_material = CLOUD_COLD;
        break;
    case 3:
        cloud_material = CLOUD_POISON;
        break;
    case 4:
        cloud_material = CLOUD_BLUE_SMOKE;
        break;
    case 5:
        cloud_material = CLOUD_STEAM;
        break;
    case 6:
        cloud_material = CLOUD_PURP_SMOKE;
        break;
    default:
        cloud_material = ctype;
        break;
    }

    // that last bit is equivalent to "random2(pow/4) + random2(pow/4)
    // + random2(pow/4)" {dlb}
    // can you see the pattern? {dlb}
    place_cloud(cloud_material, x, y, 3 + random2avg(3 * (pow / 4) - 2, 3));

    return 1;
}                               // end make_a_random_cloud()

static int passwall(int x, int y, int pow, int garbage)
{
    char dx, dy, nx = x, ny = y;
    int howdeep = 0;
    bool done = false;
    int shallow = 1 + (you.skills[SK_EARTH_MAGIC] / 8);

    if (pow > 150)
        pow = 150;

    // allow statues as entry points?
    if (grd[x][y] != DNGN_ROCK_WALL)
        // Irony: you can start on a secret door but not a door.
        // Worked stone walls are out, they're not diggable and
        // are used for impassable walls... I'm not sure we should
        // even allow statues (should be contiguous rock) -- bwr
    {
        mpr("That's not a passable wall.");
        return 0;
    }

    dx = x - you.x_pos;
    dy = y - you.y_pos;

    while (!done)
    {
        // I'm trying to figure proper borders out {dlb}
        // FIXME: dungeon border?
        if (nx > (GXM - 1) || ny > (GYM - 1) || nx < 2 || ny < 2)
        {
            mpr("You sense an overwhelming volume of rock.");
            return 0;
        }

        switch (grd[nx][ny])
        {
        default:
            done = true;
            break;
        case DNGN_ROCK_WALL:
        case DNGN_ORCISH_IDOL:
        case DNGN_GRANITE_STATUE:
        case DNGN_SECRET_DOOR:
            nx += dx;
            ny += dy;
            howdeep++;
            break;
        }
    }

    int range = shallow + random2(pow) / 25;

    if (howdeep > shallow)
    {
        mpr("This rock feels deep.");

        if (yesno("Try anyway?"))
        {
            if (howdeep > range)
            {
                ouch(1 + you.hp, 0, KILLED_BY_PETRIFICATION);
                //jmf: not return; if wizard, successful transport is option
            }
        }
        else
        {
            if (one_chance_in(30))
                mpr("Wuss.");
            else
                canned_msg(MSG_OK);
            return 1;
        }
    }

    // Note that the delay was (1 + howdeep * 2), but now that the
    // delay is stopped when the player is attacked it can be much
    // shorter since its harder to use for quick escapes. -- bwr
    start_delay( DELAY_PASSWALL, 2 + howdeep, nx, ny );

    return 1;
}                               // end passwall()

void cast_passwall(int pow)
{
    apply_one_neighbouring_square(passwall, pow);
}                               // end cast_passwall()

static int intoxicate_monsters(int x, int y, int pow, int garbage)
{
    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)
        return 0;
    if (mons_intel(menv[mon].type) < I_NORMAL)
        return 0;
    if (mons_holiness(menv[mon].type) != MH_NATURAL)
        return 0;
    if (mons_res_poison(menv[mon].type) > 0)
        return 0;

    mons_add_ench(&menv[mon], ENCH_CONFUSION);
    return 1;
}                               // end intoxicate_monsters()

void cast_intoxicate(int pow)
{
    // Actually, this cap actually helps the player, since pow is
    // only used here for the duration of the players confusion. -- bwr
    if (pow > 150)
        pow = 150;

    potion_effect(POT_CONFUSION, 10 + pow / 10);

    if (one_chance_in(20) && lose_stat(STAT_INTELLIGENCE, 1 + random2(3)))
        mpr("Your head spins!");

    apply_area_visible(intoxicate_monsters, pow);
}                               // end cast_intoxicate()

// intended as a high-level Elven (a)bility
static int glamour_monsters(int x, int y, int pow, int garbage)
{
    int mon = mgrd[x][y];

    // Power in this function is already limited by a function of
    // experience level (10 + level / 2) since it's only an ability,
    // never an actual spell. -- bwr

    if (mon == NON_MONSTER)
        return (0);

    if (one_chance_in(5))
        return (0);

    if (mons_intel(menv[mon].type) < I_NORMAL)
        return (0);

    if (mons_holiness(mon) != MH_NATURAL)
        return (0);

    if (!mons_is_humanoid( menv[mon].type ))
        return (0);

    const char show_char = mons_char( menv[mon].type );

    // gargoyles are immune.
    if (menv[mon].type == MONS_GARGOYLE
        || menv[mon].type == MONS_METAL_GARGOYLE
        || menv[mon].type == MONS_MOLTEN_GARGOYLE)
    {
        return (0);
    }

    // orcs resist thru hatred of elves
    // elves resist cause they're elves
    // boggarts are malevolent highly magical wee-folk
    if (show_char == 'o' || show_char == 'e' || menv[mon].type == MONS_BOGGART)
        pow = (pow / 2) + 1;

    if (check_mons_magres(&menv[mon], pow))
        return (0);

    switch (random2(6))
    {
    case 0:
        mons_add_ench(&menv[mon], ENCH_FEAR);
        break;
    case 1:
    case 4:
        mons_add_ench(&menv[mon], ENCH_CONFUSION);
        break;
    case 2:
    case 5:
        mons_add_ench(&menv[mon], ENCH_CHARM);
        break;
    case 3:
        menv[mon].behavior = BEH_SLEEP;
        break;
    }

    // why no, there's no message as to which effect happened >:^)
    if (!one_chance_in(4))
    {
        strcpy(info, ptr_monam( &(menv[mon]), DESC_CAP_THE));

        switch (random2(4))
        {
        case 0:
            strcat(info, " looks dazed.");
            break;
        case 1:
            strcat(info, " blinks several times.");
            break;
        case 2:
            strcat(info, " rubs its eye");
            if (menv[mon].type != MONS_CYCLOPS)
                strcat(info, "s");
            strcat(info, ".");
            break;
        case 4:
            strcat(info, " tilts its head.");
            break;
        }

        mpr(info);
    }

    return (1);
}                               // end glamour_monsters()

void cast_glamour(int pow)
{
    apply_area_visible(glamour_monsters, pow);
}                               // end cast_glamour()

bool backlight_monsters(int x, int y, int pow, int garbage)
{
    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)
        return false;

    switch (menv[mon].type)
    {
    //case MONS_INSUBSTANTIAL_WISP: //jmf: I'm not sure if these glow or not
    //case MONS_VAPOUR:
    case MONS_UNSEEN_HORROR:    // consider making this visible? probably not.
        return false;

    case MONS_FIRE_VORTEX:
    case MONS_ANGEL:
    case MONS_FIEND:
    case MONS_SHADOW:
    case MONS_EFREET:
    case MONS_HELLION:
    case MONS_GLOWING_SHAPESHIFTER:
    case MONS_FIRE_ELEMENTAL:
    case MONS_AIR_ELEMENTAL:
    case MONS_SHADOW_FIEND:
    case MONS_SPECTRAL_WARRIOR:
    case MONS_ORANGE_RAT:
    case MONS_BALRUG:
    case MONS_SPATIAL_VORTEX:
    case MONS_PIT_FIEND:
    case MONS_SHINING_EYE:
    case MONS_DAEVA:
    case MONS_SPECTRAL_THING:
    case MONS_ORB_OF_FIRE:
    case MONS_EYE_OF_DEVASTATION:
        return false;               // already glowing or invisible
    default:
        break;
    }

    strcpy(info, ptr_monam( &(menv[mon]), DESC_CAP_THE ));
    strcat(info, " is outlined in light.");
    mpr(info);

    // this enchantment wipes out invisibility (neat)
    mons_del_ench( &menv[mon], ENCH_INVIS );
    mons_add_ench( &menv[mon], ENCH_BACKLIGHT_IV );

    return true;
}                               // end backlight_monsters()

void cast_evaporate(int pow)
{
    // level 2 power cap
    if (pow > 100)
        pow = 100;

    if (you.equip[EQ_WEAPON] == -1
        || you.inv[you.equip[EQ_WEAPON]].base_type != OBJ_POTIONS)
    {
        snprintf( info, INFO_SIZE, "Wisps of steam play over your %s!", your_hand(1));
        mpr(info);
        return;
    }

    switch (you.inv[you.equip[EQ_WEAPON]].sub_type)
    {
    case POT_DEGENERATION:
    case POT_STRONG_POISON:
        apply_area_cloud(make_a_normal_cloud, you.x_pos, you.y_pos,
                         3 + pow * 3, 5 + random2avg(9, 2), CLOUD_POISON);
        break;

    case POT_POISON:
        apply_area_cloud(make_a_normal_cloud, you.x_pos, you.y_pos, 5 + pow,
                         3 + random2avg(7, 2), CLOUD_POISON);
        break;

    case POT_DECAY:
        apply_area_cloud(make_a_normal_cloud, you.x_pos, you.y_pos, 2 + pow,
                         6 + random2avg(5, 2), CLOUD_MIASMA);

        if (you.species != SP_MUMMY)
        {
            mpr("You smell decay.");
            if (one_chance_in(4))
                you.rotting += 1 + random2(5);
        }
        break;

    case POT_CONFUSION:
    case POT_SLOWING:
    case POT_PARALYSIS:
        if (you.species != SP_MUMMY)    // mummies can't smell
            mpr("Whew! That stinks!");

        apply_area_cloud(make_a_normal_cloud, you.x_pos, you.y_pos,
                         8 + pow / 3, 3 + random2avg(7, 2), CLOUD_STINK);
        break;

    case POT_MUTATION:
    case POT_MAGIC:
        apply_area_cloud(make_a_random_cloud, you.x_pos, you.y_pos, 5 + pow,
                         5 + random2avg(7, 2), CLOUD_STEAM);
        break;

    case POT_WATER:
        apply_area_cloud(make_a_normal_cloud, you.x_pos, you.y_pos,
                         5 + pow / 2, 3 + random2avg(7, 2), CLOUD_STEAM);
        break;

    default:
        if (coinflip())
        {
            apply_area_cloud(make_a_random_cloud, you.x_pos, you.y_pos,
                             5 + pow, 5 + random2avg(7, 2),
                             CLOUD_BLACK_SMOKE);
        }
        else
        {
            apply_area_cloud(make_a_normal_cloud, you.x_pos, you.y_pos,
                             5 + pow / 2, 3 + random2avg(7, 2), CLOUD_STEAM);
        }
        break;
    }

    dec_inv_item_quantity( you.equip[EQ_WEAPON], 1 );

    return;
}                               // end cast_evaporate()

void make_shuggoth(int x, int y, int hp)
{
    int mon = create_monster( MONS_SHUGGOTH, 100 + random2avg(58, 3),
        BEH_HOSTILE, x, y, MHITNOT, 250 );

    if (mon != -1)
    {
        menv[mon].hit_points = hp;
        menv[mon].max_hit_points = hp;
    }

    return;
}                               // end make_shuggoth()

static int rot_living(int x, int y, int pow, int message)
{
    int mon = mgrd[x][y];
    int ench;

    if (mon == NON_MONSTER)
        return 0;

    if (mons_holiness(menv[mon].type) != MH_NATURAL)
        return 0;

    if (check_mons_magres(&menv[mon], pow))
        return 0;

    ench = ((random2(pow) + random2(pow) + random2(pow) + random2(pow)) / 4);

    if (ench >= 50)
        ench = ENCH_YOUR_ROT_IV;
    else if (ench >= 35)
        ench = ENCH_YOUR_ROT_III;
    else if (ench >= 20)
        ench = ENCH_YOUR_ROT_II;
    else
        ench = ENCH_YOUR_ROT_I;

    mons_add_ench(&menv[mon], ench);

    return 1;
}                               // end rot_living()

static int rot_undead(int x, int y, int pow, int garbage)
{
    int mon = mgrd[x][y];
    int ench;

    if (mon == NON_MONSTER)
        return 0;

    if (mons_holiness(menv[mon].type) != MH_UNDEAD)
        return 0;

    if (check_mons_magres(&menv[mon], pow))
        return 0;

    // this does not make sense -- player mummies are
    // immune to rotting (or have been) -- so what is
    // the schema in use here to determine rotting??? {dlb}

    //jmf: up for discussion. it is clearly unfair to
    //     rot player mummies.
    //     the `shcema' here is: corporeal non-player undead
    //     rot, discorporeal undead don't rot. if you wanna
    //     insist that monsters get the same treatment as
    //     players, I demand my player mummies get to worship
    //     the evil mummy & orc god.
    switch (menv[mon].type)
    {
    case MONS_NECROPHAGE:
    case MONS_ZOMBIE_SMALL:
    case MONS_LICH:
    case MONS_MUMMY:
    case MONS_VAMPIRE:
    case MONS_ZOMBIE_LARGE:
    case MONS_WIGHT:
    case MONS_GHOUL:
    case MONS_BORIS:
    case MONS_ANCIENT_LICH:
    case MONS_VAMPIRE_KNIGHT:
    case MONS_VAMPIRE_MAGE:
    case MONS_GUARDIAN_MUMMY:
    case MONS_GREATER_MUMMY:
    case MONS_MUMMY_PRIEST:
        break;
    case MONS_ROTTING_HULK:
    default:
        return 0;               // immune (no flesh) or already rotting
    }

    ench = ((random2(pow) + random2(pow) + random2(pow) + random2(pow)) / 4);

    if (ench >= 50)
        ench = ENCH_YOUR_ROT_IV;
    else if (ench >= 35)
        ench = ENCH_YOUR_ROT_III;
    else if (ench >= 20)
        ench = ENCH_YOUR_ROT_II;
    else
        ench = ENCH_YOUR_ROT_I;

    mons_add_ench(&menv[mon], ench);

    return 1;
}                               // end rot_undead()

static int rot_corpses(int x, int y, int pow, int garbage)
{
    return make_a_rot_cloud(x, y, pow, CLOUD_MIASMA);
}                               // end rot_corpses()

void cast_rotting(int pow)
{
    if (pow > 150)
        pow = 150;

    apply_area_visible(rot_living, pow);
    apply_area_visible(rot_undead, pow);
    apply_area_visible(rot_corpses, pow);
    return;
}                               // end cast_rotting()

void do_monster_rot(int mon)
{
    int damage = 1 + random2(3);

    if (mons_holiness(menv[mon].type) == MH_UNDEAD && random2(5))
    {
        apply_area_cloud(make_a_normal_cloud, menv[mon].x, menv[mon].y,
                         10, 1, CLOUD_MIASMA);
    }

    player_hurt_monster(mon, damage);
    return;
}                               // end do_monster_rot()

static int snake_charm_monsters(int x, int y, int pow, int message)
{
    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)
        return 0;
    if (mons_friendly(&menv[mon]))
        return 0;
    if (one_chance_in(4))
        return 0;
    if (mons_charclass(menv[mon].type) != 'S')
        return 0;
    if (check_mons_magres(&menv[mon], pow))
        return 0;

    menv[mon].attitude = ATT_FRIENDLY;
    snprintf( info, INFO_SIZE, "%s sways back and forth.", ptr_monam( &(menv[mon]), DESC_CAP_THE ));
    mpr(info);

    return 1;
}

void cast_snake_charm(int pow)
{
    // powc = (you.experience_level * 2) + (you.skills[SK_INVOCATIONS] * 3);
    apply_one_neighbouring_square(snake_charm_monsters, pow);
}

void cast_fragmentation(int pow)        // jmf: ripped idea from airstrike
{
    struct dist beam;
    struct bolt blast;
    int debris = 0;
    int i, hurted;
    bool explode = false;
    char *what = NULL;

    // This is unbelievably powerful (power can get quite large):
    // hurted = (random2(pow) + random2(pow) + random2(pow) + random2(pow)) / 2;
    // hurted += random2(10) + random2(12);
    if (pow > 200)
        pow = 200;

    // This damage will need to be rolled below (done this way because
    // we apply damage directly to monsters here, but blast effects roll
    // their damage elsewhere).
    hurted = 1 + random2avg(20,2) + (pow / 5);

    mpr("Fragment what (e.g. a wall)?", MSGCH_PROMPT);
    direction(beam, DIR_TARGET);

    if (!beam.isValid)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        return;
    }

    //FIXME: if (player typed '>' to attack floor) goto do_terrain;
    blast.thrower = KILL_YOU;
    blast.ex_size = 1;              // default
    blast.type = '#';
    blast.colour = 0;
    blast.target_x = beam.tx;
    blast.target_y = beam.ty;
    blast.isTracer = false;

    i = mgrd[beam.tx][beam.ty];

    if (i != NON_MONSTER)
    {
        //struct monsters *monster = &menv[i];
        strcpy(info, ptr_monam( &(menv[i]), DESC_CAP_THE ));

        switch (menv[i].type)
        {
        case MONS_ICE_BEAST: // blast of ice fragments
        case MONS_SIMULACRUM_SMALL:
        case MONS_SIMULACRUM_LARGE:
            explode = true;
            strcpy(blast.beam_name, "icy blast");
            blast.colour = WHITE;
            blast.damage = (hurted / 2) + 1;
            blast.flavour = BEAM_ICE;
            player_hurt_monster(i, 1 + random2(hurted));
            break;

        case MONS_FLYING_SKULL:
        case MONS_SKELETON_SMALL:
        case MONS_SKELETON_LARGE:       // blast of bone
            explode = true;

            snprintf( info, INFO_SIZE, "The sk%s explodes into sharp fragments of bone!",
                    (menv[i].type == MONS_FLYING_SKULL) ? "ull" : "eleton");

            strcpy(blast.beam_name, "blast of bone shards");

            blast.colour = LIGHTGREY;
            blast.damage = (hurted / 3) + 1;
            blast.flavour = BEAM_FRAG;

            if (random2(50) < (pow / 5))        // potential insta-kill
                monster_die(&menv[i], KILL_YOU, 0);
            else
                player_hurt_monster(i, 1 + random2(hurted));
            goto all_done;      // i.e. no "Foo Explodes!"

        case MONS_WOOD_GOLEM:
            explode = false;
            simple_monster_message(&menv[i], " shudders violently!");
            player_hurt_monster(i, 1 + random2(hurted));
            break;

        case MONS_IRON_GOLEM:
        case MONS_METAL_GARGOYLE:
            explode = true;
            strcpy(blast.beam_name, "blast of metal fragments");
            blast.colour = CYAN;
            blast.damage = hurted;
            blast.flavour = BEAM_FRAG;
            player_hurt_monster(i, 1 + random2(hurted));
            break;

        case MONS_CLAY_GOLEM:   // assume baked clay and not wet loam
        case MONS_STONE_GOLEM:
        case MONS_EARTH_ELEMENTAL:
        case MONS_GARGOYLE:
            explode = true;
            blast.ex_size = 2;
            strcpy(blast.beam_name, "blast of rock fragments");
            blast.colour = BROWN;
            blast.damage = hurted / 2 + 1;
            blast.flavour = BEAM_FRAG;
            player_hurt_monster(i, 1 + random2(hurted));
            break;

        case MONS_CRYSTAL_GOLEM:
            explode = true;
            blast.ex_size = 2;
            strcpy(blast.beam_name, "blast of crystal shards");
            blast.colour = WHITE;
            blast.damage = hurted;
            blast.flavour = BEAM_FRAG;
            player_hurt_monster(i, 1 + random2( hurted * 2 ));
            break;

        default:
            player_hurt_monster(i, 1 + random2( 1 + (hurted / 30) ));
            goto do_terrain;
        }

        strcat(info, " explodes!");
        mpr(info);
        goto all_done;
    }

  do_terrain:
    // FIXME: do nothing in Abyss & Pandemonium?
    i = grd[beam.tx][beam.ty];

    switch (i)
    {
    case DNGN_ROCK_WALL:
    case DNGN_STONE_WALL:
    case DNGN_SECRET_DOOR:
        what = "wall";
        // fall-through
    case DNGN_ORCISH_IDOL:
        if (what == NULL)
            what = "stone idol";
        // fall-through
    case DNGN_GRANITE_STATUE:   // normal rock -- big explosion
        if (what == NULL)
            what = "statue";

        explode = true;
        blast.ex_size = (i == DNGN_ORCISH_IDOL || i == DNGN_GRANITE_STATUE)?2:1;
        strcpy(blast.beam_name, "blast of rock fragments");
        blast.colour = BROWN;        // FIXME: colour of actual wall?
        blast.damage = hurted;
        blast.flavour = BEAM_FRAG;

        if (one_chance_in(3))
        {
            // digging limited to just rock
            if (i != DNGN_STONE_WALL)
                grd[beam.tx][beam.ty] = DNGN_FLOOR;

            debris = DEBRIS_ROCK;
        }
        break;

    case DNGN_METAL_WALL:       // metal -- small but nasty explosion
        what = "metal wall";
        if (one_chance_in(5))
        {
            // digging limited to just rock
            // grd[beam.tx][beam.target_y] = DNGN_FLOOR;
            debris = DEBRIS_METAL;
        }

        blast.colour = CYAN;
        // fallthru
    case DNGN_SILVER_STATUE:    //jmf: statue not destroyed
        if (what == NULL)
        {
            what = "silver statue";
            blast.colour = WHITE;
        }
        explode = true;
        strcpy(blast.beam_name, "blast of metal fragments");
        blast.damage = (hurted * 4) / 3;
        blast.flavour = BEAM_FRAG;
        break;

    case DNGN_GREEN_CRYSTAL_WALL:       // crystal -- large & nasty explosion
        what = "crystal wall";
        blast.colour = GREEN;

        if (one_chance_in(2))
        {
            grd[beam.tx][beam.ty] = DNGN_FLOOR;
            debris = DEBRIS_CRYSTAL;
        }
        // fallthru
    case DNGN_ORANGE_CRYSTAL_STATUE:
        if (what == NULL)
            what = "crystal statue";

        if (blast.colour == 0)
            blast.colour = LIGHTRED; //jmf: == orange, right?

        explode = true;
        blast.ex_size = 2;
        strcpy(blast.beam_name, "blast of crystal shards");
        blast.damage = (hurted * 3) / 2;
        blast.flavour = BEAM_FRAG;
        break;

    case DNGN_TRAP_MECHANICAL:
        if (coinflip())
            grd[beam.tx][beam.ty] = DNGN_FLOOR;

        // fall-through
    // case DNGN_FLOOR:  // Avoiding Linley's floor material issues -- bwr
                         // It's also a more interesting spell this way.
    case DNGN_TRAP_MAGICAL:
    case DNGN_TRAP_III:
    case DNGN_UNDISCOVERED_TRAP:
    case DNGN_OPEN_DOOR:
    case DNGN_STONE_ARCH:       // floor -- small explosion
        what = "floor";
        explode = true;
        strcpy(blast.beam_name, "blast of rock fragments");
        blast.colour = BROWN;     // FIXME: colour of actual dungeon floor?
        blast.damage = hurted / 2 + 1;
        blast.flavour = BEAM_FRAG;
        break;

    default:
        // FIXME: cute message for water?
        break;
    }

  all_done:
    if (explode)
    {
        if (what != 0)
        {
            snprintf( info, INFO_SIZE, "The %s explodes!", what);
            mpr(info);
        }

        // Now we take the hurted amount and convert it for use as part
        // of the 3dX distribution.
        if (blast.damage < 100)
        {
            blast.damage /= 3;    // find out X (the die size)
            blast.damage += 101;  // marks it as 3dX (+1 for rounding error)

            if (blast.damage < 104)     // give a minimum
                blast.damage = 104;
        }

        explosion(blast);
    }

    if (debris)
        place_debris(beam.tx, beam.ty, debris);

    return;
}                               // end cast_fragmentation()

void cast_twist(int pow)
{
    struct dist targ;
    struct bolt tmp;    // used, but ignored

    // level one power cap -- bwr
    if (pow > 30)
        pow = 30;

    // Get target,  using DIR_TARGET for targetting only,
    // since we don't use beam() for this spell.
    if (spell_direction(targ, tmp, DIR_TARGET) == -1)
        return;

    const int mons = mgrd[ targ.tx ][ targ.ty ];

    // anything there?
    if (mons == NON_MONSTER || targ.isMe)
    {
        mpr("There is no monster there!");
        return;
    }

    // Monster can magically save vs attack.
    if (check_mons_magres( &menv[ mons ], pow * 2 ))
    {
        simple_monster_message( &menv[ mons ], " resists." );
        return;
    }

    // Roll the damage... this spell is pretty low on damage, because
    // it can target any monster in LOS (high utility).  This is
    // similar to the damage done by Magic Dart (although, the
    // distribution is much more uniform). -- bwr
    int damage = 1 + random2( 6 + pow / 4 );

    // Inflict the damage
    player_hurt_monster( mons, damage );
    return;
}                               // end cast_twist()

//
// This version of far strike is a bit too creative for level one, in
// order to make it work we needed to put a lot of restrictions on it
// (like the damage limitation), which wouldn't be necessary if it were
// a higher level spell.  This code might come back as a high level
// translocation spell later (maybe even with special effects if it's
// using some of Josh's ideas about occasionally losing the weapon).
// Would potentially make a good high-level, second book Warper spell
// (since Translocations is a utility school, it should be higher level
// that usual... especially if it turns into a flavoured smiting spell).
// This can all wait until after the next release (as it would be better
// to have a proper way to do a single weapon strike here (you_attack
// does far more than we need or want here)). --bwr
//

void cast_far_strike(int pow)
{
    struct dist targ;
    struct bolt tmp;    // used, but ignored

    // Get target,  using DIR_TARGET for targetting only,
    // since we don't use beam() for this spell.
    if (spell_direction(targ, tmp, DIR_TARGET) == -1)
        return;

    // Get the target monster...
    if (mgrd[targ.tx][targ.ty] == NON_MONSTER
        || targ.isMe)
    {
        mpr("There is no monster there!");
        return;
    }

    //  Start with weapon base damage...
    const int weapon = you.equip[ EQ_WEAPON ];

    int damage = 3;     // default unarmed damage
    int speed  = 10;    // default unarmed time

    if (weapon != -1)   // if not unarmed
    {
        // look up the damage base
        if (you.inv[ weapon ].base_type == OBJ_WEAPONS)
        {
            damage = property( you.inv[ weapon ], PWPN_DAMAGE );
            speed = property( you.inv[ weapon ], PWPN_SPEED );

            if (you.inv[ weapon ].special == SPWPN_SPEED
                || (is_random_artefact( you.inv[ weapon ] )
                    && randart_wpn_property(you.inv[weapon], RAP_BRAND) == SPWPN_SPEED))
            {
                // maybe this should just set it equal to 10, that's
                // pretty much what the result will be anyways.
                speed *= 5;
                speed /= 10;
            }
        }
        else if (you.inv[ weapon ].base_type == OBJ_STAVES)
        {
            damage = property( you.inv[ weapon ], PWPN_DAMAGE );
            speed = property( you.inv[ weapon ], PWPN_SPEED );
        }
    }

    // Because we're casting a spell (and don't want to make this level
    // one spell too good), we're not applying skill speed bonuses and at
    // the very least guaranteeing one full turn (speed == 10) like the
    // other spells (if any thing else related to speed is changed, at
    // least leave this right before the application to you.time_taken).
    // Leaving skill out of the speed bonus is an important part of
    // keeping this spell from becoming a "better than actual melee"
    // spell... although, it's fine if that's the case for early Warpers,
    // Fighter types, and such that pick up this trivial first level spell,
    // shouldn't be using it instead of melee (but rather as an accessory
    // long range plinker).  Therefore, we tone things down to try and
    // guarantee that the spell is never begins to approach real combat
    // (although the magic resistance check might end up with a higher
    // hit rate than attacking against EV for high level Warpers). -- bwr
    if (speed < 10)
        speed = 10;

    you.time_taken *= speed;
    you.time_taken /= 10;

    // Apply strength only to damage (since we're only interested in
    // force here, not finesse... the dex/to-hit part of combat is
    // instead handled via magical ability).  This part could probably
    // just be removed, as it's unlikely to make any real difference...
    // if it is, the Warper stats in newgame.cc should be changed back
    // to the standard 6 int-4 dex of spellcasters. -- bwr
    int dammod = 78;
    const int dam_stat_val = you.strength;

    if (dam_stat_val > 11)
        dammod += (random2(dam_stat_val - 11) * 2);
    else if (dam_stat_val < 9)
        dammod -= (random2(9 - dam_stat_val) * 3);

    damage *= dammod;
    damage /= 78;

    struct monsters *monster = &menv[ mgrd[targ.tx][targ.ty] ];

    // apply monster's AC
    if (monster->armour_class > 0)
        damage -= random2( 1 + monster->armour_class );

#if 0
    // Removing damage limiter since it's categorized at level 4 right now.

    // Force transmitted is limited by skill...
    const int limit = (you.skills[SK_TRANSLOCATIONS] + 1) / 2 + 3;
    if (damage > limit)
        damage = limit;
#endif

    // Roll the damage...
    damage = 1 + random2( damage );

    // Monster can magically save vs attack (this could be replaced or
    // augmented with an EV check).
    if (check_mons_magres( monster, pow * 2 ))
    {
        simple_monster_message( monster, " resists." );
        return;
    }

    // Inflict the damage
    hurt_monster( monster, damage );
    if (monster->hit_points < 1)
        monster_die( monster, KILL_YOU, 0 );
    else
        print_wounds( monster );

    return;
}                               // end cast_far_strike()

void cast_apportation(int pow)
{
    struct dist beam;

    mpr("Pull items from where?");

    direction(beam, DIR_TARGET);

    if (!beam.isValid)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        return;
    }

    // it's already here!
    if (beam.isMe)
    {
        mpr( "That's just silly." );
        return;
    }

    // Protect the player from destroying the item
    const int grid = grd[ you.x_pos ][ you.y_pos ];

    if (grid == DNGN_LAVA || grid == DNGN_DEEP_WATER)
    {
        mpr( "That would be silly while over this terrain!" );
        return;
    }

    // If this is ever changed to allow moving objects that can't
    // be seen, it should at least only allow moving from squares
    // that have been phyisically (and maybe magically) seen and
    // should probably have a range check as well.  In these cases
    // the spell should probably be upped to at least two, or three
    // if magic mapped squares are allowed.  Right now it's okay
    // at one... it has a few uses, but you still have to get line
    // of sight to the object first so it will only help a little
    // with snatching runes or the orb (although it can be quite
    // useful for getting items out of statue rooms or the abyss). -- bwr
    if (!see_grid( beam.tx, beam.ty ))
    {
        mpr( "You cannot see there!" );
        return;
    }

    // Let's look at the top item in that square...
    const int item = igrd[ beam.tx ][ beam.ty ];
    if (item == NON_ITEM)
    {
        mpr( "There are no items there." );
        return;
    }

    // mass of one unit
    const int unit_mass = mass_item( mitm[ item ] );
    // assume we can pull everything
    int max_units = mitm[ item ].quantity;

    // item has mass: might not move all of them
    if (unit_mass > 0)
    {
        // most units our power level will allow (max of 40 aum)
        max_units = ((pow >= 40) ? 400 : (pow * 10)) / unit_mass;
    }

    if (max_units <= 0)
    {
        mpr( "The mass is resisting your pull." );
        return;
    }

    // Failure should never really happen after all the above checking,
    // but we'll handle it anyways...
    if (move_top_item( beam.tx, beam.ty, you.x_pos, you.y_pos ))
    {
        if (max_units < mitm[ item ].quantity)
        {
            mitm[ item ].quantity = max_units;
            mpr( "You feel that some mass got lost in the cosmic void." );
        }
        else
        {
            mpr( "Yoink!" );
            snprintf( info, INFO_SIZE, "You pull the item%s to yourself.",
                                 (mitm[ item ].quantity > 1) ? "s" : "" );
            mpr( info );
        }
    }
    else
        mpr( "The spell fails." );
}

void cast_sandblast(int pow)
{
    bool big = true;
    struct dist spd;
    struct bolt beam;

    // this type of power manipulation should be done with the others,
    // currently over in it_use2.cc (ack) -- bwr
    // int hurt = 2 + random2(5) + random2(4) + random2(pow) / 20;

    big = false;

    if (you.equip[EQ_WEAPON] != -1)
    {
        int wep = you.equip[EQ_WEAPON];
        if (you.inv[wep].base_type == OBJ_MISSILES
           && (you.inv[wep].sub_type == MI_STONE || you.inv[wep].sub_type == MI_LARGE_ROCK))
           big = true;
    }

    if (spell_direction(spd, beam) == -1)
        return;

    if (spd.isMe)
    {
        canned_msg(MSG_UNTHINKING_ACT);
        return;
    }

    if (big)
    {
        dec_inv_item_quantity( you.equip[EQ_WEAPON], 1 );
        zapping(ZAP_SANDBLAST, pow, beam);
    }
    else
    {
        zapping(ZAP_SMALL_SANDBLAST, pow, beam);
    }
}                               // end cast_sandblast()

static bool mons_can_host_shuggoth(int type)    //jmf: simplified
{
    if (mons_holiness(type) != MH_NATURAL)
        return false;
    if (mons_flag(type, M_WARM_BLOOD))
        return true;

    return false;
}

void cast_shuggoth_seed(int powc)
{
    struct dist beam;
    int i;

    mpr("Sow seed in whom?", MSGCH_PROMPT);

    direction(beam, DIR_TARGET);

    if (!beam.isValid)
    {
        mpr("You feel a distant frustration.");
        return;
    }

    if (beam.isMe)
    {
        if (!you.is_undead)
        {
            you.duration[DUR_INFECTED_SHUGGOTH_SEED] = 10;
            mpr("A deathly dread twitches in your chest.");
        }
        else
            mpr("You feel a distant frustration.");
    }

    i = mgrd[beam.tx][beam.ty];

    if (i == NON_MONSTER)
    {
        mpr("You feel a distant frustration.");
        return;
    }

    if (mons_can_host_shuggoth(menv[i].type))
    {
        if (random2(powc) > 100)
            mons_add_ench(&menv[i], ENCH_YOUR_SHUGGOTH_III);
        else
            mons_add_ench(&menv[i], ENCH_YOUR_SHUGGOTH_IV);

        simple_monster_message(&menv[i], " twitches.");
    }

    return;
}

void cast_condensation_shield(int pow)
{
    if (you.equip[EQ_SHIELD] != -1 || you.fire_shield)
        canned_msg(MSG_SPELL_FIZZLES);
    else
    {
        if (!you.duration[DUR_CONDENSATION_SHIELD])
        {
            mpr("A crackling disc of dense vapour forms in the air!");
            you.redraw_armour_class = 1;
        }

        you.duration[DUR_CONDENSATION_SHIELD] += 10
                                    + ((random2(pow) + random2(pow)) / 2);

        if (you.duration[DUR_CONDENSATION_SHIELD] > 50)
            you.duration[DUR_CONDENSATION_SHIELD] = 50;
    }

    return;
}                               // end cast_condensation_shield()

static int quadrant_blink(int x, int y, int pow, int garbage)
{
    if (x == you.x_pos && y == you.y_pos)
        return 0;

    if (pow > 100)
        pow = 100;

    // setup: Brent's new algorithm
    // we are interested in two things: distance of a test point from
    // the ideal 'line',  and the distance of a test point from two
    // actual points,  one in the 'correct' direction and one in the
    // 'incorrect' direction.

    // scale distance by 10 for more interesting numbers.
    int l,m;        // for line equation lx + my = 0
    l = (x - you.x_pos);
    m = (you.y_pos - y);

    int tx, ty;         // test x,y
    int rx, ry;         // x,y relative to you.
    int sx, sy;         // test point in the correct direction
    int bx = x;         // best x
    int by = y;         // best y

    int best_dist = 10000;

    sx = l;
    sy = -m;

    // for each point (a,b), distance from the line is | la + mb |

    for(int tries = pow * pow / 500 + 1; tries > 0; tries--)
    {
        if (!random_near_space(you.x_pos, you.y_pos, tx, ty))
            return 0;

        rx = tx - you.x_pos;
        ry = ty - you.y_pos;

        int dist = l * rx + m * ry;
        dist *= 10 * dist;      // square and multiply by 10

        // check distance to test points
        int dist1 = distance(rx, ry, sx, sy) * 10;
        int dist2 = distance(rx, ry, -sx, -sy) * 10;

        // 'good' points will always be closer to test point 1
        if (dist2 < dist1)
            dist += 80;          // make the point less attractive

        if (dist < best_dist)
        {
            best_dist = dist;
            bx = tx;
            by = ty;
        }
    }

    you.x_pos = bx;
    you.y_pos = by;

    if (you.level_type == LEVEL_ABYSS)
    {
        abyss_teleport();
        you.pet_target = MHITNOT;
    }

    return 1;
}

void cast_semi_controlled_blink(int pow)
{
    apply_one_neighbouring_square(quadrant_blink, pow);
    return;
}

void cast_stoneskin(int pow)
{
    if (you.is_undead != 0)
    {
        mpr("This spell does not affect your undead flesh.");
        return;
    }

    if (!(you.attribute[ATTR_TRANSFORMATION] == TRAN_NONE ||
          you.attribute[ATTR_TRANSFORMATION] == TRAN_BLADE_HANDS))
    {
        mpr("This spell does not affect your current form.");
        return;
    }

    if (you.duration[DUR_STONEMAIL] || you.duration[DUR_ICY_ARMOUR])
    {
        mpr("This spell conflicts with another spell still in effect.");
        return;
    }

    if (!you.duration[DUR_STONESKIN])
    {
        mpr("Your skin hardens.");
        you.redraw_armour_class = 1;
    }
    else
        mpr("Your skin feels harder.");

    you.duration[DUR_STONESKIN] += 10 + random2(pow) + random2(pow);

    if (you.duration[DUR_STONESKIN] > 50)
        you.duration[DUR_STONESKIN] = 50;
}
