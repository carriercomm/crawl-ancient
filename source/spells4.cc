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

#include <string.h>
#include <stdio.h>

#include "globals.h"
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
#include "it_use2.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "invent.h"
#include "misc.h"
#include "monplace.h"
#include "monstuff.h"
#include "mon-util.h"
#include "mstuff2.h"
#include "ouch.h"
#include "player.h"
#include "randart.h"
#include "religion.h"
#include "skills.h"
#include "spells1.h"
#include "spells3.h"
#include "spells4.h"
#include "spl-cast.h"
#include "spl-util.h"
#include "stuff.h"
#include "transfor.h"
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

// static int make_a_random_cloud(int x, int y, int pow, int ctype);
static int make_a_rot_cloud( int x, int y, int pow, int ctype );

//void cast_animate_golem(int pow); // see actual function for reasoning {dlb}
//void cast_detect_magic(int pow);  //jmf: as above...
//void cast_eringyas_surprising_bouquet(int powc);
void do_monster_rot(int mon);

//jmf: FIXME: put somewhere else (misc.cc?)
// A feeble attempt at Nethack-like completeness for cute messages.
const char *your_hand( bool plural )
{
    static char hand_buff[80];

    switch (you.attribute[ATTR_TRANSFORMATION])
    {
    default:
        mpr("ERROR: unknown transformation in your_hand() (spells4.cc)");
    case TRAN_NONE:
    case TRAN_STATUE:
        if (you.species == SP_TROLL || you.species == SP_GHOUL)
        {
            strcpy(hand_buff, "claw");
            break;
        }
        // or fall-through
    case TRAN_ICE_BEAST:
    case TRAN_LICH:
        strcpy(hand_buff, "hand");
        break;
    case TRAN_SPIDER:
        strcpy(hand_buff, "front leg");
        break;
    case TRAN_SERPENT_OF_HELL:
    case TRAN_DRAGON:
        strcpy(hand_buff, "foreclaw");
        break;
    case TRAN_BLADE_HANDS:
        strcpy(hand_buff, "scythe-like blade");
        break;
    case TRAN_AIR:
        strcpy(hand_buff, "misty tendril");
        break;
    }

    if (plural)
        strcat(hand_buff, "s");

    return (hand_buff);
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
        large = make_item( random2(3), OBJ_MISSILES, MI_LARGE_ROCK, true, 1, 250 );
        small = make_item( 3 + random2(6) + random2(6) + random2(6),
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
    UNUSED( x );
    UNUSED( y );
    UNUSED( debris_type );
    return;
#endif
}                               // end place_debris()

// Here begin the actual spells:
static int shatter_monsters(int x, int y, int pow, int garbage)
{
    UNUSED( garbage );

    dice_def   dam_dice( 0, 5 + pow / 5 );  // number of dice set below
    const int  mid = mgrd[x][y];

    if (mid == NON_MONSTER)
        return (0);

    // Removed a lot of silly monsters down here... people, just because
    // it says ice, rock, or iron in the name doesn't mean it's actually
    // made out of the substance. -- bwr
    switch (menv[mid].type)
    {
    case MONS_ICE_BEAST:        // 3/2 damage
    case MONS_SIMULACRUM_SMALL:
    case MONS_SIMULACRUM_LARGE:
        dam_dice.num = 4;
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
        dam_dice.num = 6;
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
        dam_dice.num = 0;
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
        dam_dice.num = 1;
        dam_dice.size /= 2;
        break;

    case MONS_DANCING_WEAPON:     // flies, but earth based
    case MONS_MOLTEN_GARGOYLE:
    case MONS_QUICKSILVER_DRAGON:
        // Soft, earth creatures... would normally resist to 1 die, but
        // are sensitive to this spell. -- bwr
        dam_dice.num = 2;
        break;

    default:                    // normal damage
        if (mons_flies( &menv[mid] ))
            dam_dice.num = 1;
        else
            dam_dice.num = 3;
        break;
    }

    int damage = roll_dice( dam_dice );
    damage = apply_mons_armour( damage, &menv[mid] );

    you_hurt_monster( &menv[mid], damage );

    return (damage);
}                               // end shatter_monsters()

static int shatter_items(int x, int y, int pow, int garbage)
{
    UNUSED( pow );
    UNUSED( garbage );

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
        if (player_can_hear(x, y))
            mpr(MSGCH_SOUND,"You hear glass break." );

        return 1;
    }

    return 0;
}                               // end shatter_items()

static int shatter_walls(int x, int y, int pow, int garbage)
{
    UNUSED( garbage );

    int  chance = 0;
    int  stuff = 0;

    // if not in-bounds then we can't really shatter it -- bwr
    if (!in_bounds( x, y ))
        return (0);

    switch (grd[x][y])
    {
    case DNGN_SECRET_DOOR:
        if (see_grid(x, y))
            mpr("A secret door shatters!");
        grd[x][y] = DNGN_FLOOR;
        stuff = DEBRIS_WOOD;
        chance = 100;
        break;

    case DNGN_CLOSED_DOOR:
    case DNGN_OPEN_DOOR:
        if (see_grid(x, y))
            mpr("A door shatters!");
        grd[x][y] = DNGN_FLOOR;
        stuff = DEBRIS_WOOD;
        chance = 100;
        break;

    case DNGN_METAL_WALL:
    case DNGN_SILVER_STATUE:
        stuff = DEBRIS_METAL;
        chance = pow / 10;
        break;

    case DNGN_ORCISH_IDOL:
    case DNGN_GRANITE_STATUE:
        chance = 50;
        stuff = DEBRIS_STONE;
        break;

    case DNGN_STONE_WALL:
        chance = pow / 6;
        stuff = DEBRIS_STONE;
        break;

    case DNGN_ROCK_WALL:
        chance = pow / 4;
        stuff = DEBRIS_ROCK;
        break;

    case DNGN_ORANGE_CRYSTAL_STATUE:
        chance = pow / 6;
        stuff = DEBRIS_CRYSTAL;
        break;

    case DNGN_GREEN_CRYSTAL_WALL:
        chance = 50;
        stuff = DEBRIS_CRYSTAL;
        break;

    default:
        break;
    }

    if (stuff && random2(100) < chance)
    {
        noisy( SL_EARTHQUAKE, x, y );
        grd[x][y] = DNGN_FLOOR;
        place_debris(x, y, stuff);
        return (1);
    }

    return (0);
}                               // end shatter_walls()

void cast_shatter(int pow)
{
    int damage = 0;

    const bool sil = silenced( you.x_pos, you.y_pos );

    mpr( ((!sil) ? MSGCH_PLAIN : MSGCH_SOUND),
            "The dungeon %s!", (sil) ? "shakes" : "rumbles");

    noisy( SL_EARTHQUAKE, you.x_pos, you.y_pos );

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
        damage = 15 + roll_zdice( 4, (pow / 5) + 1 );
        break;

    case TRAN_ICE_BEAST:        // 1/2 damage
        damage = 10 + roll_zdice( 4, (pow / 5) + 1 ) / 2;
        break;

    case TRAN_BLADE_HANDS:      // 2d3 damage
        mpr("Your scythe-like blades vibrate painfully!");
        damage = roll_dice(2,5);
        break;

    default:
        mpr("cast_shatter(): unknown transformation in spells4.cc");
    }

    if (damage)
        ouch(damage, 0, KILLED_BY_TARGETTING);

    int rad = 3 + (you.skills[SK_EARTH_MAGIC] / 5);

    apply_area_within_radius(shatter_items, you.x_pos, you.y_pos, pow, rad, 0);
    apply_area_within_radius(shatter_monsters, you.x_pos, you.y_pos, pow, rad, 0);
    int dest = apply_area_within_radius( shatter_walls, you.x_pos, you.y_pos,
                                         pow, rad, 0 );

    if (dest && !sil)
        mpr( MSGCH_SOUND,"Ka-crash!" );
}                               // end cast_shatter()

// cast_forescry: raises evasion (by 8 currently) via divination
void cast_forescry( int pow )
{
    if (!you.duration[DUR_FORESCRY])
        mpr("You begin to receive glimpses of the immediate future...");

    you.duration[DUR_FORESCRY] += 5 + random2(pow);

    if (you.duration[DUR_FORESCRY] > 30)
        you.duration[DUR_FORESCRY] = 30;

    set_redraw_status( REDRAW_EVASION );
}                               // end cast_forescry()

void cast_see_invisible( int pow )
{
    if (player_see_invis())
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
static void cast_detect_magic( int pow )
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

    mpr(MSGCH_PROMPT,"Which direction?" );
    direction( bmove, DIR_DIR );

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
        if (is_dumpable_artefact
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
void cast_detect_secret_doors( int pow )
{
    int found = 0;

    for (int x = you.x_pos - 8; x <= you.x_pos + 8; x++)
    {
        for (int y = you.y_pos - 8; y <= you.y_pos + 8; y++)
        {
            if (!in_bounds( x, y ))
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

        mpr( "You detect %s secret door%s.",
             (found > 1) ? "some" : "a", (found > 1) ? "s" : "" );
    }
}                               // end cast_detect_secret_doors()

bool cast_summon_butterflies( int pow )
{
    bool ret = false;

    // explicitly limiting the number
    int num = 4 + random2(3) + random2( pow ) / 10;
    if (num > 16)
        num = 16;

    for (int scount = 1; scount < num; scount++)
    {
        if (create_monster( MONS_BUTTERFLY, BEH_FRIENDLY, 3 ) != -1)
            ret = true;
    }

    return (ret);
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

    create_monster( mon, BEH_FRIENDLY, 3 );
}

void cast_sticks_to_snakes( int pow )
{
    int mon, i;
    beh_type behaviour;

    int how_many = 0;

    int max = 1 + random2( 1 + you.skills[SK_TRANSMIGRATION] ) / 4;

    int dur = 3 + random2(pow) / 20;
    if (dur > 5)
        dur = 5;

    const int stick = get_inv_in_hand();

    if (stick == -1)
    {
        mpr( "Your %s feel slithery!", your_hand(true) );
        return;
    }

    behaviour = item_cursed( you.inv[stick] ) ? BEH_HOSTILE
                                              : BEH_FRIENDLY;

    if ((you.inv[stick].base_type == OBJ_MISSILES
         && (you.inv[stick].sub_type == MI_ARROW)))
    {
        if (you.inv[stick].quantity < max)
            max = you.inv[stick].quantity;

        for (i = 0; i < max; i++)
        {
            //jmf: perhaps also check for poison ammo?
            if (pow > 50 || (pow > 25 && one_chance_in(3)))
                mon = MONS_SNAKE;
            else
                mon = MONS_SMALL_SNAKE;

            if (create_monster( mon, behaviour, dur ) != -1)
                how_many++;
        }
    }

    if (you.inv[stick].base_type == OBJ_WEAPONS
        && (you.inv[stick].sub_type == WPN_CLUB
            || you.inv[stick].sub_type == WPN_SPEAR
            || you.inv[stick].sub_type == WPN_QUARTERSTAFF
            || you.inv[stick].sub_type == WPN_STAFF
            || you.inv[stick].sub_type == WPN_SCYTHE
            || you.inv[stick].sub_type == WPN_GIANT_CLUB
            || you.inv[stick].sub_type == WPN_GIANT_SPIKED_CLUB
            || you.inv[stick].sub_type == WPN_BOW
            || you.inv[stick].sub_type == WPN_LONGBOW
            || you.inv[stick].sub_type == WPN_ANCUS
            || you.inv[stick].sub_type == WPN_HALBERD
            || you.inv[stick].sub_type == WPN_GLAIVE
            || you.inv[stick].sub_type == WPN_BLOWGUN))
    {
        how_many = 1;

        // Upsizing Snakes to Brown Snakes as the base class for using
        // the really big sticks (so bonus applies really only to trolls,
        // ogres, and most importantly ogre magi).  Still it's unlikely
        // any character is strong enough to bother lugging a few of
        // these around.  -- bwr
        if (item_mass( you.inv[stick] ) < 500)
            mon = MONS_SNAKE;
        else
            mon = MONS_BROWN_SNAKE;

        if (pow > 90 && one_chance_in(3))
            mon = MONS_GREY_SNAKE;

        if (pow > 70 && one_chance_in(3))
            mon = MONS_BLACK_SNAKE;

        if (pow > 40 && one_chance_in(3))
            mon = MONS_YELLOW_SNAKE;

        if (pow > 20 && one_chance_in(3))
            mon = MONS_BROWN_SNAKE;

        create_monster( mon, behaviour, dur );
    }

#ifdef USE_DEBRIS_CODE
    if (you.inv[stick].base_type == OBJ_DEBRIS
        && (you.inv[stick].sub_type == DEBRIS_WOOD))
    {
        // this is how you get multiple big snakes
        how_many = 1;
        mpr("FIXME: implement OBJ_DEBRIS conversion! (spells4.cc)");
    }
#endif // USE_DEBRIS_CODE

    if (how_many > you.inv[stick].quantity)
        how_many = you.inv[stick].quantity;

    if (how_many)
    {
        dec_inv_item_quantity( stick, how_many );

        snprintf( info, INFO_SIZE, "You create %s snake%s!",
                  (how_many > 1) ? "some" : "a", (how_many > 1) ? "s" : "" );
    }
    else
    {
        snprintf( info, INFO_SIZE, "Your %s feel slithery!", your_hand(true) );
    }

    mpr(info);
    return;
}                               // end cast_sticks_to_snakes()

struct dragon_def
{
    monster_type        mon;
    skill_type          elem;
    int                 rare;
};

// We might also consider changing this to using material
// components eventually, but for now we'll just use skills.
void cast_summon_dragon( int pow )
{
    const dragon_def dragons[] =
    {
        // Note: increased rarity of small types to boost creation of
        // full Dragons for Fire elementalists.
        { MONS_FIRE_DRAKE,         SK_FIRE_MAGIC,    8 },
        { MONS_STEAM_DRAGON,       SK_FIRE_MAGIC,    5 },
        { MONS_MOTTLED_DRAGON,     SK_FIRE_MAGIC,    5 },
        { MONS_LINDWURM,           SK_FIRE_MAGIC,    4 },
        { MONS_DRAGON,             SK_FIRE_MAGIC,   10 },

        { MONS_FROST_DRAKE,        SK_ICE_MAGIC,    10 },
        { MONS_ICE_DRAGON,         SK_ICE_MAGIC,     8 },

        { MONS_EARTHWURM,          SK_EARTH_MAGIC,  10 },
        { MONS_IRON_DRAGON,        SK_EARTH_MAGIC,   6 },

        { MONS_SPARK_DRAKE,        SK_AIR_MAGIC,    10 },
        { MONS_STORM_DRAGON,       SK_AIR_MAGIC,     6 },

        { MONS_SWAMP_DRAKE,        SK_POISON_MAGIC, 10 },
        { MONS_SWAMP_DRAGON,       SK_POISON_MAGIC,  9 },

        { MONS_DEATH_DRAKE,        SK_NECROMANCY,    4 },
        { MONS_SHADOW_DRAGON,      SK_NECROMANCY,    4 },
        { MONS_SKELETAL_DRAGON,    SK_NECROMANCY,    4 },

        { MONS_QUICKSILVER_DRAGON, SK_NONE,          3 }, // earth/air/conj
        { MONS_GOLDEN_DRAGON,      SK_NONE,          3 }, // fire/ice/poison
    };

    const int num_dragons = sizeof( dragons ) / sizeof( dragon_def );

    int num_summ = 0;
    int num_unhappy = 0;
    int hd = 0;

    // Max hd here chosen so that this spell doesn't give multiple full
    // dragons (but can give multiple drakes... possibily with a single
    // major dragon). -- bwr
    for (int total_hd = 0; total_hd < 9; total_hd += hd)
    {
        int           drag;
        monster_type  mon;

        int sk = 0;

        // #attempts controls chance of getting an off-element surprise
        for (int i = 0; i < 40; i++)
        {
            do
            {
                drag = random2( num_dragons );
            }
            while (random2(10) >= dragons[drag].rare);

            mon = dragons[drag].mon;
            hd = mons_class_hit_dice( mon );

            if (dragons[drag].mon == MONS_GOLDEN_DRAGON)
            {
                // special case: golden dragons have triple element alignment
                sk = (2 * (you.skills[SK_FIRE_MAGIC]
                            + you.skills[SK_ICE_MAGIC]
                            + you.skills[SK_POISON_MAGIC])) / 3 ;
            }
            else if (dragons[drag].mon == MONS_QUICKSILVER_DRAGON)
            {
                // special case: making quicksilver a top level earth/air
                // to complement the golden dragon.  Using conjurations
                // here as well because the breath is like magic missile.
                sk = (2 * (you.skills[SK_EARTH_MAGIC]
                            + you.skills[SK_AIR_MAGIC]
                            + you.skills[SK_CONJURATIONS])) / 3 ;
            }
            else
            {
                ASSERT( dragons[drag].elem != SK_NONE ); // special cases done

                sk = 2 * you.skills[ dragons[drag].elem ];
            }

            if (random2(sk) > hd)
                break;
        }

        beh_type  beh = BEH_HOSTILE;

        // adjust power by skill factor for friendliness test:
        if (random2( (pow * sk) / 30 ) > hd)
            beh = ((random2(sk) > hd) ? BEH_FRIENDLY : BEH_CHARMED);

        if (create_monster( mon, beh, 3 ) != -1)
        {
            num_summ++;
            num_unhappy += (beh != BEH_FRIENDLY);
        }
    }

    if (!num_summ)
        canned_msg( MSG_NOTHING_HAPPENS );
    else if (num_summ == 1)
    {
        strncpy( info, "A dragon appears", INFO_SIZE );

        if (num_unhappy)
            strncat( info, ", but it doesn't appear very happy.", INFO_SIZE );
        else
            strncat( info, ".", INFO_SIZE );

        mpr( info );
    }
    else // if (num_summ > 1)
    {
        strncpy( info, "Some dragons appear", INFO_SIZE );

        if (num_unhappy == 0)
            strncat( info, ".", INFO_SIZE );
        else if (num_unhappy != num_summ)
            strncat( info, ", but they don't all look happy.", INFO_SIZE );
        else
            strncat( info, ", but they don't appear very happy.", INFO_SIZE );

        mpr( info );
    }
}                               // end cast_summon_dragon()

void cast_conjure_ball_lightning( int pow )
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

        // int mon = create_monster( MONS_BALL_LIGHTNING, BEH_FRIENDLY, 0,
        //                           tx, ty, MHITNOT, 250 );

        if (mon != -1)
        {
            menv[mon].flags |= MF_SHORT_LIVED;
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
    UNUSED( garbage );
    int mnstr = mgrd[x][y];

    if (mnstr == NON_MONSTER)                                   return 0;
    if (mons_holiness( &menv[mnstr] ) != MH_NATURAL)             return 0;
    if (check_mons_resist_magic( &menv[mnstr], pow ))           return 0;

    // Why shouldn't we be able to sleep friendly monsters? -- bwr
    // if (mons_friendly( &menv[mnstr] ))                          return 0;

    //jmf: now that sleep == hibernation:
    if (mons_res_cold( &menv[mnstr] ) > 0 && coinflip())        return 0;
    if (mons_has_ench( &menv[mnstr], ENCH_SLEEP_WARY ))         return 0;

    menv[mnstr].behaviour = BEH_SLEEP;
    mons_add_ench( &menv[mnstr], ENCH_SLEEP_WARY );

    if (mons_class_flag( menv[mnstr].type, M_COLD_BLOOD ) && coinflip())
        mons_add_ench( &menv[mnstr], ENCH_SLOW );

    return 1;
}                               // end sleep_monsters()

void cast_mass_sleep(int pow)
{
    apply_area_visible(sleep_monsters, pow);
}                               // end cast_mass_sleep()

static int tame_beast_monsters( int x, int y, int pow, int garbage )
{
    UNUSED( garbage );
    int which_mons = mgrd[x][y];

    if (which_mons == NON_MONSTER)
        return (0);

    struct monsters *monster = &menv[which_mons];

    if (mons_holiness( monster ) != MH_NATURAL
        || mons_intel_type( monster->type ) != I_ANIMAL
        || mons_friendly( monster ))
    {
        return (0);
    }

    // 50% bonus for dogs, add cats if they get implemented
    if (monster->type == MONS_HOUND
        || monster->type == MONS_WAR_DOG
        || monster->type == MONS_BLACK_BEAR)
    {
        pow += (pow / 2);
    }

    // 50% bonus for wargs, but only with orcs
    if (you.species == SP_HILL_ORC && monster->type == MONS_WARG)
        pow += (pow / 2);

    if (check_mons_resist_magic( monster, pow ))
        return (0);

    // I'd like to make the monsters affected permanently, but that's
    // pretty powerful. Maybe a small (pow/10) chance of being permanently
    // tamed, large chance of just being enslaved.
    mon_msg(monster, " is tamed!");

    if (random2(100) < random2(pow / 10))
        monster->attitude = ATT_FRIENDLY;       // permanent, right?
    else
        mons_add_ench( monster, ENCH_CHARM );

    return (1);
}                               // end tame_beast_monsters()

void cast_tame_beasts(int pow)
{
    apply_area_visible(tame_beast_monsters, pow);
}                               // end cast_tame_beasts()

static int ignite_poison_objects(int x, int y, int pow, int garbage)
{
    UNUSED( pow );
    UNUSED( garbage );

    int obj = igrd[x][y], next, strength = 0;

    if (obj == NON_ITEM)
        return (0);

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
        place_cloud( CLOUD_FIRE, x, y, strength + roll_dice(3, strength / 4) );

    return (strength);
}                               // end ignite_poison_objects()

static int ignite_poison_clouds( int x, int y, int pow, int garbage )
{
    UNUSED( pow );
    UNUSED( garbage );

    int did_anything = 0;

    const int cloud = env.cgrid[x][y];

    if (cloud != EMPTY_CLOUD)
    {
        if (env.cloud[ cloud ].type == CLOUD_STINK
            || env.cloud[ cloud ].type == CLOUD_STINK_MON)
        {
            did_anything++;
            env.cloud[ cloud ].type = CLOUD_FIRE;

            env.cloud[ cloud ].decay /= 2;

            if (env.cloud[ cloud ].decay < 1)
                env.cloud[ cloud ].decay = 1;
        }
        else if (env.cloud[ cloud ].type == CLOUD_POISON
                 || env.cloud[ cloud ].type == CLOUD_POISON_MON)
        {
            did_anything++;
            env.cloud[ cloud ].type = CLOUD_FIRE;
        }
    }

    return (did_anything);
}                               // end ignite_poison_clouds()

static int ignite_poison_monsters(int x, int y, int pow, int garbage)
{
    UNUSED( garbage );

    struct bolt beam;
    beam.flavour = BEAM_FIRE;   // this is dumb, only used for adjust!

    dice_def  dam_dice( 0, 5 + pow / 7 );  // dice added below if applicable

    const int mon_index = mgrd[x][y];
    if (mon_index == NON_MONSTER)
        return (0);

    struct monsters *const mon = &menv[ mon_index ];

    // Monsters which have poison corpses or poisonous attacks:
    if (mons_corpse_effect( mon->type ) == CE_POISONOUS
        || mon->type == MONS_GIANT_ANT
        || mon->type == MONS_SMALL_SNAKE
        || mon->type == MONS_SNAKE
        || mon->type == MONS_JELLYFISH
        || mons_is_mimic( mon->type ))
    {
        dam_dice.num = 3;
    }

    // Monsters which are poisoned:
    int strength = 0;

    // first check for player poison:
    mon_enchant_def *const ench = mons_has_ench( mon, ENCH_POISON );
    if (ench)
        strength += dur_to_levels( ENCH_POISON, ench->duration );

    // strength is now the sum of both poison types (although only
    // one should actually be present at a given time):
    dam_dice.num += strength;

    int damage = roll_dice( dam_dice );
    if (damage > 0)
    {
        damage = mons_adjust_flavoured( mon, beam, damage );

#if DEBUG_DIAGNOSTICS
        mpr( MSGCH_DIAGNOSTICS, "Dice: %dd%d; Damage: %d",
                dam_dice.num, dam_dice.size, damage );

#endif

        // if monster survives, remove the poison.
        if (!you_hurt_monster( &menv[mon_index], damage ))
            mons_del_ench_ptr( mon, ench );

        return (1);
    }

    return (0);
}

void cast_ignite_poison( int pow )
{
    int damage = 0, strength = 0, pcount = 0, acount = 0, totalstrength = 0;
    char item;
    bool wasWielding = false;
    char str_pass[ ITEMNAME_SIZE ];

    // temp weapon of venom => temp fire brand
    const int wpn = get_inv_wielded();

    if (wpn != -1
        && you.duration[DUR_WEAPON_BRAND]
        && get_weapon_brand( you.inv[wpn] ) == SPWPN_VENOM)
    {
        if (set_item_ego_type( you.inv[wpn], OBJ_WEAPONS, SPWPN_FLAMING ))
        {
            in_name( wpn, DESC_CAP_YOUR, str_pass );
            strcpy( info, str_pass );
            strcat( info, " bursts into flame!" );
            mpr(info);

            set_redraw_status( REDRAW_WIELD );
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
            if (you.inv[item].special == SPMSL_POISONED
                || you.inv[item].special == SPMSL_POISONED_UNUSED)
            {
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
            dec_inv_item_quantity( item, you.inv[item].quantity );

        totalstrength += strength;
    }

    if (acount > 0)
        mpr( "Some projectiles you are carrying burn!" );

    if (pcount > 0)
    {
        mpr( "%s potion%s you are carrying explode%s!",
                (pcount > 1) ? "Some" : "A",
                (pcount > 1) ? "s" : "",
                (pcount > 1) ? "" : "s" );
    }

    if (wasWielding == true)
        canned_msg( MSG_EMPTY_HANDED );

    if (totalstrength)
    {
        place_cloud( CLOUD_FIRE, you.x_pos, you.y_pos,
                     1 + roll_zdice( 4, 1 + totalstrength / 4 ) );
    }

    // player is poisonous
    if (you.mutation[MUT_SPIT_POISON] || you.mutation[MUT_STINGER]
        || you.attribute[ATTR_TRANSFORMATION] == TRAN_SPIDER // poison attack
        || (!transform_changed_physiology()
            && (you.species == SP_GREEN_DRACONIAN       // poison breath
                || you.species == SP_KOBOLD             // poisonous corpse
                || you.species == SP_NAGA)))            // spit poison
    {
        damage = roll_dice( 3, 5 + pow / 7 );
    }

    // player is poisoned
    damage += roll_dice( you.poison, 6 );

    if (damage)
    {
        const int resist = player_res_fire();

        if (resist > 0)
        {
            mpr("You feel like your blood is boiling!");
            damage = damage / 3;
        }
        else if (resist < 0)
        {
            damage *= 3;
            mpr("The poison in your system burns terribly!");
        }
        else
        {
            mpr("The poison in your system burns!");
        }

        ouch( damage, 0, KILLED_BY_TARGETTING );

        if (you.poison > 0)
        {
            mpr( "You feel that the poison has left your system." );
            you.poison = 0;
        }
    }

    apply_area_visible( ignite_poison_clouds, pow );
    apply_area_visible( ignite_poison_objects, pow );
    apply_area_visible( ignite_poison_monsters, pow );
}                               // end cast_ignite_poison()

int cast_quiet( int pow, const struct bolt &beam )
{
    const int mon = mgrd[beam.target_x][beam.target_y];

    if (mon == NON_MONSTER)
    {
        canned_msg( MSG_NOTHING_HAPPENS );
        return (SPRET_FAIL);
    }

    struct monsters *const monster = &menv[mon];

    if (check_mons_resist_magic( monster, pow ))
    {
        mon_msg( monster, " resists." );
        return (SPRET_FAIL);
    }

    zap_animation( element_colour( EC_AIR ), monster );
    mons_add_ench( monster, ENCH_QUIET );
    mon_msg( monster, " is enveloped in silence!" );

    return (SPRET_SUCCESS);
}

void cast_silence(int pow)
{
    if (!you.attribute[ATTR_WAS_SILENCED])
        mpr("A profound silence engulfs you.");

    you.attribute[ATTR_WAS_SILENCED] = 1;

    you.duration[DUR_SILENCE] += 10 + roll_dice( 2, pow / 2 );

    if (you.duration[DUR_SILENCE] > 100)
        you.duration[DUR_SILENCE] = 100;
}                               // end cast_silence()


/* ******************************************************************
// no hooks for this anywhere {dlb}:

void cast_animate_golem(int pow)
{
    // must have more than 20 max_hitpoints
    // must be wielding a Scroll of Paper (for chem)
    // must be standing on a pile of wood, metal, rock, or stone
    // Will cost you 5-10% of max_hitpoints, or 20 + some, whichever is more

    mpr("You imbue the inanimate form with a portion of your life force.");

    did_god_conduct( DID_CREATED_LIFE, 10 );
}

****************************************************************** */

static int discharge_monsters( int x, int y, int pow, int garbage )
{
    UNUSED( garbage );

    int         damage = 0;
    struct bolt beam;

    beam.flavour = BEAM_ELECTRICITY; // used for mons_adjust_flavoured

    if (x == you.x_pos && y == you.y_pos)
    {
        if (player_res_electricity() > 0 || player_is_levitating())
            return (0);

        zap_animation( element_colour( EC_ELECTRICITY ) );

        mpr( "You are struck by lightning." );
        damage = 3 + random2( 5 + pow / 5 );
        damage = check_your_resists( damage, BEAM_ELECTRICITY );
        ouch( damage, 0, KILLED_BY_WILD_MAGIC );
    }
    else
    {
        const int mon = mgrd[x][y];

        if (mon == NON_MONSTER)
            return (0);

        if (mons_res_elec( &menv[mon] ) > 0 || mons_flies( &menv[mon] ))
            return (0);

        zap_animation( element_colour( EC_ELECTRICITY ), &menv[mon], true );

        damage = 3 + random2( 5 + pow / 5 );
        damage = mons_adjust_flavoured( &menv[mon], beam, damage );

        if (damage)
        {
            mon_msg( &menv[mon], " is struck by lightning.", false );
            you_hurt_monster( &menv[mon], damage );
        }
    }

    // Recursion to give us chain-lightning -- bwr
    if (pow >= 10 && !one_chance_in(3))
    {
        mpr( "The lightning arcs!" );
        pow -= random2(10);
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
    int num_targs = 1 + random2( 1 + pow / 10 );
    int dam;

    dam = apply_random_around_square( discharge_monsters, you.x_pos, you.y_pos,
                                      true, pow, num_targs );

#if DEBUG_DIAGNOSTICS
    mpr( MSGCH_DIAGNOSTICS, "Arcs: %d Damage: %d", num_targs, dam );
#endif

    if (dam == 0)
    {
        if (coinflip())
            mpr("The air around you crackles with electrical energy.");
        else
        {
            const bool plural = coinflip();

            mpr( "%s blue arc%s ground%s harmlessly %s you.",
                    (plural) ? "Some" : "A",
                    (plural) ? "s" : "",
                    (plural) ? " themselves" : "s itself",
                    (plural) ? "around" : (coinflip() ? "beside" :
                                           coinflip() ? "behind" : "before") );
        }
    }
}                               // end cast_discharge()


int distort_being( int mid, int killer, bool apply_damage )
{
    ASSERT( mid != MHITNOT );

    int damage = 0;

    //jmf: blink frogs *like* distortion
    // I think could be amended to let blink frogs "grow" like
    // jellies do {dlb}
    if (mid != MHITYOU && menv[mid].type == MONS_BLINK_FROG)
    {
        mon_msg( &menv[mid], "basks in the translocular energy." );
        heal_monster( &menv[mid], 1 + roll_zdice(2,4), true );
    }
    else if (one_chance_in(3))
    {
        if (mid != MHITYOU)
            mon_msg( &menv[mid], "Space bends around %s." );
        else
            mpr( "Your body is twisted painfully." );

        damage += 1 + roll_zdice(2,4);
    }
    else if (one_chance_in(3))
    {
        if (mid != MHITYOU)
            mon_msg( &menv[mid], "Space warps horribly around %s!" );
        else
            mpr( "Your body is terribly warped!" );

        damage += 1 + roll_dice(2,12);
    }
    else if (one_chance_in(3))
    {
        if (mid != MHITYOU)
            monster_blink( &menv[mid] );
        else
            random_blink(0);
    }
    else if (coinflip())
    {
        int delay = random2(5);

        if (mid != MHITYOU)
            monster_teleport( &menv[mid], delay );
        else if (!delay)
            you_teleport2( true, one_chance_in(5) );
        else
            you_teleport();
    }
    else if (coinflip())
    {
        if (mid != MHITYOU)
            monster_die( &menv[mid], KILL_RESET, killer );
        else
            banished( DNGN_ENTER_ABYSS );
    }

    if (apply_damage)
    {
        if (mid != MHITYOU)
        {
            hurt_monster_to_kill( &menv[mid], damage,
                                  (killer == MHITYOU) ? KILL_YOU : KILL_MON,
                                  killer );
        }
        else
        {
            ouch( damage, killer, KILLED_BY_MONSTER );
        }
    }

    return (damage);
}

// Really this is just applying the best of Band/Warp weapon/Warp field
// into a spell that gives the "make monsters go away" benefit without
// the insane damage potential.  -- bwr
int disperse_monsters( int x, int y, int pow, int message )
{
    UNUSED( message );

    const int mid = mgrd[x][y];

    if (mid == NON_MONSTER)
        return (0);

    struct monsters *const defender = &menv[mid];
    int ret = 0;

    if (defender->type == MONS_BLINK_FROG)
    {
        ret++;
        mon_msg(defender, " resists.");
    }
    else if (check_mons_resist_magic(defender, pow))
    {
        ret++;
        if (coinflip() && monster_blink( defender, false, true ))
            mon_msg(defender, " partially resists.");
        else
            mon_msg(defender, " resists.");

        return (1);
    }
    else
    {
        ret++;
        monster_teleport( defender );
    }

    return (ret);
}

void cast_dispersal(int pow)
{
    if (apply_area_around_square( disperse_monsters,
                                  you.x_pos, you.y_pos, pow ) == 0)
    {
        mpr( "There is a brief shimmering in the air around you." );
    }
}

static int spell_swap_func(int x, int y, int pow, int message)
{
    UNUSED( message );

    int mid = mgrd[x][y];

    if (mid == NON_MONSTER)
        return 0;

    struct monsters *const defender = &menv[mid];

    if (defender->type == MONS_BLINK_FROG
        || check_mons_resist_magic( defender, pow ))
    {
        mon_msg( defender, " resists." );
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
            move_player_to_grid( old_x, old_y, false, true, true );
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

            place_cloud( ctype, x, y, 1 + roll_zdice( 3, 1 + pow / 4 ) );
            return 1;
        }

        obj = next;
    }

    return 0;
}                               // end make_a_rot_cloud()

static int make_a_random_cloud( int x, int y, int pow, int ctype )
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
    place_cloud( cloud_material, x, y, roll_dice( 3, (pow / 4) ) );

    return 1;
}                               // end make_a_random_cloud()

int make_a_normal_cloud( int x, int y, int pow, int ctype )
{
    if (ctype == CLOUD_RANDOM)
        make_a_random_cloud( x, y, pow, CLOUD_GREY_SMOKE );
    else
        place_cloud( ctype, x, y, roll_dice( 3, 1 + pow / 4 ) );

    return 1;
}                               // end make_a_normal_cloud()

static int passwall( int x, int y, int pow, int garbage )
{
    UNUSED( garbage );

    const int depth = 2 + (you.skills[SK_EARTH_MAGIC] / 8) + random2(pow) / 25;

    if (grd[x][y] != DNGN_ROCK_WALL)
    {
        mpr("That's not a passable wall.");
        return (0);
    }

    const int dx = x - you.x_pos;
    const int dy = y - you.y_pos;

    // Note that the delay was (1 + howdeep * 2), but now that the
    // delay is stopped when the player is attacked it can be much
    // shorter since its harder to use for quick escapes. -- bwr
    // start_delay( DELAY_PASSWALL, 2 + howdeep, nx, ny );
    start_delay( DELAY_PASSWALL, depth, dx, dy );

    return (1);
}                               // end passwall()

void cast_passwall(int pow)
{
    apply_one_neighbouring_square(passwall, pow);
}                               // end cast_passwall()

static int intoxicate_monsters( int x, int y, int pow, int garbage )
{
    UNUSED( pow );
    UNUSED( garbage );

    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)
        return 0;

    if (mons_intel( menv[mon].type ) <= I_INSECT)
        return 0;

    if (mons_holiness( &menv[mon] ) != MH_NATURAL)
        return 0;

    if (mons_res_poison( &menv[mon] ) > 0)
        return 0;

    mons_add_ench( &menv[mon], ENCH_CONFUSION );
    return 1;
}                               // end intoxicate_monsters()

void cast_intoxicate( int pow )
{
    if (pow > 100)
        pow = 100;

    const bool res = (player_res_poison() > 0);

    // no saving throw for susceptible monsters -> needs disadvantage
    potion_effect( POT_CONFUSION, res ? 10 : 10 + (100 - pow) / 10 );

    if (!res && one_chance_in(20) && lose_stat( STAT_INTELLIGENCE, 1 ))
        mpr("Your head spins!");

    apply_area_visible( intoxicate_monsters, pow );
}                               // end cast_intoxicate()

// intended as a high-level Elven (a)bility
static int glamour_monsters( int x, int y, int pow, int garbage )
{
    UNUSED( garbage );

    if (mgrd[x][y] == NON_MONSTER)
        return (0);

    struct monsters *const mon = &menv[ mgrd[x][y] ];

    if (mons_intel( mon->type ) < I_NORMAL)
        return (0);

    if (mons_holiness( mon ) != MH_NATURAL)
        return (0);

    if (!mons_is_humanoid( mon->type ))
        return (0);

    if (!mons_player_visible( mon ))
        return (0);

    const int species = mons_species( mon->type );

    // orcs resist thru hatred of elves
    // elves resist cause they're elves
    // boggarts are malevolent highly magical wee-folk
    if (species == MONS_ORC || species == MONS_ELF || mon->type == MONS_BOGGART)
        pow = 1 + (pow * 3) / 4;

    if (check_mons_resist_magic( mon, pow ))
    {
        mon_msg( mon, " resists your wiles." );
        return (0);
    }

    switch (random2(6))
    {
    case 0:
        mons_add_ench( mon, ENCH_FEAR, MHITYOU );
        break;
    case 1:
        mons_add_ench( mon, ENCH_CONFUSION );
        break;
    case 4:
        mons_add_ench( mon, ENCH_STUN );
        break;
    case 2:
    case 5:
        mons_add_ench( mon, ENCH_CHARM );
        break;
    case 3:
        mon->behaviour = BEH_SLEEP;
        // to guarantee a turn or two
        if (mon->energy > 25)
            mon->energy -= (10 + random2(10));
        break;
    }

    // why no, there's no message as to which effect happened >:^)
    strcpy( info, ptr_monam(mon, DESC_CAP_THE) );

    switch (random2(4))
    {
    case 0:
        mon_msg( mon, " looks dazed." );
        break;
    case 1:
        mon_msg( mon, " blinks several times." );
        break;
    case 2:
        if (mon->type == MONS_CYCLOPS)
            mon_msg( mon, " rubs its eye." );
        else
            mon_msg( mon, " rubs its eyes." );
        break;
    case 3:
        mon_msg( mon, " tilts its head." );
        break;
    }

    return (1);
}                               // end glamour_monsters()

void cast_glamour( int pow )
{
    apply_area_visible( glamour_monsters, pow );
}                               // end cast_glamour()

bool backlight_monsters( int x, int y, int pow, int garbage )
{
    UNUSED( pow );
    UNUSED( garbage );

    int mc = mgrd[x][y];

    if (mc == NON_MONSTER)
        return (false);

    switch (menv[mc].type)
    {
    //case MONS_INSUBSTANTIAL_WISP: //jmf: I'm not sure if these glow or not
    //case MONS_VAPOUR:
    case MONS_UNSEEN_HORROR:    // consider making this visible? probably not.
        return (false);

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
        return (false);               // already glowing or invisible
    default:
        break;
    }

    struct monsters *const mon = &menv[mc];

    enchant_retval rval = mons_inc_ench_levels( mon, ENCH_BACKLIGHT, MHITYOU, 4 );

    if (rval == ERV_NEW)
        mon_msg( mon, " is outlined in light." );
    else if (rval == ERV_INCREASED)
        mon_msg( mon, " glows brighter." );

    // this enchantment wipes out invisibility (neat)
    if (rval != ERV_FAIL)
        mons_del_ench( mon, ENCH_INVIS );

    return (rval != ERV_FAIL);
}                               // end backlight_monsters()

int cast_evaporate(int pow)
{
    struct dist spelld;
    struct bolt beem;

    const int potion = prompt_invent_item( "Throw which potion?", OBJ_POTIONS );

    if (potion == -1)
    {
        mpr( "Wisps of steam play over your %s!", your_hand(true) );

        return (SPRET_ABORT);
    }
    else if (you.inv[potion].base_type != OBJ_POTIONS)
    {
        mpr( "This spell works only on potions!" );
        canned_msg(MSG_SPELL_FIZZLES);
        return (SPRET_ABORT);
    }

    mpr( MSGCH_PROMPT, STD_DIRECTION_PROMPT );

    message_current_target();

    direction( spelld, DIR_NONE, TARG_ENEMY );

    if (!spelld.isValid)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        return (SPRET_ABORT);
    }

    beem.target_x = spelld.tx;
    beem.target_y = spelld.ty;

    beem.source_x = you.x_pos;
    beem.source_y = you.y_pos;

    strcpy( beem.name, "potion" );
    beem.colour = you.inv[potion].colour;
    beem.range = 8;
    beem.rangeMax = 8;
    beem.type = SYM_FLASK;
    beem.beam_source = MHITYOU;
    beem.thrower = KILL_YOU_MISSILE;
    beem.aux_source = NULL;
    beem.is_beam = false;
    beem.is_enchant = false;
    beem.is_tracer = false;

    // XXX: need to fix this to call a throwing to-hit routine
    beem.hit = you.dex / 2 + roll_dice( 2, you.skills[SK_RANGED_COMBAT] / 2 + 1 );
    beem.damage = dice_def( 1, 0 );  // no damage, just producing clouds

    beem.ench_power = (pow > 50) ? 50 : pow;

    beem.flavour = BEAM_POTION_STINKING_CLOUD;

    switch (you.inv[potion].sub_type)
    {
    case POT_STRONG_POISON:
        beem.flavour = BEAM_POTION_POISON;
        beem.ench_power = (3 * beem.ench_power) / 2;
        break;

    case POT_DEGENERATION:
        beem.flavour = (coinflip() ? BEAM_POTION_POISON : BEAM_POTION_MIASMA);
        break;

    case POT_POISON:
        beem.flavour = BEAM_POTION_POISON;
        break;

    case POT_DECAY:
        beem.flavour = BEAM_POTION_MIASMA;
        beem.ench_power = (3 * beem.ench_power) / 2;
        break;

    case POT_PARALYSIS:
        beem.ench_power *= 2;
        // fall through
    case POT_CONFUSION:
    case POT_SLOWING:
        beem.flavour = BEAM_POTION_STINKING_CLOUD;
        break;

    case POT_WATER:
    case POT_PORRIDGE:
    case POT_CURE_MUTATION:
    case POT_RESTORE_ABILITIES:
        beem.flavour = BEAM_POTION_STEAM;
        break;

    case POT_BERSERK_RAGE:
        beem.flavour = (coinflip() ? BEAM_POTION_FIRE : BEAM_POTION_STEAM);
        break;

    case POT_MUTATION:
        beem.flavour = BEAM_POTION_RANDOM;
        break;

    case POT_GAIN_STRENGTH:
    case POT_GAIN_DEXTERITY:
    case POT_GAIN_INTELLIGENCE:
    case POT_EXPERIENCE:
    case POT_MAGIC:
        switch (random2(8))
        {
        case 0:   beem.flavour = BEAM_POTION_FIRE;            break;
        case 1:   beem.flavour = BEAM_POTION_COLD;            break;
        case 2:   beem.flavour = BEAM_POTION_POISON;          break;
        case 3:   beem.flavour = BEAM_POTION_MIASMA;          break;
        default:  beem.flavour = BEAM_POTION_RANDOM;          break;
        }
        break;

    default:
        switch (random2(6))
        {
        case 0:   beem.flavour = BEAM_POTION_STINKING_CLOUD;  break;
        case 1:   beem.flavour = BEAM_POTION_RANDOM;          break;
        case 2:   beem.flavour = BEAM_POTION_BLUE_SMOKE;      break;
        case 3:   beem.flavour = BEAM_POTION_BLACK_SMOKE;     break;
        case 4:   beem.flavour = BEAM_POTION_PURP_SMOKE;      break;
        default:  beem.flavour = BEAM_POTION_STEAM;           break;
        }
        break;
    }

    if (coinflip())
        exercise( SK_RANGED_COMBAT, 1 );

    fire_beam(beem);

    // both old and new code use up a potion:
    dec_inv_item_quantity( potion, 1 );

    return (SPRET_SUCCESS);
}                               // end cast_evaporate()

// The intent of this spell isn't to produce helpful potions
// for drinking, but rather to provide ammo for the Evaporate
// spell out of corpses, thus potentially making it useful.
// Producing helpful potions would break game balance here...
// and producing more than one potion from a corpse, or not
// using up the corpse might also lead to game balance problems. -- bwr
int cast_fulsome_distillation( int powc )
{
    char str_pass[ ITEMNAME_SIZE ];

    if (powc > 50)
        powc = 50;

    int corpse = -1;

    // Search items at the players location for corpses.
    // XXX: Turn this into a separate function and merge with
    // the messes over in butchery, animating, and maybe even
    // item pickup from stacks (which would make it easier to
    // create a floor stack menu system later) -- bwr
    for (int curr_item = igrd[you.x_pos][you.y_pos];
             curr_item != NON_ITEM;
             curr_item = mitm[curr_item].link)
    {
        if (mitm[curr_item].base_type == OBJ_CORPSES
            && mitm[curr_item].sub_type == CORPSE_BODY)
        {
            it_name( curr_item, DESC_NOCAP_THE, str_pass );
            snprintf( info, INFO_SIZE, "Distill a potion from %s?", str_pass );

            if (yesno( info, true, false ))
            {
                corpse = curr_item;
                break;
            }
        }
    }

    if (corpse == -1)
    {
        canned_msg( MSG_SPELL_FIZZLES );
        return (SPRET_ABORT);
    }

    // Need to be at least 3 HD to have a chance of qualifying...
    // This used to be a flat >= 5 check, but that makes a lot
    // of the special cases pointless. -- bwr
    const int hit_dice = mons_class_hit_dice( mitm[corpse].plus );
    const bool big_monster = (random2( hit_dice + 5 ) >= 7);

    const bool rotten = (mitm[corpse].special < 100);
    const bool power_up = (rotten && big_monster);

    if (random2(60) > 5 + hit_dice + powc)
    {
        mpr( "You fail to produce a viable potion from the corpse." );
        return (SPRET_FAIL);
    }

    int potion_type = POT_WATER;

    switch (mitm[corpse].plus)
    {
    case MONS_GIANT_BAT:             // extracting batty behaviour : 1 HD
    case MONS_UNSEEN_HORROR:         // extracting batty behaviour : 7 HD
    case MONS_GIANT_BLOWFLY:         // extracting batty behaviour : 5 HD
        potion_type = POT_CONFUSION;
        break;

    case MONS_RED_WASP:              // paralysis attack : 8
    case MONS_YELLOW_WASP:           // paralysis attack : 4
        potion_type = POT_PARALYSIS;
        break;

    case MONS_SNAKE:                 // clean meat, but poisonous attack : 2
    case MONS_GIANT_ANT:             // clean meat, but poisonous attack : 3
        potion_type = (power_up ? POT_POISON : POT_CONFUSION);
        break;

    case MONS_ORANGE_RAT:            // poisonous meat, but draining attack : 3
        potion_type = (power_up ? POT_DECAY : POT_POISON);
        break;

    case MONS_SPINY_WORM:            // 12
        potion_type = (power_up ? POT_DECAY : POT_STRONG_POISON);
        break;

    default:
        switch (mons_corpse_effect( mitm[corpse].plus ))
        {
        case CE_CLEAN:
            potion_type = (power_up ? POT_CONFUSION : POT_WATER);
            break;

        case CE_CONTAMINATED:
            potion_type = (power_up ? POT_DEGENERATION : POT_POISON);
            break;

        case CE_POISONOUS:
            potion_type = (power_up ? POT_STRONG_POISON : POT_POISON);
            break;

        case CE_MUTAGEN_RANDOM:
        case CE_MUTAGEN_GOOD:   // unused
        case CE_RANDOM:         // unused
            potion_type = POT_MUTATION;
            break;

        case CE_MUTAGEN_BAD:    // unused
        case CE_ROTTEN:         // actually this only occurs via mangling
        case CE_HCL:            // necrophage
            potion_type = (power_up ? POT_DECAY : POT_STRONG_POISON);
            break;

        case CE_NOCORPSE:       // shouldn't occur
        default:
            break;
        }
        break;
    }

    // If not powerful enough, we downgrade the potion
    if (random2(80) > powc + 10 * rotten)
    {
        switch (potion_type)
        {
        case POT_DECAY:
        case POT_DEGENERATION:
        case POT_STRONG_POISON:
            potion_type = POT_POISON;
            break;

        case POT_MUTATION:
        case POT_POISON:
            potion_type = POT_CONFUSION;
            break;

        case POT_PARALYSIS:
            potion_type = POT_SLOWING;
            break;

        case POT_CONFUSION:
        case POT_SLOWING:
        default:
            potion_type = POT_WATER;
            break;
        }
    }

    // We borrow the corpse's object to make our potion:
    mitm[corpse].base_type = OBJ_POTIONS;
    mitm[corpse].sub_type = potion_type;
    mitm[corpse].quantity = 1;
    mitm[corpse].plus = 0;
    mitm[corpse].plus2 = 0;
    item_colour( mitm[corpse] );  // sets special as well

    it_name( corpse, DESC_NOCAP_A, str_pass );
    mpr( "You extract %s from the corpse.", str_pass );

    // try to move the potion to the player (for convenience)
    if (move_item_to_player( corpse, 1 ) != 1)
        mpr( "Unfortunately, you can't carry it right now!" );

    return (SPRET_SUCCESS);
}

static int rot_living( int x, int y, int pow, int message )
{
    UNUSED( message );

    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)
        return (0);

    if (!mons_has_lifeforce( &menv[mon] ))
        return (0);

    if (check_mons_resist_magic( &menv[mon], pow ))
        return (0);

    int lvl = ((random2(pow) + random2(pow) + random2(pow) + random2(pow)) / 4);

    if (lvl >= 50)
        lvl = 4;
    else if (lvl >= 35)
        lvl = 3;
    else if (lvl >= 20)
        lvl = 2;
    else
        lvl = 1;

    mons_add_ench( &menv[mon], ENCH_ROT, MHITNOT, -1, lvl );

    return (1);
}                               // end rot_living()

static int rot_undead( int x, int y, int pow, int garbage )
{
    UNUSED( garbage );

    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)
        return (0);

    if (mons_holiness( &menv[mon] ) != MH_UNDEAD)
        return (0);

    if (check_mons_resist_magic( &menv[mon], pow ))
        return (0);

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
        return (0);               // immune (no flesh) or already rotting
    }

    int lvl = ((random2(pow) + random2(pow) + random2(pow) + random2(pow)) / 4);

    if (lvl >= 50)
        lvl = 4;
    else if (lvl >= 35)
        lvl = 3;
    else if (lvl >= 20)
        lvl = 2;
    else
        lvl = 1;

    mons_add_ench( &menv[mon], ENCH_ROT, MHITNOT, -1, lvl );

    return (1);
}                               // end rot_undead()

static int rot_corpses(int x, int y, int pow, int garbage)
{
    UNUSED( garbage );

    return make_a_rot_cloud(x, y, pow, CLOUD_MIASMA);
}                               // end rot_corpses()

void cast_rotting(int pow)
{
    apply_area_visible(rot_living, pow);
    apply_area_visible(rot_undead, pow);
    apply_area_visible(rot_corpses, pow);
    return;
}                               // end cast_rotting()

void do_monster_rot( int mid )
{
    int damage = 1 + random2(3);

    if (mons_holiness( &menv[mid] ) == MH_UNDEAD && !one_chance_in(5))
    {
        apply_area_cloud( make_a_normal_cloud, menv[mid].x, menv[mid].y, 5, 1,
                          CLOUD_MIASMA );
    }

    you_hurt_monster( &menv[mid], damage );
    return;
}                               // end do_monster_rot()

static int snake_charm_monsters(int x, int y, int pow, int message)
{
    UNUSED( message );

    int mon = mgrd[x][y];

    if (mon == NON_MONSTER)                             return 0;
    if (mons_friendly(&menv[mon]))                      return 0;
    if (one_chance_in(4))                               return 0;
    if (mons_char(menv[mon].type) != 'S')               return 0;
    if (check_mons_resist_magic(&menv[mon], pow))       return 0;

    menv[mon].attitude = ATT_FRIENDLY;

    mon_msg( &(menv[mon]), " sways back and forth." );

    return (1);
}

void cast_snake_charm(int pow)
{
    // powc = (you.xp_level * 2) + (you.skills[SK_INVOCATIONS] * 3);
    apply_one_neighbouring_square( snake_charm_monsters, pow );
}

int cast_fragmentation(int pow)        // jmf: ripped idea from airstrike
{
    struct dist beam;
    struct bolt blast;
    int debris = 0;
    int trap;
    bool explode = false;
    bool hole = true;
    const char *what = NULL;

    mpr( MSGCH_PROMPT,"Fragment what (e.g. a wall)?" );
    direction( beam, DIR_TARGET, TARG_ENEMY );

    if (!beam.isValid)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        return (SPRET_ABORT);
    }

    //FIXME: if (player typed '>' to attack floor) goto do_terrain;
    blast.beam_source = MHITYOU;
    blast.thrower = KILL_YOU;
    blast.aux_source = NULL;
    blast.ex_size = 1;              // default
    blast.type = '#';
    blast.colour = 0;
    blast.target_x = beam.tx;
    blast.target_y = beam.ty;
    blast.is_tracer = false;
    blast.flavour = BEAM_FRAG;

    // Number of dice vary... 3 is easy/common, but it can get as high as 6.
    blast.damage = dice_def( 0, 5 + pow / 10 );

    const int grid = grd[beam.tx][beam.ty];
    const int mon  = mgrd[beam.tx][beam.ty];

    const bool okay_to_dest = in_bounds( beam.tx, beam.ty );

    if (mon != NON_MONSTER)
    {
        // This needs its own hand_buff... we also need to do it first
        // in case the target dies. -- bwr
        char explode_msg[80];

        snprintf( explode_msg, sizeof( explode_msg ), "%s explodes!",
                  ptr_monam( &menv[mon], DESC_CAP_THE ) );

        switch (menv[mon].type)
        {
        case MONS_ICE_BEAST: // blast of ice fragments
        case MONS_SIMULACRUM_SMALL:
        case MONS_SIMULACRUM_LARGE:
            explode = true;
            strcpy(blast.name, "icy blast");
            blast.colour = WHITE;
            blast.damage.num = 2;
            blast.flavour = BEAM_ICE;

            if (you_hurt_monster( &menv[mon], roll_dice( blast.damage ) ))
                blast.damage.num += 1;
            break;

        case MONS_FLYING_SKULL:
        case MONS_SKELETON_SMALL:
        case MONS_SKELETON_LARGE:       // blast of bone
            explode = true;

            snprintf( info, INFO_SIZE, "The sk%s explodes into sharp fragments of bone!",
                    (menv[mon].type == MONS_FLYING_SKULL) ? "ull" : "eleton");

            strcpy(blast.name, "blast of bone shards");

            blast.colour = LIGHTGREY;

            if (random2(50) < (pow / 5))        // potential insta-kill
            {
                monster_die(&menv[mon], KILL_YOU, 0);
                blast.damage.num = 4;
            }
            else
            {
                blast.damage.num = 2;

                if (you_hurt_monster( &menv[mon], roll_dice( blast.damage ) ))
                    blast.damage.num = 4;
            }
            goto all_done;      // i.e. no "Foo Explodes!"

        case MONS_WOOD_GOLEM:
            explode = false;
            mon_msg(&menv[mon], " shudders violently!");

            // We use blast.damage not only for inflicting damage here,
            // but so that later on we'll know that the spell didn't
            // fizzle (since we don't actually explode wood golems). -- bwr
            blast.damage.num = 2;
            you_hurt_monster( &menv[mon], roll_dice( blast.damage ) );
            break;

        case MONS_IRON_GOLEM:
        case MONS_METAL_GARGOYLE:
            explode = true;
            strcpy( blast.name, "blast of metal fragments" );
            blast.colour = CYAN;
            blast.damage.num = 4;

            if (you_hurt_monster( &menv[mon], roll_dice( blast.damage ) ))
                blast.damage.num += 2;
            break;

        case MONS_CLAY_GOLEM:   // assume baked clay and not wet loam
        case MONS_STONE_GOLEM:
        case MONS_EARTH_ELEMENTAL:
        case MONS_GARGOYLE:
            explode = true;
            blast.ex_size = 2;
            strcpy(blast.name, "blast of rock fragments");
            blast.colour = BROWN;
            blast.damage.num = 3;

            if (you_hurt_monster( &menv[mon], roll_dice( blast.damage ) ))
                blast.damage.num += 1;
            break;

        case MONS_CRYSTAL_GOLEM:
            explode = true;
            blast.ex_size = 2;
            strcpy(blast.name, "blast of crystal shards");
            blast.colour = WHITE;
            blast.damage.num = 4;

            if (you_hurt_monster( &menv[mon], roll_dice( blast.damage ) ))
                blast.damage.num += 2;
            break;

        default:
            blast.damage.num = 1;  // to mark that a monster was targetted

            // Yes, this spell does lousy damage if the
            // monster isn't susceptable. -- bwr
            you_hurt_monster( &menv[mon], roll_dice( 1, 5 + pow / 25 ) );
            goto do_terrain;
        }

        mpr( explode_msg );
        goto all_done;
    }

  do_terrain:
    // FIXME: do nothing in Abyss & Pandemonium?

    switch (grid)
    {
    //
    // Stone and rock terrain
    //
    case DNGN_ROCK_WALL:
    case DNGN_SECRET_DOOR:
        blast.colour = env.rock_colour;
        // fall-through
    case DNGN_STONE_WALL:
        what = "wall";
        if (player_in_branch( BRANCH_HALL_OF_ZOT ))
            blast.colour = env.rock_colour;
        // fall-through
    case DNGN_ORCISH_IDOL:
        if (what == NULL)
            what = "stone idol";
        if (blast.colour == 0)
            blast.colour = DARKGREY;
        // fall-through
    case DNGN_GRANITE_STATUE:   // normal rock -- big explosion
        if (what == NULL)
            what = "statue";

        explode = true;

        strcpy(blast.name, "blast of rock fragments");
        blast.damage.num = 3;
        if (blast.colour == 0)
            blast.colour = LIGHTGREY;

        if (okay_to_dest
            && (grid == DNGN_ORCISH_IDOL
                || grid == DNGN_GRANITE_STATUE
                || (pow >= 40 && grid == DNGN_ROCK_WALL && one_chance_in(3))
                || (pow >= 60 && grid == DNGN_STONE_WALL && one_chance_in(10))))
        {
            // terrain blew up real good:
            blast.ex_size = 2;
            grd[beam.tx][beam.ty] = DNGN_FLOOR;
            debris = DEBRIS_ROCK;
        }
        break;

    //
    // Metal -- small but nasty explosion
    //

    case DNGN_METAL_WALL:
        what = "metal wall";
        blast.colour = CYAN;
        // fallthru
    case DNGN_SILVER_STATUE:
        if (what == NULL)
        {
            what = "silver statue";
            blast.colour = WHITE;
        }

        explode = true;
        strcpy( blast.name, "blast of metal fragments" );
        blast.damage.num = 4;

        if (okay_to_dest && pow >= 80 && random2(500) < pow / 5)
        {
            blast.damage.num += 2;
            grd[beam.tx][beam.ty] = DNGN_FLOOR;
            debris = DEBRIS_METAL;
        }
        break;

    //
    // Crystal
    //

    case DNGN_GREEN_CRYSTAL_WALL:       // crystal -- large & nasty explosion
        what = "crystal wall";
        blast.colour = GREEN;
        // fallthru
    case DNGN_ORANGE_CRYSTAL_STATUE:
        if (what == NULL)
        {
            what = "crystal statue";
            blast.colour = LIGHTRED; //jmf: == orange, right?
        }

        explode = true;
        blast.ex_size = 2;
        strcpy(blast.name, "blast of crystal shards");
        blast.damage.num = 5;

        if (okay_to_dest
            && ((grid == DNGN_GREEN_CRYSTAL_WALL && coinflip())
                || (grid == DNGN_ORANGE_CRYSTAL_STATUE
                    && pow >= 50 && one_chance_in(10))))
        {
            blast.ex_size = coinflip() ? 3 : 2;
            grd[beam.tx][beam.ty] = DNGN_FLOOR;
            debris = DEBRIS_CRYSTAL;
        }
        break;

    //
    // Traps
    //

    case DNGN_UNDISCOVERED_TRAP:
    case DNGN_TRAP_MECHANICAL:
        trap = trap_at_xy( beam.tx, beam.ty );

        if (trap != -1)
        {
            // non-mechanical traps don't explode with this spell -- bwr
            if (trap_category(env.trap[trap].type) != DNGN_TRAP_MECHANICAL)
                break;
        }

        // undiscovered traps appear as exploding from the floor -- bwr
        what = ((grid == DNGN_UNDISCOVERED_TRAP) ? "floor" : "trap");

        explode = true;
        hole = false;           // to hit monsters standing on traps
        strcpy( blast.name, "blast of fragments" );
        blast.colour = env.floor_colour;  // in order to blend in
        blast.damage.num = 2;

        // Exploded traps are nonfunctional, ammo is also ruined -- bwr
        if (okay_to_dest)
            remove_trap( trap );
        break;

    //
    // Stone doors and arches
    //

    case DNGN_OPEN_DOOR:
    case DNGN_CLOSED_DOOR:
        // Doors always blow up, stone arches never do (would cause problems)
        if (okay_to_dest)
            grd[beam.tx][beam.ty] = DNGN_FLOOR;

        // fall-through
    case DNGN_STONE_ARCH:       // floor -- small explosion
        explode = true;
        hole = false;           // to hit monsters standing on doors
        strcpy( blast.name, "blast of rock fragments" );
        blast.colour = LIGHTGREY;
        blast.damage.num = 2;
        break;

    //
    // Permarock and floor are unaffected -- bwr
    //
    case DNGN_PERMAROCK_WALL:
    case DNGN_FLOOR:
        explode = false;
        snprintf( info, INFO_SIZE, "%s seems to be unnaturally hard.",
                  (grid == DNGN_PERMAROCK_WALL) ? "That wall"
                                                : "The dungeon floor" );
        explode = false;
        break;

    case DNGN_TRAP_III: // What are these? Should they explode? -- bwr
    default:
        // FIXME: cute message for water?
        break;
    }

  all_done:
    if (explode && blast.damage.num > 0)
    {
        if (what != NULL)
            mpr( "The %s explodes!", what );

        explosion( blast, hole );
    }
    else if (blast.damage.num == 0)
    {
        // if damage dice are zero we assume that nothing happened at all.
        canned_msg(MSG_SPELL_FIZZLES);
    }

    if (debris)
        place_debris(beam.tx, beam.ty, debris);

    return (SPRET_SUCCESS);
}                               // end cast_fragmentation()

int cast_apportation(int pow)
{
    struct dist beam;

    mpr("Pull items from where?");

    direction( beam, DIR_TARGET );

    if (!beam.isValid)
    {
        canned_msg(MSG_SPELL_FIZZLES);
        return (SPRET_ABORT);
    }

    // it's already here!
    if (beam.isMe)
    {
        mpr( "That's just silly." );
        return (SPRET_ABORT);
    }

    // Protect the player from destroying the item
    const int grid = grd[ you.x_pos ][ you.y_pos ];

    if (grid_destroys_items( grid ))
    {
        mpr( "That would be silly while over this terrain!" );
        return (SPRET_ABORT);
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
        return (SPRET_ABORT);
    }

    // Let's look at the top item in that square...
    const int item = igrd[beam.tx][beam.ty];
    if (item == NON_ITEM)
    {
        const int  mon = mgrd[beam.tx][beam.ty];

        if (mon == NON_MONSTER || !player_monster_visible( &menv[mon] ))
            mpr( "There are no items there." );
        else if (mons_is_mimic( menv[mon].type ))
        {
            if (!shift_monster( &menv[mon], you.x_pos, you.y_pos ))
                mon_msg( &(menv[mon]), " twitches." );
        }
        else
            mpr( "This spell does not work on creatures." );

        return (SPRET_FAIL);
    }

    // mass of one unit
    const int unit_mass = item_mass( mitm[item] );
    // assume we can pull everything
    int max_units = mitm[item].quantity;

    // item has mass: might not move all of them
    if (unit_mass > 0)
    {
        const int max_mass = pow * 30 + random2( pow * 20 );

        // most units our power level will allow
        max_units = max_mass / unit_mass;
    }

    if (max_units <= 0)
    {
        mpr( "The mass is resisting your pull." );
        return (SPRET_FAIL);
    }

    // Failure should never really happen after all the above checking,
    // but we'll handle it anyways...
    if (!move_top_item( beam.tx, beam.ty, you.x_pos, you.y_pos ))
    {
        mpr( "The spell fails." );
        return (SPRET_FAIL);
    }

    if (max_units < mitm[item].quantity)
    {
        mitm[item].quantity = max_units;
        mpr( "You feel that some mass got lost in the cosmic void." );
    }
    else
    {
        if (mitm[item].base_type == OBJ_ORBS
            || (mitm[item].base_type == OBJ_MISCELLANY
                && mitm[item].sub_type == MISC_RUNE_OF_ZOT))
        {
            mpr( "Yoink!" );
        }

        mpr( "You pull the item%s to yourself.",
             (mitm[ item ].quantity > 1) ? "s" : "" );
    }

    return (SPRET_SUCCESS);
}

void cast_sandblast( int pow, struct bolt &beam )
{
    bool big = false;

    const int stones = get_inv_in_hand();

    if (stones != -1)
    {
        if (you.inv[stones].base_type == OBJ_MISSILES
            && (you.inv[stones].sub_type == MI_STONE
                || you.inv[stones].sub_type == MI_LARGE_ROCK))
        {
            big = true;

            if (you.inv[stones].sub_type == MI_LARGE_ROCK)
                pow = 50;  // value capped at 50
        }
    }

    if (!big)
        zapping( ZAP_SMALL_SANDBLAST, pow, beam );
    else
    {
        dec_inv_item_quantity( stones, 1 );
        zapping( ZAP_SANDBLAST, pow, beam );
    }
}                               // end cast_sandblast()

void cast_condensation_shield(int pow)
{
    if (you.equip[EQ_SHIELD] != -1 || you.fire_shield)
        canned_msg(MSG_SPELL_FIZZLES);
    else
    {
        if (you.duration[DUR_CONDENSATION_SHIELD] > 0)
            you.duration[DUR_CONDENSATION_SHIELD] += 5 + roll_dice(2, 3);
        else
        {
            mpr("A crackling disc of dense vapour forms in the air!");
            set_redraw_status( REDRAW_ARMOUR_CLASS );

            you.duration[DUR_CONDENSATION_SHIELD] = 10 + roll_dice(2, pow / 5);
        }

        if (you.duration[DUR_CONDENSATION_SHIELD] > 30)
            you.duration[DUR_CONDENSATION_SHIELD] = 30;
    }

    return;
}                               // end cast_condensation_shield()

void cast_stoneskin(int pow)
{
    if (you.is_undead)
    {
        mpr("This spell does not affect your undead flesh.");
        return;
    }

    if (you.attribute[ATTR_TRANSFORMATION] != TRAN_NONE
        && you.attribute[ATTR_TRANSFORMATION] != TRAN_STATUE
        && you.attribute[ATTR_TRANSFORMATION] != TRAN_BLADE_HANDS)
    {
        mpr("This spell does not affect your current form.");
        return;
    }

    if (you.duration[DUR_STONEMAIL] || you.duration[DUR_ICY_ARMOUR])
    {
        mpr("This spell conflicts with another spell still in effect.");
        return;
    }

    if (you.duration[DUR_STONESKIN])
        mpr( "Your skin feels harder." );
    else
    {
        if (you.attribute[ATTR_TRANSFORMATION] == TRAN_STATUE)
            mpr( "Your stone body feels more resilient." );
        else
            mpr( "Your skin hardens." );

        set_redraw_status( REDRAW_ARMOUR_CLASS );
    }

    you.duration[DUR_STONESKIN] += 10 + random2(pow) + random2(pow);

    if (you.duration[DUR_STONESKIN] > 50)
        you.duration[DUR_STONESKIN] = 50;
}
