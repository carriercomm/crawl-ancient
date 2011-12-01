/*
 *  File:       itemprop.cc
 *  Summary:    Misc functions.
 *  Written by: Brent Ross
 *
 *  Change History (most recent first):
 *
 *      <1>      -/--/--        BWR             Created
 */

#include "AppHdr.h"
#include "itemname.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef DOS
#include <conio.h>
#endif

#include "globals.h"
#include "externs.h"

#include "items.h"
#include "itemprop.h"
#include "macro.h"
#include "mon-util.h"
#include "player.h"
#include "randart.h"
#include "skills2.h"
#include "stuff.h"
#include "transfor.h"
#include "view.h"


// XXX: name strings in most of the following are currently unused!
struct armour_def
{
    armour_type         id;
    const char         *name;
    int                 ac;
    int                 ev;
    int                 mass;

    bool                light;
    equipment_type      slot;
    size_type           fit_min;
    size_type           fit_max;
};

// Note: the Little-Giant range is used to make armours which are very
// flexible and adjustable and can be worn by any player character...
// providing they also pass the shape test, of course.
static int Armour_index[NUM_ARMOURS];
static armour_def Armour_prop[NUM_ARMOURS] =
{
    { ARM_ANIMAL_SKIN,          "animal skin",            2,  0,  100,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_ROBE,                 "robe",                   2,  0,   60,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_BIG },
    { ARM_LEATHER_ARMOUR,       "leather armour",         3, -1,  150,
        true,  EQ_BODY_ARMOUR, SIZE_SMALL,  SIZE_MEDIUM },
    { ARM_STUDDED_LEATHER_ARMOUR,"studded leather armour",4, -1,  180,
        true,  EQ_BODY_ARMOUR, SIZE_SMALL,  SIZE_MEDIUM },

    { ARM_RING_MAIL,            "ring mail",              4, -2,  250,
        false, EQ_BODY_ARMOUR, SIZE_SMALL,  SIZE_MEDIUM },
    { ARM_SCALE_MAIL,           "scale mail",             5, -3,  350,
        false, EQ_BODY_ARMOUR, SIZE_SMALL,  SIZE_MEDIUM },
    { ARM_CHAIN_MAIL,           "chain mail",             6, -4,  400,
        false, EQ_BODY_ARMOUR, SIZE_SMALL,  SIZE_MEDIUM },
    { ARM_BANDED_MAIL,          "banded mail",            7, -5,  500,
        false, EQ_BODY_ARMOUR, SIZE_MEDIUM, SIZE_MEDIUM },
    { ARM_SPLINT_MAIL,          "splint mail",            8, -5,  550,
        false, EQ_BODY_ARMOUR, SIZE_MEDIUM, SIZE_MEDIUM },
    { ARM_PLATE_MAIL,           "plate mail",            10, -6,  650,
        false, EQ_BODY_ARMOUR, SIZE_MEDIUM, SIZE_MEDIUM },
    { ARM_CRYSTAL_PLATE_MAIL,   "crystal plate mail",    14, -8, 1200,
        false, EQ_BODY_ARMOUR, SIZE_MEDIUM, SIZE_MEDIUM },

    { ARM_TROLL_HIDE,           "troll hide",             2, -1,  220,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_TROLL_LEATHER_ARMOUR, "troll leather armour",   4, -1,  220,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_STEAM_DRAGON_HIDE,    "steam dragon hide",      2,  0,  120,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_STEAM_DRAGON_ARMOUR,  "steam dragon armour",    4,  0,  120,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_MOTTLED_DRAGON_HIDE,  "mottled dragon hide",    2, -1,  150,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_MOTTLED_DRAGON_ARMOUR,"mottled dragon armour",  5, -1,  150,
        true,  EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },

    { ARM_SWAMP_DRAGON_HIDE,    "swamp dragon hide",      3, -2,  200,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_SWAMP_DRAGON_ARMOUR,  "swamp dragon armour",    5, -2,  200,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_DRAGON_HIDE,          "dragon hide",            3, -3,  350,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_DRAGON_ARMOUR,        "dragon armour",          6, -3,  350,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_ICE_DRAGON_HIDE,      "ice dragon hide",        3, -3,  350,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_ICE_DRAGON_ARMOUR,    "ice dragon armour",      6, -3,  350,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_STORM_DRAGON_HIDE,    "storm dragon hide",      4, -5,  600,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_STORM_DRAGON_ARMOUR,  "storm dragon armour",    8, -5,  600,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_GOLD_DRAGON_HIDE,     "gold dragon hide",       5, -8, 1100,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },
    { ARM_GOLD_DRAGON_ARMOUR,   "gold dragaon armour",   10, -8, 1100,
        false, EQ_BODY_ARMOUR, SIZE_LITTLE, SIZE_GIANT },

    { ARM_CLOAK,                "cloak",                  1,  0,   40,
        true,  EQ_CLOAK,       SIZE_LITTLE, SIZE_BIG },
    { ARM_GLOVES,               "gloves",                 1,  0,   20,
        true,  EQ_GLOVES,      SIZE_SMALL,  SIZE_MEDIUM },

    { ARM_HELMET,               "helmet",                 1,  0,   80,
        false, EQ_HELMET,      SIZE_SMALL,  SIZE_MEDIUM },
    { ARM_CAP,                  "cap",                    0,  0,   40,
        true,  EQ_HELMET,      SIZE_LITTLE, SIZE_LARGE },

    // Note that barding size is compared against torso so it currently
    // needs to fit medium, but that doesn't matter as much as race
    // and shapeshift status.
    { ARM_BOOTS,                "boots",                  1,  0,   30,
        true,  EQ_BOOTS,       SIZE_SMALL,  SIZE_MEDIUM },
    { ARM_CENTAUR_BARDING,      "centaur barding",        4, -2,  100,
        true,  EQ_BOOTS,       SIZE_MEDIUM, SIZE_MEDIUM },
    { ARM_NAGA_BARDING,         "naga barding",           4, -2,  100,
        true,  EQ_BOOTS,       SIZE_MEDIUM, SIZE_MEDIUM },

    // Note: shields use ac-value as sh-value, EV pen is used for heavy_shield
    { ARM_BUCKLER,              "buckler",                3,  0,   90,
        true,  EQ_SHIELD,      SIZE_LITTLE, SIZE_MEDIUM },
    { ARM_SHIELD,               "shield",                 5, -1,  150,
        false, EQ_SHIELD,      SIZE_SMALL,  SIZE_BIG    },
    { ARM_LARGE_SHIELD,         "large shield",           7, -2,  230,
        false, EQ_SHIELD,      SIZE_MEDIUM, SIZE_GIANT  },
};

struct weapon_def
{
    int                 id;
    const char         *name;
    int                 dam;
    int                 hit;
    int                 speed;
    int                 mass;
    int                 str_weight;

    skill_type          skill;
    hands_reqd_type     hands;
    size_type           fit_size;     // actual size is one size smaller
    missile_type        ammo;         // MI_NONE for non-launchers
    bool                throwable;

    int                 dam_type;
};

static int Weapon_index[NUM_WEAPONS];
static weapon_def Weapon_prop[NUM_WEAPONS] =
{
    // Maces & Flails
    { WPN_WHIP,              "whip",                4,  2, 13,  30,  2,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLASHING },
    { WPN_CLUB,              "club",                5,  3, 13,  50,  7,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_SMALL,  MI_NONE, true,
        DAMV_CRUSHING },
    { WPN_HAMMER,            "hammer",              7,  3, 13,  90,  7,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_SMALL,  MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_MACE,              "mace",                8,  3, 14, 120,  8,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_SMALL,  MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_FLAIL,             "flail",               9,  2, 15, 130,  8,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_SMALL,  MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_ANCUS,             "ancus",               9,  2, 14, 120,  8,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_MORNINGSTAR,       "morningstar",        10, -2, 15, 140,  8,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_PIERCING | DAM_BLUDGEON },
    { WPN_DEMON_WHIP,        "demon whip",         10,  2, 13,  30,  2,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLASHING },
    { WPN_SPIKED_FLAIL,      "spiked flail",       12, -3, 16, 190,  8,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_PIERCING | DAM_BLUDGEON },
    { WPN_EVENINGSTAR,       "eveningstar",        12, -2, 15, 180,  8,
        SK_MACES_FLAILS, HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_PIERCING | DAM_BLUDGEON },
    { WPN_DIRE_FLAIL,        "dire flail",         13, -6, 14, 240,  9,
        SK_MACES_FLAILS, HANDS_DOUBLE, SIZE_MEDIUM, MI_NONE, false,
        DAMV_PIERCING | DAM_BLUDGEON },
    { WPN_GREAT_MACE,        "great mace",         18, -5, 19, 270,  9,
        SK_MACES_FLAILS, HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_GIANT_CLUB,        "giant club",         19, -8, 20, 330, 10,
        SK_MACES_FLAILS, HANDS_TWO,    SIZE_BIG,    MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_GIANT_SPIKED_CLUB, "giant spiked club",  20, -9, 21, 350, 10,
        SK_MACES_FLAILS, HANDS_TWO,    SIZE_BIG,    MI_NONE, false,
        DAMV_PIERCING | DAM_BLUDGEON },

    // Short blades
    { WPN_KNIFE,             "knife",               3,  5, 10,  10,  1,
        SK_SHORT_BLADES, HANDS_ONE,    SIZE_LITTLE, MI_NONE, false,
        DAMV_STABBING | DAM_SLICE },
    { WPN_DAGGER,            "dagger",              4,  6, 10,  20,  1,
        SK_SHORT_BLADES, HANDS_ONE,    SIZE_LITTLE, MI_NONE, true,
        DAMV_STABBING | DAM_SLICE },
    { WPN_QUICK_BLADE,       "quick blade",         5,  6,  8,  50,  0,
        SK_SHORT_BLADES, HANDS_ONE,    SIZE_LITTLE, MI_NONE, false,
        DAMV_STABBING | DAM_SLICE },
    { WPN_SHORT_SWORD,       "short sword",         6,  4, 11,  80,  2,
        SK_SHORT_BLADES, HANDS_ONE,    SIZE_SMALL,  MI_NONE, false,
        DAMV_SLICING | DAM_PIERCE },
    { WPN_SABRE,             "sabre",               7,  4, 11,  90,  2,
        SK_SHORT_BLADES, HANDS_ONE,    SIZE_SMALL,  MI_NONE, false,
        DAMV_SLICING | DAM_PIERCE },

    // Long swords
    { WPN_FALCHION,          "falchion",            8,  2, 13, 170,  4,
        SK_LONG_SWORDS,  HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },  // or perhaps DAMV_CHOPPING is more apt?
    { WPN_LONG_SWORD,        "long sword",         10, -2, 14, 160,  3,
        SK_LONG_SWORDS,  HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },
    { WPN_SCIMITAR,          "scimitar",           11, -2, 14, 170,  3,
        SK_LONG_SWORDS,  HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },
    { WPN_KATANA,            "katana",             13, -1, 13, 160,  3,
        SK_LONG_SWORDS,  HANDS_HALF,   SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },
    { WPN_DEMON_BLADE,       "demon blade",        13, -3, 15, 200,  4,
        SK_LONG_SWORDS,  HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },
    { WPN_BLESSED_BLADE,     "blessed blade",      14, -3, 14, 200,  4,
        SK_LONG_SWORDS,  HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },
    { WPN_DOUBLE_SWORD,      "double sword",       15, -5, 16, 220,  5,
        SK_LONG_SWORDS,  HANDS_HALF,   SIZE_MEDIUM, MI_NONE, false,
        DAMV_SLICING },
    { WPN_GREAT_SWORD,       "great sword",        16, -5, 17, 250,  6,
        SK_LONG_SWORDS,  HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_SLICING },
    { WPN_TRIPLE_SWORD,      "triple sword",       17, -5, 17, 260,  6,
        SK_LONG_SWORDS,  HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_SLICING },

    // Axes
    { WPN_HAND_AXE,          "hand axe",            7,  3, 13,  80,  6,
        SK_AXES,         HANDS_ONE,    SIZE_SMALL,  MI_NONE, true,
        DAMV_CHOPPING },
    { WPN_WAR_AXE,           "war axe",            11, -3, 16, 180,  7,
        SK_AXES,         HANDS_ONE,    SIZE_MEDIUM, MI_NONE, false,
        DAMV_CHOPPING },
    { WPN_BROAD_AXE,         "broad axe",          14, -6, 17, 230,  8,
        SK_AXES,         HANDS_HALF,   SIZE_MEDIUM, MI_NONE, false,
        DAMV_CHOPPING },
    { WPN_BATTLEAXE,         "battleaxe",          16, -7, 18, 250,  8,
        SK_AXES,         HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_CHOPPING },
    { WPN_EXECUTIONERS_AXE,  "executioner\'s axe", 18, -8, 18, 280,  9,
        SK_AXES,         HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_CHOPPING },

    // Polearms
    { WPN_SPEAR,             "spear",               6,  3, 12,  50,  3,
        SK_POLEARMS,     HANDS_HALF,   SIZE_SMALL,  MI_NONE, true,
        DAMV_PIERCING },
    { WPN_TRIDENT,           "trident",             9,  2, 14, 160,  4,
        SK_POLEARMS,     HANDS_HALF,   SIZE_MEDIUM, MI_NONE, false,
        DAMV_PIERCING },
    { WPN_DEMON_TRIDENT,     "demon trident",      13,  2, 14, 160,  4,
        SK_POLEARMS,     HANDS_HALF,   SIZE_MEDIUM, MI_NONE, false,
        DAMV_PIERCING },
    { WPN_HALBERD,           "halberd",            13, -3, 16, 200,  5,
        SK_POLEARMS,     HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_CHOPPING | DAM_PIERCE },
    { WPN_SCYTHE,            "scythe",             14, -4, 20, 220,  7,
        SK_POLEARMS,     HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_SLICING },
    { WPN_GLAIVE,            "glaive",             15, -3, 18, 200,  6,
        SK_POLEARMS,     HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_CHOPPING },
    { WPN_LOCHABER_AXE,      "lochaber axe",       18, -6, 20, 200,  8,
        SK_POLEARMS,     HANDS_TWO,    SIZE_LARGE,  MI_NONE, false,
        DAMV_CHOPPING },

    // Staves (WPN_STAFF is now the magic stave base type)
    { WPN_STAFF,             "staff",               7,  4, 12, 100,  5,
        SK_STAVES,       HANDS_DOUBLE, SIZE_SMALL,  MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_QUARTERSTAFF,      "quarterstaff",        9,  2, 12, 180,  7,
        SK_STAVES,       HANDS_DOUBLE, SIZE_LARGE,  MI_NONE, false,
        DAMV_CRUSHING },
    { WPN_LAJATANG,          "lajatang",           14, -3, 14, 200,  3,
        SK_STAVES,      HANDS_DOUBLE,  SIZE_LARGE,  MI_NONE, false,
        DAMV_SLICING },

    // Range weapons
    // Notes:
    // - HANDS_HALF means a reloading time penalty if using shield
    // - damage field is used for bonus strength damage (string tension)
    // - slings get a bonus from dex, not str (as tension is meaningless)
    // - str weight is used for speed and applying dex to skill
    { WPN_BLOWGUN,           "blowgun",             0,  2, 10,  20,  0,
        SK_DARTS,        HANDS_HALF,   SIZE_LITTLE, MI_NEEDLE, false,
        DAMV_NON_MELEE },
    { WPN_SLING,             "sling",               0,  2, 11,  20,  1,
        SK_SLINGS,       HANDS_HALF,   SIZE_LITTLE, MI_STONE, false,
        DAMV_NON_MELEE },
    { WPN_HAND_CROSSBOW,     "hand crossbow",       3,  4, 15,  70,  5,
        SK_CROSSBOWS,    HANDS_HALF,   SIZE_SMALL,  MI_DART, false,
        DAMV_NON_MELEE },
    { WPN_CROSSBOW,          "crossbow",            5,  4, 15, 150,  8,
        SK_CROSSBOWS,    HANDS_TWO,    SIZE_MEDIUM, MI_BOLT, false,
        DAMV_NON_MELEE },
    { WPN_BOW,               "bow",                 4,  2, 11,  90,  2,
        SK_BOWS,         HANDS_TWO,    SIZE_MEDIUM, MI_ARROW, false,
        DAMV_NON_MELEE },
    { WPN_LONGBOW,           "longbow",             5,  0, 12, 120,  3,
        SK_BOWS,         HANDS_TWO,    SIZE_LARGE,  MI_ARROW, false,
        DAMV_NON_MELEE },
};

struct missile_def
{
    int         id;
    const char *name;
    int         dam;
    int         mass;
    bool        throwable;
};

static int Missile_index[NUM_MISSILES];
static missile_def Missile_prop[NUM_MISSILES] =
{
    { MI_NEEDLE,        "needle",       0,    1, false },
    { MI_STONE,         "stone",        4,    2, true  },
    { MI_DART,          "dart",         5,    3, true  },
    { MI_ARROW,         "arrow",        6,    5, false },
    { MI_BOLT,          "bolt",         8,    5, false },
    { MI_LARGE_ROCK,    "large rock",  20, 1000, true  },
};

struct food_def
{
    int         id;
    const char *name;
    int         value;
    int         carn_mod;
    int         herb_mod;
    int         mass;
    int         turns;
};

// NOTE: Any food with special random messages or side effects
// currently only takes one turn to eat (except ghouls and chunks)...
// if this changes then those items will have to have special code
// (like ghoul chunks) to guarantee that the special thing is only
// done once.  See the ghoul eating code over in food.cc.
static int Food_index[NUM_FOODS];
static food_def Food_prop[NUM_FOODS] =
{
    { FOOD_MEAT_RATION,  "meat ration",  5000,   500, -1500,  80, 4 },
    { FOOD_SAUSAGE,      "sausage",      1500,   150,  -400,  40, 1 },
    { FOOD_CHUNK,        "chunk",        1000,   100,  -500, 100, 3 },
    { FOOD_BEEF_JERKY,   "beef jerky",    800,   100,  -250,  20, 1 },

    { FOOD_BREAD_RATION, "bread ration", 4400, -1500,   750,  80, 4 },
    { FOOD_SNOZZCUMBER,  "snozzcumber",  1500,  -500,   500,  50, 1 },
    { FOOD_ORANGE,       "orange",       1000,  -350,   400,  20, 1 },
    { FOOD_BANANA,       "banana",       1000,  -350,   400,  20, 1 },
    { FOOD_LEMON,        "lemon",        1000,  -350,   400,  20, 1 },
    { FOOD_PEAR,         "pear",          700,  -250,   300,  20, 1 },
    { FOOD_APPLE,        "apple",         700,  -250,   300,  20, 1 },
    { FOOD_APRICOT,      "apricot",       700,  -250,   300,  15, 1 },
    { FOOD_CHOKO,        "choko",         600,  -200,   250,  30, 1 },
    { FOOD_RAMBUTAN,     "rambutan",      600,  -200,   250,  10, 1 },
    { FOOD_LYCHEE,       "lychee",        600,  -200,   250,  10, 1 },
    { FOOD_STRAWBERRY,   "strawberry",    200,   -80,   100,   5, 1 },
    { FOOD_GRAPE,        "grape",         100,   -40,    50,   2, 1 },
    { FOOD_SULTANA,      "sultana",        70,   -30,    30,   1, 1 },

    { FOOD_ROYAL_JELLY,  "royal jelly",  4000,     0,     0,  55, 1 },
    { FOOD_HONEYCOMB,    "honeycomb",    2000,     0,     0,  40, 1 },
    { FOOD_PIZZA,        "pizza",        1500,     0,     0,  40, 1 },
    { FOOD_CHEESE,       "cheese",       1200,     0,     0,  40, 1 },
};

// Must call this functions early on so that the above tables can
// be accessed correctly.
void init_properties(void)
{
    int i;

    for (i = 0; i < NUM_ARMOURS; i++)
        Armour_index[ Armour_prop[i].id ] = i;

    for (i = 0; i < NUM_WEAPONS; i++)
        Weapon_index[ Weapon_prop[i].id ] = i;

    for (i = 0; i < NUM_MISSILES; i++)
        Missile_index[ Missile_prop[i].id ] = i;

    for (i = 0; i < NUM_FOODS; i++)
        Food_index[ Food_prop[i].id ] = i;
}


// Some convenient functions to hide the bit operations and create
// an interface layer between the code and the data in case this
// gets changed again. -- bwr

//
// Item cursed status functions:
//
bool item_cursed( const item_def &item )
{
    return (item.flags & ISFLAG_CURSED);
}

bool item_known_cursed( const item_def &item )
{
    return ((item.flags & ISFLAG_KNOW_CURSE) && (item.flags & ISFLAG_CURSED));
}

bool item_known_uncursed( const item_def &item )
{
    return ((item.flags & ISFLAG_KNOW_CURSE) && !(item.flags & ISFLAG_CURSED));
}

void do_curse_item( item_def &item )
{
    item.flags |= ISFLAG_CURSED;

    if (item.base_type == OBJ_STAVES && item.sub_type == STAFF_POWER)
        calc_mp_max();
}

void do_uncurse_item( item_def &item )
{
    item.flags &= (~ISFLAG_CURSED);

    if (item.base_type == OBJ_STAVES && item.sub_type == STAFF_POWER)
        calc_mp_max();
}

//
// Item identification status:
//
bool item_ident( const item_def &item, unsigned long flags )
{
    return ((item.flags & flags) == flags);
}

void set_ident_flags( item_def &item, unsigned long flags )
{
    item.flags |= flags;
}

void unset_ident_flags( item_def &item, unsigned long flags )
{
    item.flags &= (~flags);
}

//
// Equipment race and description:
//
unsigned long get_equip_race( const item_def &item )
{
    return (item.flags & ISFLAG_RACIAL_MASK);
}

unsigned long get_equip_desc( const item_def &item )
{
    return (item.flags & ISFLAG_COSMETIC_MASK);
}

void set_equip_race( item_def &item, unsigned long flags )
{
    ASSERT( (flags & ~ISFLAG_RACIAL_MASK) == 0 );

    // first check for base-sub pairs that can't ever have racial types
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        if (item.sub_type == WPN_GIANT_CLUB
            || item.sub_type == WPN_GIANT_SPIKED_CLUB
            || item.sub_type == WPN_KATANA
            || item.sub_type == WPN_LAJATANG
            || item.sub_type == WPN_SLING
            || item.sub_type == WPN_KNIFE
            || item.sub_type == WPN_QUARTERSTAFF
            || item.sub_type == WPN_STAFF
            || item.sub_type == WPN_DEMON_BLADE
            || item.sub_type == WPN_DEMON_WHIP
            || item.sub_type == WPN_DEMON_TRIDENT)
        {
            return;
        }
        break;

    case OBJ_ARMOUR:
        // not hides, dragon armour, crystal plate, or barding
        if (item.sub_type >= ARM_DRAGON_HIDE
            && item.sub_type <= ARM_SWAMP_DRAGON_ARMOUR
            && item.sub_type != ARM_CENTAUR_BARDING
            && item.sub_type != ARM_NAGA_BARDING)
        {
            return;
        }
        break;

    case OBJ_MISSILES:
        if (item.sub_type == MI_STONE || item.sub_type == MI_LARGE_ROCK)
            return;
        break;

    default:
        return;
    }

    // check that item is appropriate for racial type
    switch (flags)
    {
    case ISFLAG_ELVEN:
        if (item.base_type == OBJ_ARMOUR
            && (item.sub_type == ARM_SPLINT_MAIL
                || item.sub_type == ARM_BANDED_MAIL
                || item.sub_type == ARM_PLATE_MAIL))
        {
            return;
        }
        break;

    case ISFLAG_DWARVEN:
        if (item.base_type == OBJ_ARMOUR
            && (item.sub_type == ARM_ROBE
                || item.sub_type == ARM_LEATHER_ARMOUR
                || item.sub_type == ARM_STUDDED_LEATHER_ARMOUR))
        {
            return;
        }
        break;

    case ISFLAG_ORCISH:
    default:
        break;
    }

    item.flags &= ~ISFLAG_RACIAL_MASK; // delete previous
    item.flags |= flags;
}

void set_equip_desc( item_def &item, unsigned long flags )
{
    ASSERT( (flags & ~ISFLAG_COSMETIC_MASK) == 0 );

    item.flags &= ~ISFLAG_COSMETIC_MASK; // delete previous
    item.flags |= flags;
}

//
// These functions handle the description and subtypes for helmets/caps
//
short get_helmet_type( const item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR
            && get_armour_slot( item ) == EQ_HELMET );

    return (item.plus2 & THELM_TYPE_MASK);
}

short get_helmet_desc( const item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR
            && get_armour_slot( item ) == EQ_HELMET );

    return (item.plus2 & THELM_DESC_MASK);
}

void set_helmet_type( item_def &item, short type )
{
    ASSERT( (type & ~THELM_TYPE_MASK) == 0 );
    ASSERT( item.base_type == OBJ_ARMOUR
            && get_armour_slot( item ) == EQ_HELMET );

    // make sure we have the right sub_type (to get properties correctly)
    if (type == THELM_HELMET || type == THELM_HELM)
        item.sub_type = ARM_HELMET;
    else
        item.sub_type = ARM_CAP;

    item.plus2 &= ~THELM_TYPE_MASK;
    item.plus2 |= type;
}

void set_helmet_desc( item_def &item, short type )
{
    ASSERT( (type & ~THELM_DESC_MASK) == 0 );
    ASSERT( item.base_type == OBJ_ARMOUR
            && get_armour_slot( item ) == EQ_HELMET );

    if (item.sub_type == ARM_CAP && type > THELM_DESC_PLUMED)
        type = THELM_DESC_PLAIN;

    item.plus2 &= ~THELM_DESC_MASK;
    item.plus2 |= type;
}

void set_helmet_random_desc( item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR
            && get_armour_slot( item ) == EQ_HELMET );

    item.plus2 &= ~THELM_DESC_MASK;

    if (item.sub_type == ARM_HELMET)
        item.plus2 |= (random2(8) << 8);
    else
        item.plus2 |= (random2(5) << 8);

}

bool is_helmet_type( const item_def &item, short val )
{
    if (item.base_type != OBJ_ARMOUR || get_armour_slot( item ) != EQ_HELMET)
        return (false);

    return (get_helmet_type( item ) == val);
}

//
// Ego item functions:
//
bool set_item_ego_type( item_def &item, int item_type, int ego_type )
{
    if (item.base_type == item_type
        && !is_random_artefact( item )
        && !is_fixed_artefact( item ))
    {
        item.special = ego_type;
        return (true);
    }

    return (false);
}

int get_weapon_brand( const item_def &item )
{
    // Weapon ego types are "brands", so we do the randart lookup here.

    // Staves "brands" handled specially
    if (item.base_type != OBJ_WEAPONS)
        return (SPWPN_NORMAL);

    if (is_fixed_artefact( item ))
    {
        switch (item.special)
        {
        case SPWPN_SWORD_OF_CEREBOV:
            return (SPWPN_FLAMING);

        case SPWPN_STAFF_OF_OLGREB:
            return (SPWPN_VENOM);

        case SPWPN_VAMPIRES_TOOTH:
            return (SPWPN_VAMPIRICISM);

        default:
            return (SPWPN_NORMAL);
        }
    }
    else if (is_random_artefact( item ))
    {
        return (item.ra_props[RAP_BRAND]);
    }

    return (item.special);
}

int get_ammo_brand( const item_def &item )
{
    // no artefact arrows yet -- bwr
    if (item.base_type != OBJ_MISSILES || is_random_artefact( item ))
        return (SPMSL_NORMAL);

    return (item.special);
}

int get_armour_ego_type( const item_def &item )
{
    // artefact armours have no ego type, must look up powers separately
    if (item.base_type != OBJ_ARMOUR
        || (is_random_artefact( item ) && !is_unrandom_artefact( item )))
    {
        return (SPARM_NORMAL);
    }

    return (item.special);
}

//
// Armour information and checking functions
//
bool hide2armour( item_def &item )
{
    if (item.base_type != OBJ_ARMOUR)
        return (false);

    switch (item.sub_type)
    {
    default:
        return (false);

    case ARM_DRAGON_HIDE:
        item.sub_type = ARM_DRAGON_ARMOUR;
        break;

    case ARM_TROLL_HIDE:
        item.sub_type = ARM_TROLL_LEATHER_ARMOUR;
        break;

    case ARM_ICE_DRAGON_HIDE:
        item.sub_type = ARM_ICE_DRAGON_ARMOUR;
        break;

    case ARM_MOTTLED_DRAGON_HIDE:
        item.sub_type = ARM_MOTTLED_DRAGON_ARMOUR;
        break;

    case ARM_STORM_DRAGON_HIDE:
        item.sub_type = ARM_STORM_DRAGON_ARMOUR;
        break;

    case ARM_GOLD_DRAGON_HIDE:
        item.sub_type = ARM_GOLD_DRAGON_ARMOUR;
        break;

    case ARM_SWAMP_DRAGON_HIDE:
        item.sub_type = ARM_SWAMP_DRAGON_ARMOUR;
        break;

    case ARM_STEAM_DRAGON_HIDE:
        item.sub_type = ARM_STEAM_DRAGON_ARMOUR;
        break;
    }

    return (true);
}                               // end hide2armour()

// Return the enchantment limit of a piece of armour
int armour_max_enchant( const item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    const int eq_slot = get_armour_slot( item );

    int max_plus = MAX_SEC_ENCHANT;
    if (eq_slot == EQ_BODY_ARMOUR || eq_slot == EQ_SHIELD)
        max_plus = MAX_ARM_ENCHANT;

    return (max_plus);
}

// doesn't include animal skin (only skins we can make and enchant)
bool armour_is_hide( const item_def &item, bool inc_made )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    switch (item.sub_type)
    {
    case ARM_TROLL_LEATHER_ARMOUR:
    case ARM_DRAGON_ARMOUR:
    case ARM_ICE_DRAGON_ARMOUR:
    case ARM_STEAM_DRAGON_ARMOUR:
    case ARM_MOTTLED_DRAGON_ARMOUR:
    case ARM_STORM_DRAGON_ARMOUR:
    case ARM_GOLD_DRAGON_ARMOUR:
    case ARM_SWAMP_DRAGON_ARMOUR:
        return (inc_made);

    case ARM_TROLL_HIDE:
    case ARM_DRAGON_HIDE:
    case ARM_ICE_DRAGON_HIDE:
    case ARM_STEAM_DRAGON_HIDE:
    case ARM_MOTTLED_DRAGON_HIDE:
    case ARM_STORM_DRAGON_HIDE:
    case ARM_GOLD_DRAGON_HIDE:
    case ARM_SWAMP_DRAGON_HIDE:
        return (true);

    default:
        break;
    }

    return (false);
}

// used to distinguish shiny and embroidered
bool armour_not_shiny( const item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    switch (item.sub_type)
    {
    case ARM_ROBE:
    case ARM_CLOAK:
    case ARM_GLOVES:
    case ARM_BOOTS:
    case ARM_NAGA_BARDING:
    case ARM_CENTAUR_BARDING:
    case ARM_CAP:
    case ARM_LEATHER_ARMOUR:
    case ARM_STUDDED_LEATHER_ARMOUR:
    case ARM_ANIMAL_SKIN:
    case ARM_TROLL_HIDE:
    case ARM_TROLL_LEATHER_ARMOUR:
        return (true);

    default:
        break;
    }

    return (false);
}

int armour_str_required( const item_def &arm )
{
    ASSERT (arm.base_type == OBJ_ARMOUR );

    int ret = 0;

    const equipment_type  slot = get_armour_slot( arm );
    const int             mass = item_mass( arm );

    switch (slot)
    {
    case EQ_BODY_ARMOUR:
        ret = mass / 35;
        break;

    case EQ_SHIELD:
        ret = mass / 15;
        break;

    default:
        break;
    }

    return ((ret < STR_REQ_THRESHOLD) ? 0 : ret);
}

equipment_type get_armour_slot( const item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    return (Armour_prop[ Armour_index[item.sub_type] ].slot);
}

bool jewellery_is_amulet( const item_def &item )
{
    ASSERT( item.base_type == OBJ_JEWELLERY );

    return (item.sub_type >= AMU_RAGE);
}

bool check_jewellery_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_JEWELLERY );

    // Currently assuming amulets are always wearable (only needs
    // to be held over head or heart... giants can strap it on with
    // a bit of binder twine).  However, rings need to actually fit
    // around the ring finger to work, and so the big cannot use them.
    return (size <= SIZE_LARGE || jewellery_is_amulet( item ));
}

// Returns the basic light status of an armour, ignoring things like the
// elven bonus... you probably want is_light_armour() most times.
bool base_armour_is_light( const item_def &item )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    return (Armour_prop[ Armour_index[item.sub_type] ].light);
}

// returns number of sizes off
int fit_armour_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    const size_type min = Armour_prop[ Armour_index[item.sub_type] ].fit_min;
    const size_type max = Armour_prop[ Armour_index[item.sub_type] ].fit_max;

    if (size < min)
        return (min - size);    // -'ve means levels too small
    else if (size > max)
        return (max - size);    // +'ve means levels too large

    return (0);
}

// returns true if armour fits size (shape needs additional verification)
bool check_armour_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    return (fit_armour_size( item, size ) == 0);
}

// Note that this function is used to check validity of equipment
// coming out of transformations... so it shouldn't contain any
// wield/unwield only checks like two-handed weapons and shield.
bool check_armour_shape( const item_def &item, bool quiet )
{
    ASSERT( item.base_type == OBJ_ARMOUR );

    const int slot = get_armour_slot( item );

    if (!player_is_shapechanged())
    {
        switch (slot)
        {
        case EQ_BOOTS:
            switch (you.species)
            {
            case SP_NAGA:
                if (item.sub_type != ARM_NAGA_BARDING)
                {
                    if (!quiet)
                        mpr( "You can't wear that!" );

                    return (false);
                }
                break;

            case SP_CENTAUR:
                if (item.sub_type != ARM_CENTAUR_BARDING)
                {
                    if (!quiet)
                        mpr( "You can't wear that!" );

                    return (false);
                }
                break;

            case SP_KENKU:
                if (!quiet)
                {
                    if (item.sub_type == ARM_BOOTS)
                        mpr( "Boots don't fit your feet!" );
                    else
                        mpr( "You can't wear barding!" );
                }
                return (false);

            case SP_MERFOLK:
                if (player_in_water() && item.sub_type == ARM_BOOTS)
                {
                    if (!quiet)
                        mpr( "You don't currently have feet!" );

                    return (false);
                }
                // intentional fall-through
            default:
                if (item.sub_type == ARM_NAGA_BARDING
                    || item.sub_type == ARM_CENTAUR_BARDING)
                {
                    if (!quiet)
                        mpr( "You can't wear barding!" );

                    return (false);
                }

                if (you.mutation[MUT_HOOVES] >= 2)
                {
                    if (!quiet)
                        mpr( "You can't wear boots with hooves!" );

                    return (false);
                }
                break;
            }
            break;

        case EQ_HELMET:
            if (item.sub_type == ARM_CAP)
                break;

            if (you.species == SP_KENKU)
            {
                if (!quiet)
                    mpr( "That helmet does not fit your head!" );

                return (false);
            }

            if (you.species == SP_MINOTAUR || you.mutation[MUT_HORNS])
            {
                if (!quiet)
                    mpr( "You can't wear that with your horns!" );

                return (false);
            }
            break;

        case EQ_GLOVES:
            if (you.mutation[MUT_CLAWS] >= 3)
            {
                if (!quiet)
                    mpr( "You can't wear gloves with your huge claws!" );

                return (false);
            }
            break;

        case EQ_BODY_ARMOUR:
            // Cannot swim in heavy armour
            if (player_is_swimming() && !is_light_armour( item ))
            {
                if (!quiet)
                   mpr("You can't swim in that!");

                return (false);
            }

            // Draconians are human-sized, but have wings that cause problems
            // with most body armours (only very flexible fit allowed).
            if (player_genus( GENPC_DRACONIAN )
                && !check_armour_size( item, SIZE_BIG ))
            {
                if (!quiet)
                   mpr( "That armour doesn't fit your wings." );

                return (false);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        // Note: some transformation include all of the above as well
        if (item.sub_type == ARM_NAGA_BARDING
            || item.sub_type == ARM_CENTAUR_BARDING)
        {
            if (!quiet)
               mpr( "You can't wear barding in your current form!" );

            return (false);
        }
    }

    // Note: This need to be checked after all the special cases
    // above, and in addition to shapechanged or not.  This is
    // a simple check against the armour types that are forced off.
    if (!transform_can_equip_type( slot ))
    {
        if (!quiet)
           mpr( "You can't wear that in your current form!" );

        return (false);
    }

    return (true);
}

//
// Weapon information and checking functions:
//

// Checks how rare a weapon is. Many of these have special routines for
// placement, especially those with a rarity of zero. Chance is out of 10.
int weapon_rarity( int w_type )
{
    switch (w_type)
    {
    case WPN_CLUB:
    case WPN_DAGGER:
        return (10);

    case WPN_HAND_AXE:
    case WPN_MACE:
    case WPN_QUARTERSTAFF:
        return (9);

    case WPN_BOW:
    case WPN_FLAIL:
    case WPN_HAMMER:
    case WPN_SABRE:
    case WPN_SHORT_SWORD:
    case WPN_SLING:
    case WPN_SPEAR:
        return (8);

    case WPN_FALCHION:
    case WPN_LONG_SWORD:
    case WPN_MORNINGSTAR:
    case WPN_WAR_AXE:
        return (7);

    case WPN_BATTLEAXE:
    case WPN_CROSSBOW:
    case WPN_GREAT_SWORD:
    case WPN_SCIMITAR:
    case WPN_TRIDENT:
        return (6);

    case WPN_GLAIVE:
    case WPN_HALBERD:
    case WPN_BLOWGUN:
    case WPN_STAFF:
        return (5);

    case WPN_BROAD_AXE:
    case WPN_HAND_CROSSBOW:
    case WPN_SPIKED_FLAIL:
    case WPN_WHIP:
        return (4);

    case WPN_GREAT_MACE:
        return (3);

    case WPN_ANCUS:
    case WPN_DIRE_FLAIL:
    case WPN_SCYTHE:
    case WPN_LONGBOW:
        return (2);

    case WPN_GIANT_CLUB:
    case WPN_GIANT_SPIKED_CLUB:
    case WPN_LOCHABER_AXE:
        return (1);

    case WPN_DOUBLE_SWORD:
    case WPN_EVENINGSTAR:
    case WPN_EXECUTIONERS_AXE:
    case WPN_KATANA:
    case WPN_LAJATANG:
    case WPN_KNIFE:
    case WPN_QUICK_BLADE:
    case WPN_TRIPLE_SWORD:
    case WPN_DEMON_TRIDENT:
    case WPN_DEMON_WHIP:
    case WPN_DEMON_BLADE:
    case WPN_BLESSED_BLADE:
        // zero value weapons must be placed specially -- see make_item() {dlb}
        return (0);

    default:
        break;
    }

    return (0);
}                               // end rare_weapon()

int get_vorpal_type( const item_def &item )
{
    int ret = DVORP_NONE;

    if (item.base_type == OBJ_WEAPONS)
        ret = (Weapon_prop[ Weapon_index[item.sub_type] ].dam_type & DAMV_MASK);

    return (ret);
}                               // end vorpal_type()

int get_damage_type( const item_def &item )
{
    int ret = DAM_BASH;

    if (item.base_type == OBJ_WEAPONS)
        ret = (Weapon_prop[ Weapon_index[item.sub_type] ].dam_type & DAM_MASK);

    return (ret);
}                               // end damage_type()

bool does_damage_type( const item_def &item, int dam_type )
{
    return (get_damage_type( item ) & dam_type);
}                               // end does_damage_type()


// give hands required to wield weapon for a torso of "size"
hands_reqd_type hands_reqd( const item_def &item, size_type size )
{
    int         ret = HANDS_ONE;
    int         fit;
    bool        doub = false;

    switch (item.base_type)
    {
    case OBJ_STAVES:
    case OBJ_WEAPONS:
        // Merging staff with magical staves for consistancy... doing
        // as a special case because we want to be very flexible with
        // these useful objects (we want spriggans and ogre magi to
        // be able to use them).
        if (item.base_type == OBJ_STAVES || item.sub_type == WPN_STAFF)
        {
            if (size < SIZE_SMALL)
                ret = HANDS_TWO;
            else if (size > SIZE_LARGE)
                ret = HANDS_ONE;
            else
                ret = HANDS_HALF;
            break;
        }

        ret = Weapon_prop[ Weapon_index[item.sub_type] ].hands;

        // size is the level where we can use one hand for one end
        if (ret == HANDS_DOUBLE)
        {
            doub = true;
            ret = HANDS_HALF;
        }

        // adjust handedness for size only for non-whip melee weapons
        if (!is_range_weapon( item )
            && item.sub_type != WPN_WHIP
            && item.sub_type != WPN_DEMON_WHIP)
        {
            fit = cmp_weapon_size( item, size );

            // Adjust handedness for non-medium races:
            // (XX values don't matter, see fit_weapon_wieldable_size)
            //
            //         Spriggan Kobold  Human   Ogre    Big     Giant
            // Little      0       0      0      XX     XX      XX
            // Small      +1       0      0      -2     XX      XX
            // Medium     XX      +1      0      -1     -2      XX
            // Large      XX      XX      0       0     -1      -2
            // Big        XX      XX     XX       0      0      -1
            // Giant      XX      XX     XX      XX      0       0

            // Note the stretching of double weapons for larger characters
            // by one level since they tend to be larger weapons.
            if (size < SIZE_MEDIUM && fit > 0)
                ret += fit;
            else if (size > SIZE_MEDIUM && fit < 0)
                ret += (fit + doub);
        }
        break;

    case OBJ_CORPSES:   // unwieldy
        ret = HANDS_TWO;
        break;

    case OBJ_ARMOUR:    // barding and body armours are unwieldy
        if (item.sub_type == ARM_NAGA_BARDING
            || item.sub_type == ARM_CENTAUR_BARDING
            || get_armour_slot( item ) == EQ_BODY_ARMOUR)
        {
            ret = HANDS_TWO;
        }
        break;

    default:
        break;
    }

    if (ret > HANDS_TWO)
        ret = HANDS_TWO;
    else if (ret < HANDS_ONE)
        ret = HANDS_ONE;

    return (static_cast< hands_reqd_type >( ret ));
}

bool is_double_ended( const item_def &item )
{
    if (item.base_type == OBJ_STAVES)
        return (true);
    else if (item.base_type != OBJ_WEAPONS)
        return (false);

    return (Weapon_prop[ Weapon_index[item.sub_type] ].hands == HANDS_DOUBLE);
}

int double_wpn_awkward_speed( const item_def &item )
{
    ASSERT( is_double_ended( item ) );

    const int base = property( item, PWPN_SPEED );

    return ((base * 30 + 10) / 20 + 2);
}


bool is_demonic( const item_def &item )
{
    if (item.base_type == OBJ_WEAPONS)
    {
        switch (item.sub_type)
        {
        case WPN_DEMON_BLADE:
        case WPN_DEMON_WHIP:
        case WPN_DEMON_TRIDENT:
            return (true);

        default:
            break;
        }
    }

    return (false);
}                               // end is_demonic()

int weapon_str_weight( const item_def &wpn )
{
    ASSERT (wpn.base_type == OBJ_WEAPONS || wpn.base_type == OBJ_STAVES);

    if (wpn.base_type == OBJ_STAVES)
        return (Weapon_prop[ Weapon_index[WPN_STAFF] ].str_weight);

    return (Weapon_prop[ Weapon_index[wpn.sub_type] ].str_weight);
}

int weapon_dex_weight( const item_def &wpn )
{
    return (10 - weapon_str_weight( wpn ));
}

int weapon_impact_mass( const item_def &wpn )
{
    ASSERT (wpn.base_type == OBJ_WEAPONS || wpn.base_type == OBJ_STAVES);

    return ((weapon_str_weight( wpn ) * item_mass( wpn ) + 5) / 10);
}

int weapon_str_required( const item_def &wpn, bool hand_half )
{
    ASSERT (wpn.base_type == OBJ_WEAPONS || wpn.base_type == OBJ_STAVES);

    const int req = weapon_impact_mass( wpn ) / ((hand_half) ? 11 : 10);

    return ((req < STR_REQ_THRESHOLD) ? 0 : req);
}

// returns melee skill of item
skill_type weapon_skill( const item_def &item )
{
    if (item.base_type == OBJ_WEAPONS && !is_range_weapon( item ))
        return (Weapon_prop[ Weapon_index[item.sub_type] ].skill);
    else if (item.base_type == OBJ_STAVES)
        return (SK_STAVES);

    // this is used to mark that only fighting applies.
    return (SK_FIGHTING);
}

// front function for the above when we don't have a physical item to check
skill_type weapon_skill( int wclass, int wtype )
{
    item_def    wpn;

    wpn.base_type = wclass;
    wpn.sub_type = wtype;

    return (weapon_skill( wpn ));
}                               // end weapon_skill()

// returns range skill of the item
skill_type range_skill( const item_def &item )
{
    if (item.base_type == OBJ_WEAPONS && is_range_weapon( item ))
        return (Weapon_prop[ Weapon_index[item.sub_type] ].skill);
    else if (item.base_type == OBJ_MISSILES && item.sub_type == MI_DART)
        return (SK_DARTS);

    return (SK_RANGED_COMBAT);
}

// front function for the above when we don't have a physical item to check
skill_type range_skill( int wclass, int wtype )
{
    item_def    wpn;

    wpn.base_type = wclass;
    wpn.sub_type = wtype;

    return (range_skill( wpn ));
}                               // end weapon_skill()


// Calculate the bonus to melee EV for using "wpn", with "skill" and "dex"
// to protect a body of size "body".
int weapon_ev_bonus( const item_def &wpn, int skill, size_type body, int dex,
                     bool hide_hidden )
{
    ASSERT( wpn.base_type == OBJ_WEAPONS || wpn.base_type == OBJ_STAVES );

    int ret = 0;

    // Note: ret currently measured in halves (see skill factor)
    if (wpn.sub_type == WPN_WHIP || wpn.sub_type == WPN_DEMON_WHIP)
        ret = 3 + (dex / 5);
    else if (weapon_skill( wpn ) == SK_POLEARMS)
        ret = 2 + (dex / 5);

    // weapons of reaching are naturally a bit longer/flexier
    if (!hide_hidden || item_ident( wpn, ISFLAG_KNOW_TYPE ))
    {
        if (get_weapon_brand( wpn ) == SPWPN_REACHING)
            ret += 1;
    }

    // only consider additional modifications if we have a positive base:
    if (ret > 0)
    {
        // Size factors:
        // - large characters can't cover their flanks as well
        // - note that not all weapons are available to small characters
        if (body > SIZE_LARGE)
            ret -= (4 * (body - SIZE_LARGE) - 2);
        else if (body < SIZE_MEDIUM)
            ret += 1;

        // apply skill (and dividing by 2)
        ret = (ret * (skill + 10)) / 20;

        // make sure things can't get too insane
        if (ret > 8)
            ret = 8 + (ret - 8) / 2;
    }

    // Note: this is always a bonus
    return ((ret > 0) ? ret : 0);
}

static size_type weapon_size( const item_def &item )
{
    ASSERT (item.base_type == OBJ_WEAPONS || item.base_type == OBJ_STAVES);

    if (item.base_type == OBJ_STAVES)
        return (Weapon_prop[ Weapon_index[WPN_STAFF] ].fit_size);

    return (Weapon_prop[ Weapon_index[item.sub_type] ].fit_size);
}

// returns number of sizes off
int cmp_weapon_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_WEAPONS || item.base_type == OBJ_STAVES );

    return (weapon_size( item ) - size);
}

// Returns number of sizes away from being a usable weapon
int fit_weapon_wieldable_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_WEAPONS || item.base_type == OBJ_STAVES );

    const int fit = cmp_weapon_size( item, size );

    return ((fit < -2) ? fit + 2 : (fit > 1) ? fit - 1 : 0);
}

// Returns number of sizes away from being throwable... the window
// is currently [size - 5, size - 1].
int fit_item_throwable_size( const item_def &item, size_type size )
{
    int ret = item_size( item ) - size;

    return ((ret >= 0) ? ret + 1 : (ret > -6) ? 0 : ret + 5);
}

// Returns true if weapon is usable as a tool
// Note that we assume that tool usable >= wieldable
bool check_weapon_tool_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_WEAPONS || item.base_type == OBJ_STAVES );

    // Staves are currently usable for everyone just to be nice.
    if (item.base_type == OBJ_STAVES || item.sub_type == WPN_STAFF)
        return (true);

    const int fit = cmp_weapon_size( item, size );

    return (fit >= -3 && fit <= 1);
}

// Returns true if weapon is uasable as a weapon
bool check_weapon_wieldable_size( const item_def &item, size_type size )
{
    ASSERT( item.base_type == OBJ_WEAPONS || item.base_type == OBJ_STAVES );

    return (fit_weapon_wieldable_size( item, size ) == 0);
}

// Note that this function is used to check validity of equipment
// coming out of transformations... so it shouldn't contain any
// wield/unwield only checks like two-handed weapons and shield.
// check_id is only used for descriptions, where we don't want to
// give away any information the player doesn't have yet.
bool check_weapon_shape( const item_def &item, bool quiet, bool check_id )
{
    const int brand = get_weapon_brand( item );

    if ((!check_id || item_ident( item, ISFLAG_KNOW_TYPE ))
        && ((item.base_type == OBJ_WEAPONS
                && item.sub_type == WPN_BLESSED_BLADE)
            || brand == SPWPN_HOLY_WRATH
            || brand == SPWPN_DISRUPTION)
        && (you.is_undead || you.species == SP_DEMONSPAWN))
    {
        if (!quiet)
            mpr( "This weapon will not allow you to wield it." );

        return (false);
    }

    // Note: this should always be done last, see check_armour_shape()
    if (!transform_can_equip_type( EQ_WEAPON ))
    {
        if (!quiet)
           mpr( "You can't wield anything in your current form!" );

        return (false);
    }

    return (true);
}

// Returns the you.inv[] index of our wielded weapon or -1 (no item, not wield)
int get_inv_wielded( void )
{
    return (player_weapon_wielded() ? you.equip[EQ_WEAPON] : -1);
}

// Returns the you.inv[] index of our hand tool or -1 (no item, not usable)
// Note that we don't count armour and such as "tools" here because
// this function is used to judge if the item will sticky curse to
// our hands.
int get_inv_hand_tool( void )
{
    const int tool = you.equip[EQ_WEAPON];

    if (tool == -1 || !is_tool( you.inv[tool] ))
        return (-1);

    if (you.inv[tool].base_type == OBJ_WEAPONS
        || you.inv[tool].base_type == OBJ_STAVES)
    {
        // assuring that bad "shape" weapons aren't in hand
        ASSERT( check_weapon_shape( you.inv[tool], false ) );

        if (!check_weapon_tool_size( you.inv[tool], player_size() ))
            return (-1);
    }

    return (tool);
}

// Returns the you.inv[] index of the thing in our hand... this is provided
// as a service to specify that both of the above are irrelevant.
// Do not use this for low level functions dealing with wielding directly.
int get_inv_in_hand( void )
{
    return (you.equip[EQ_WEAPON]);
}

//
// Launcher and ammo functions:
//
missile_type fires_ammo_type( const item_def &item )
{
    if (item.base_type != OBJ_WEAPONS)
        return (MI_NONE);

    return (Weapon_prop[ Weapon_index[item.sub_type] ].ammo);
}

bool is_range_weapon( const item_def &item )
{
    return (fires_ammo_type( item ) != MI_NONE);
}

bool is_range_weapon_type( weapon_type wtype )
{
    item_def wpn;

    wpn.base_type = OBJ_WEAPONS;
    wpn.sub_type = wtype;

    return (is_range_weapon( wpn ));
}

const char * ammo_name( const item_def &bow )
{
    ASSERT( is_range_weapon( bow ) );

    const int ammo = fires_ammo_type( bow );

    return (Missile_prop[ Missile_index[ammo] ].name);
}

// returns true if item can be reasonably thrown without a launcher
bool is_throwable( const item_def &wpn )
{
    if (wpn.base_type == OBJ_WEAPONS)
        return (Weapon_prop[ Weapon_index[wpn.sub_type] ].throwable);
    else if (wpn.base_type == OBJ_MISSILES)
        return (Missile_prop[ Missile_index[wpn.sub_type] ].throwable);

    return (false);
}

// decide if "being" is launching or throwing "ammo"
launch_retval is_launched( int being_id, const item_def &ammo, bool msg )
{
    ASSERT( being_id != MHITNOT );

    launch_retval  ret = LRET_FUMBLED;

    const item_def * lnch = 0;
    int              fit = 0;

    if (being_id == MHITYOU)
    {
        const int wpn = get_inv_wielded();
        lnch = (wpn == -1) ? 0 : &you.inv[wpn];
        fit = fit_item_throwable_size( ammo, player_size() );
    }
    else // monster case
    {
        const int wpn = menv[being_id].inv[MSLOT_WEAPON];
        lnch = (wpn == NON_ITEM) ? 0 : &mitm[wpn];
        fit = fit_item_throwable_size( ammo, mons_size( menv[being_id].type ) );
    }

    if (lnch
        && lnch->base_type == OBJ_WEAPONS
        && is_range_weapon( *lnch )
        && ammo.base_type == OBJ_MISSILES
        && ammo.sub_type == fires_ammo_type( *lnch ))
    {
        ret = LRET_LAUNCHED;
    }
    else if (is_throwable( ammo ))
    {
        if (fit == 0)
            ret = LRET_THROWN;
        else
        {
            ret = LRET_FUMBLED;

            if (being_id == MHITYOU && msg)
            {
                mpr( MSGCH_WARN, "It's difficult to throw such a%s object.",
                     (fit > 0) ? " large" : (fit < 0) ? " small" : "n awkward" );
            }
        }
    }

    return (ret);
}

//
// Staff/rod functions:
//
bool item_is_rod( const item_def &item )
{
    return (item.base_type == OBJ_STAVES
            && item.sub_type >= STAFF_SMITING && item.sub_type < STAFF_AIR);
}

bool item_is_staff( const item_def &item )
{
    // Isn't De Morgan's law wonderful. -- bwr
    return (item.base_type == OBJ_STAVES
            && (item.sub_type < STAFF_SMITING || item.sub_type >= STAFF_AIR));
}

//
// Ring functions:
//
// Returns number of pluses on jewellery (always none for amulets yet).
int ring_has_pluses( const item_def &item )
{
    ASSERT (item.base_type == OBJ_JEWELLERY);

    switch (item.sub_type)
    {
    case RING_SLAYING:
        return (2);

    case RING_PROTECTION:
    case RING_EVASION:
    case RING_STRENGTH:
    case RING_INTELLIGENCE:
    case RING_DEXTERITY:
        return (1);

    default:
        break;
    }

    return (0);
}

//
// Food functions:
//
bool food_is_meat( const item_def &item )
{
    ASSERT( is_valid_item( item ) && item.base_type == OBJ_FOOD );

    return (Food_prop[ Food_index[item.sub_type] ].carn_mod > 0);
}

bool food_is_veg( const item_def &item )
{
    ASSERT( is_valid_item( item ) && item.base_type == OBJ_FOOD );

    return (Food_prop[ Food_index[item.sub_type] ].herb_mod > 0);
}

// returns food value for one turn of eating
int food_value( const item_def &item )
{
    ASSERT( is_valid_item( item ) && item.base_type == OBJ_FOOD );

    const int herb = you.mutation[MUT_HERBIVOROUS];

    // XXX: this needs to be better merged with the mutation system
    const int carn = (you.species == SP_KOBOLD || you.species == SP_GHOUL) ? 3
                                            : you.mutation[MUT_CARNIVOROUS];

    const food_def &food = Food_prop[ Food_index[item.sub_type] ];

    int ret = food.value;

    ret += (carn * food.carn_mod);
    ret += (herb * food.herb_mod);

    return ((ret > 0) ? div_rand_round( ret, food.turns ) : 0);
}

int food_turns( const item_def &item )
{
    ASSERT( is_valid_item( item ) && item.base_type == OBJ_FOOD );

    return (Food_prop[ Food_index[item.sub_type] ].turns);
}

bool can_cut_meat( const item_def &item )
{
    return (does_damage_type( item, DAM_SLICE ));
}

// returns true if item counts as a tool for tool size comaparisons and msgs
bool is_tool( const item_def &item )
{
    // Currently using OBJ_WEAPONS instead of can_cut_meat() as almost
    // any weapon might be an evocable artefact.
    return (item.base_type == OBJ_WEAPONS
            || item.base_type == OBJ_STAVES
            || (item.base_type == OBJ_MISCELLANY
                && item.sub_type != MISC_RUNE_OF_ZOT));
}


//
// Generic item functions:
//
int property( const item_def &item, int prop_type )
{
    switch (item.base_type)
    {
    case OBJ_ARMOUR:
        if (prop_type == PARM_AC)
            return (Armour_prop[ Armour_index[item.sub_type] ].ac);
        else if (prop_type == PARM_EVASION)
            return (Armour_prop[ Armour_index[item.sub_type] ].ev);
        break;

    case OBJ_WEAPONS:
        if (prop_type == PWPN_DAMAGE)
            return (Weapon_prop[ Weapon_index[item.sub_type] ].dam);
        else if (prop_type == PWPN_HIT)
            return (Weapon_prop[ Weapon_index[item.sub_type] ].hit);
        else if (prop_type == PWPN_SPEED)
            return (Weapon_prop[ Weapon_index[item.sub_type] ].speed);
        break;

    case OBJ_MISSILES:
        if (prop_type == PWPN_DAMAGE)
            return (Missile_prop[ Missile_index[item.sub_type] ].dam);
        break;

    case OBJ_STAVES:
        if (prop_type == PWPN_DAMAGE)
            return (Weapon_prop[ Weapon_index[WPN_STAFF] ].dam);
        else if (prop_type == PWPN_HIT)
            return (Weapon_prop[ Weapon_index[WPN_STAFF] ].hit);
        else if (prop_type == PWPN_SPEED)
            return (Weapon_prop[ Weapon_index[WPN_STAFF] ].speed);
        break;

    default:
        break;
    }

    return (0);
}

int item_mass( const item_def &item )
{
    int unit_mass = 0;

    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        unit_mass = Weapon_prop[ Weapon_index[item.sub_type] ].mass;
        break;

    case OBJ_ARMOUR:
        unit_mass = Armour_prop[ Armour_index[item.sub_type] ].mass;

        if (get_equip_race( item ) == ISFLAG_ELVEN)
        {
            const int reduc = (unit_mass >= 25) ? unit_mass / 5 : 5;

            // truncate to the nearest 5 and reduce the item mass:
            unit_mass -= ((reduc / 5) * 5);
            unit_mass = MAXIMUM( unit_mass, 5 );
        }
        break;

    case OBJ_MISSILES:
        unit_mass = Missile_prop[ Missile_index[item.sub_type] ].mass;
        break;

    case OBJ_FOOD:
        unit_mass = Food_prop[ Food_index[item.sub_type] ].mass;
        break;

    case OBJ_WANDS:
        unit_mass = 100;
        break;

    case OBJ_UNKNOWN_I:
        unit_mass = 200;        // labeled "books"
        break;

    case OBJ_SCROLLS:
        unit_mass = 20;
        break;

    case OBJ_JEWELLERY:
        unit_mass = 10;
        break;

    case OBJ_POTIONS:
        unit_mass = 40;
        break;

    case OBJ_UNKNOWN_II:
        unit_mass = 5;          // labeled "gems"
        break;

    case OBJ_BOOKS:
        unit_mass = 70;
        break;

    case OBJ_STAVES:
        unit_mass = 130;
        break;

    case OBJ_ORBS:
        unit_mass = 300;
        break;

    case OBJ_MISCELLANY:
        switch (item.sub_type)
        {
        case MISC_BOTTLED_EFREET:
        case MISC_CRYSTAL_BALL_OF_SEEING:
        case MISC_CRYSTAL_BALL_OF_ENERGY:
        case MISC_CRYSTAL_BALL_OF_FIXATION:
            unit_mass = 150;
            break;

        default:
            unit_mass = 100;
            break;
        }
        break;

    case OBJ_CORPSES:
        unit_mass = mons_weight( item.plus );

        if (item.sub_type == CORPSE_SKELETON)
            unit_mass /= 10;
        break;

    default:
    case OBJ_GOLD:
        unit_mass = 0;
        break;
    }

    return ((unit_mass > 0) ? unit_mass : 0);
}

// Note that this function, an item sizes in general aren't quite on the
// same scale as PCs and monsters.
size_type item_size( const item_def &item )
{
    int size = SIZE_TINY;

    switch (item.base_type)
    {
    case OBJ_WEAPONS:
    case OBJ_STAVES:
        size = Weapon_prop[ Weapon_index[item.sub_type] ].fit_size - 1;
        break;

    case OBJ_ARMOUR:
        size = SIZE_MEDIUM;
        switch (item.sub_type)
        {
        case ARM_GLOVES:
        case ARM_HELMET:
        case ARM_CAP:
        case ARM_BOOTS:
        case ARM_BUCKLER:
            // tiny armour
            break;

        case ARM_SHIELD:
            size = SIZE_LITTLE;
            break;

        case ARM_LARGE_SHIELD:
            size = SIZE_SMALL;
            break;

        default:        // body armours and bardings
            size = SIZE_MEDIUM;
            break;
        }
        break;

    case OBJ_MISSILES:
        if (item.sub_type == MI_LARGE_ROCK)
            size = SIZE_SMALL;
        break;

    case OBJ_MISCELLANY:
        if (item.sub_type == MISC_PORTABLE_ALTAR_OF_NEMELEX
            || item.sub_type == MISC_ROD_OF_STRIKING)
        {
            size = SIZE_SMALL;
        }
        break;

    case OBJ_CORPSES:
        size = mons_size( item.plus, PSIZE_BODY );
        break;

    default:            // sundry tiny items
        break;
    }

    if (size < SIZE_TINY)
        size = SIZE_TINY;
    else if (size > SIZE_HUGE)
        size = SIZE_HUGE;

    return (static_cast<size_type>( size ));
}

// returns true if we might be interested in dumping the colour
bool is_colourful_item( const item_def &item )
{
    bool ret = false;

    switch (item.base_type)
    {
    case OBJ_ARMOUR:
        if (item.sub_type == ARM_ROBE
            || item.sub_type == ARM_CLOAK
            || item.sub_type == ARM_CAP
            || item.sub_type == ARM_NAGA_BARDING
            || item.sub_type == ARM_CENTAUR_BARDING)
        {
            ret = true;
        }
        break;

    default:
        break;
    }

    return (ret);
}
