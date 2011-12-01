/*
 *  File:       religion.cc
 *  Summary:    Misc religion related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *
 *   <7>   11jan2001     gdl    added M. Valvoda's changes to
 *                              god_colour() and god_name()
 *   <6>   06-Mar-2000   bwr    added penance, gift_timeout,
 *                              divine_retribution(), god_speaks()
 *   <5>   11/15/99      cdl    Fixed Daniel's yellow Xom patch  :)
 *                              Xom will sometimes answer prayers
 *   <4>   10/11/99      BCR    Added Daniel's yellow Xom patch
 *   <3>    6/13/99      BWR    Vehumet book giving code.
 *   <2>    5/20/99      BWR    Added screen redraws
 *   <1>    -/--/--      LRH    Created
 */

#include "AppHdr.h"
#include "religion.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "globals.h"
#include "externs.h"

#include "abl-show.h"
#include "beam.h"
#include "debug.h"
#include "decks.h"
#include "describe.h"
#include "dungeon.h"
#include "effects.h"
#include "food.h"
#include "item_use.h"
#include "it_use2.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "misc.h"
#include "monplace.h"
#include "mon-util.h"
#include "mutation.h"
#include "newgame.h"
#include "ouch.h"
#include "player.h"
#include "randart.h"
#include "shopping.h"
#include "skills2.h"
#include "spells1.h"
#include "spells2.h"
#include "spells3.h"
#include "spl-cast.h"
#include "stuff.h"
#include "view.h"

const char *sacrifice[] = {
    " glows silver and disappears.",                    // Zin
    " glows a brilliant golden colour and disappears.", // Shining One
    " rots away in an instant.",                        // Kiku
    " crumbles to dust.",                               // Yred
    " is eaten by a bug.",                              // Xom (never)
    " explodes into nothingness.",                      // Vehumet
    " is consumed in a burst of flame.",                // Okawaru
    " is consumed in a roaring column of flame.",       // Makhleb
    " glows faintly for a moment, then is gone.",       // Sif Muna
    " is consumed in a roaring column of flame.",       // Trog
    " glows with a rainbow of weird colours and disappears.",  // Nemelex
    " evaporates."                                       // Elyvilon
};

void altar_prayer(void);
void dec_penance( int god, int val );
void divine_retribution( int god );
void inc_penance( int god, int val );
void inc_penance( int val );

void dec_penance( int god, int val )
{
    if (you.penance[god] > 0)
    {
        if (you.penance[god] <= val)
        {
            simple_god_message(" seems mollified.", god);
            you.penance[god] = 0;
            set_redraw_status( REDRAW_SKILL );
        }
        else
            you.penance[god] -= val;
    }
}                               // end dec_penance()

void dec_penance( int val )
{
    dec_penance( you.religion, val );
}                               // end dec_penance()

void inc_penance( int god, int val )
{
    const bool need_redraw = (you.penance[god] == 0 && god == you.religion);

    if (you.penance[god] + val > 200)
        you.penance[god] = 200;
    else
        you.penance[god] += val;

    if (need_redraw)
        set_redraw_status( REDRAW_SKILL );
}                               // end inc_penance()

void inc_penance( int val )
{
    inc_penance(you.religion, val);
}                               // end inc_penance()

static void inc_gift_timeout( int val )
{
    // XXX: this gets saved out as (and currently is) an unsigned char!
    if (static_cast<int>(you.gift_timeout) + val > 250)
        you.gift_timeout = 250;
    else
        you.gift_timeout += val;
}                               // end inc_gift_timeout()

void pray( void )
{
    int            temp_rand = 0;
    unsigned char  was_praying = you.duration[DUR_PRAYER];
    bool           success = false;

    if (silenced( you.x_pos, you.y_pos ))
    {
        mpr("You are unable to make a sound!");
        return;
    }

    // all prayers take time
    you.turn_is_over = true;

    if (you.religion != GOD_NO_GOD
        && grd[you.x_pos][you.y_pos] == DNGN_ALTAR_ZIN + you.religion - 1)
    {
        altar_prayer();
    }
    else if (grd[you.x_pos][you.y_pos] >= DNGN_ALTAR_ZIN
                && grd[you.x_pos][you.y_pos] <= DNGN_ALTAR_ELYVILON)
    {
        if (you.species == SP_DEMIGOD)
        {
            mpr("Sorry, a being of your status cannot worship here.");
            return;
        }

        god_pitch( grd[you.x_pos][you.y_pos] - DNGN_ALTAR_ZIN + 1 );
        return;
    }

    if (you.religion == GOD_NO_GOD)
    {
        strcpy(info, "You spend a moment contemplating the meaning of ");

        if (you.is_undead)
            strcat(info, "un");

        strcat(info, "life.");
        mpr(info);
        return;
    }
    else if (you.religion == GOD_XOM)
    {
        if (one_chance_in(100))
        {
            // Every now and then, Xom listens
            // This is for flavour, not effect, so praying should not be
            // encouraged.

            // Xom is nicer to experienced players
            bool nice = (27 <= random2( 27 + you.xp_level ));

            // and he's not very nice even then
            int sever = (nice) ? random2( random2( you.xp_level ) )
                               : you.xp_level;

            // bad results are enforced, good are not
            bool force = !nice;

            Xom_acts( nice, 1 + sever, force );
        }
        else
            mpr("Xom ignores you.");

        return;
    }

    strcpy( info, "You offer a prayer to " );
    strcat( info, god_name( you.religion ) );
    strcat( info, "." );
    mpr( info );

    you.duration[DUR_PRAYER] = 9 + (random2(you.piety) / 20)
                                            + (random2(you.piety) / 20);

    if (player_under_penance())
        simple_god_message( " demands penance!" );
    else
    {
        strcpy(info, god_name(you.religion));
        strcat(info, " is ");

        strcat(info, (you.piety > 130) ? "exalted by your worship" :
                     (you.piety > 100) ? "extremely pleased with you" :
                     (you.piety >  70) ? "greatly pleased with you" :
                     (you.piety >  40) ? "most pleased with you" :
                     (you.piety >  20) ? "pleased with you" :
                     (you.piety >   5) ? "noncommittal"
                                       : "displeased");

        strcat(info, ".");
        god_speaks(you.religion, info);

        if (you.piety > 130)
            you.duration[DUR_PRAYER] *= 3;
        else if (you.piety > 70)
            you.duration[DUR_PRAYER] *= 2;
    }

#if DEBUG_DIAGNOSTICS
    mpr( MSGCH_DIAGNOSTICS, "piety: %d", you.piety );
#endif

    // Consider a gift if we don't have a timeout and weren't
    // already praying when we prayed.
    if (!you.penance[you.religion] && !you.gift_timeout && !was_praying)
    {
        //   Remember to check for water/lava
        switch (you.religion)
        {
        default:
            break;

        case GOD_ZIN:
            if (you.hunger_state == HS_STARVING
                && random2(250) <= you.piety)
            {
                god_speaks( you.religion, "Your stomach feels content." );
                set_hunger( 6000, true );
                lose_piety( 5 + roll_dice(2,5) );
                inc_gift_timeout( 30 + roll_dice(2,10) );
            }
            break;

        case GOD_NEMELEX_XOBEH:
            if (you.piety > 20  // must be "pleased" not "noncommital"
                && random2(200) <= you.piety
                && (!you.attribute[ATTR_CARD_TABLE] || one_chance_in(3))
                && !grid_destroys_items( grd[you.x_pos][you.y_pos] ))
            {
                int thing_created = NON_ITEM;
                unsigned char gift_type = MISC_DECK_OF_TRICKS;

                if (!you.attribute[ATTR_CARD_TABLE])
                {
                    thing_created = make_item( 1, OBJ_MISCELLANY,
                                               MISC_PORTABLE_ALTAR_OF_NEMELEX,
                                               true, 1, 250 );

                    if (thing_created != NON_ITEM)
                        you.attribute[ATTR_CARD_TABLE] = 1;
                }
                else
                {
                    if (one_chance_in(3))
                        gift_type = MISC_DECK_OF_SUMMONINGS;

                    if (random2(200) <= you.piety && one_chance_in(3))
                    {
                        gift_type = (one_chance_in(3) ? MISC_DECK_OF_WONDERS
                                                      : MISC_DECK_OF_POWER);
                    }

                    thing_created = make_item( 1, OBJ_MISCELLANY, gift_type,
                                               true, 1, 250 );
                }

                if (thing_created != NON_ITEM)
                {
                    move_item_to_grid( &thing_created, you.x_pos, you.y_pos );

                    simple_god_message( " grants you a gift!" );
                    more();
                    canned_msg( MSG_SOMETHING_APPEARS );

                    inc_gift_timeout( 5 + roll_dice(2,5) );
                    you.num_gifts[you.religion]++;
                    mitm[thing_created].flags |= ISFLAG_GOD_GIFT;
                }
            }
            break;

        case GOD_OKAWARU:
            if (you.piety > 80
                && random2( you.piety ) > 70
                && !grid_destroys_items( grd[you.x_pos][you.y_pos] )
                && one_chance_in(4)
                && you.skills[ best_skill(SK_SLINGS, SK_RANGED_COMBAT) ] >= 3)
            {
                success = acquirement( OBJ_MISSILES, true );
                if (success)
                {
                    simple_god_message( " has granted you a gift!" );
                    more();

                    inc_gift_timeout( 4 + roll_dice(2,4) );
                    you.num_gifts[ you.religion ]++;
                }
                break;
            }
            // intentional fall through

        case GOD_TROG:
            if (you.piety > 130         // must be exhalted
                && random2( you.piety ) > 120
                && !grid_destroys_items( grd[you.x_pos][you.y_pos] )
                && one_chance_in(4))
            {
                const int class_wanted = coinflip() ? OBJ_WEAPONS : OBJ_ARMOUR;

                success = acquirement( class_wanted, true );
                if (success)
                {
                    simple_god_message(" has granted you a gift!");
                    more();

                    inc_gift_timeout( 30 + roll_dice(2,10) );
                    you.num_gifts[ you.religion ]++;
                }
            }
            break;

        case GOD_YREDELEMNUL:
            // must be "greatly pleased"
            if (random2( you.piety ) > 80 && one_chance_in(5))
            {
                int thing_called = MONS_PROGRAM_BUG;  // error trapping {dlb}

                temp_rand = random2(100);
                thing_called = ((temp_rand > 66) ? MONS_WRAITH :          // 33%
                                (temp_rand > 52) ? MONS_WIGHT :           // 12%
                                (temp_rand > 40) ? MONS_SPECTRAL_WARRIOR :// 16%
                                (temp_rand > 31) ? MONS_ROTTING_HULK :    //  9%
                                (temp_rand > 23) ? MONS_SKELETAL_WARRIOR ://  8%
                                (temp_rand > 16) ? MONS_FLAYED_GHOST :    //  7%
                                (temp_rand > 10) ? MONS_GHOUL :           //  6%
                                (temp_rand >  4) ? MONS_MUMMY             //  6%
                                                 : MONS_VAMPIRE);         //  5%

                if (create_monster( thing_called, BEH_FRIENDLY ) != -1)
                {
                    simple_god_message(" grants you an undead servant!");
                    more();
                    inc_gift_timeout( 4 + roll_dice(2,4) );
                    you.num_gifts[you.religion]++;
                }
            }
            break;

        case GOD_KIKUBAAQUDGHA:
        case GOD_SIF_MUNA:
        case GOD_VEHUMET:
            // must be a Champion
            if (you.piety > 160 && random2( you.piety ) > 100)
            {
                unsigned int gift = NUM_BOOKS;

                switch (you.religion)
                {
                case GOD_KIKUBAAQUDGHA:     // gives death books
                    if (!you.had_book[BOOK_NECROMANCY])
                        gift = BOOK_NECROMANCY;
                    else if (!you.had_book[BOOK_DEATH])
                        gift = BOOK_DEATH;
                    else if (!you.had_book[BOOK_UNLIFE])
                        gift = BOOK_UNLIFE;
                    else if (!you.had_book[BOOK_NECRONOMICON])
                        gift = BOOK_NECRONOMICON;
                    break;

                case GOD_SIF_MUNA:
                    gift = OBJ_RANDOM;      // Sif Muna - gives any
                    break;

                // Vehumet - gives conj/summ. books (higher skill first)
                case GOD_VEHUMET:
                    if (!you.had_book[BOOK_CONJURATIONS_I])
                        gift = give_first_conjuration_book();
                    else if (!you.had_book[BOOK_POWER])
                        gift = BOOK_POWER;
                    else if (!you.had_book[BOOK_ANNIHILATIONS])
                        gift = BOOK_ANNIHILATIONS;  // conj books

                    if (you.skills[SK_CONJURATIONS] < you.skills[SK_SUMMONINGS]
                        || gift == NUM_BOOKS)
                    {
                        if (!you.had_book[BOOK_CALLINGS])
                            gift = BOOK_CALLINGS;
                        else if (!you.had_book[BOOK_SUMMONINGS])
                            gift = BOOK_SUMMONINGS;
                        else if (!you.had_book[BOOK_DEMONOLOGY])
                            gift = BOOK_DEMONOLOGY; // summoning bks
                    }
                    break;
                }

                if (gift != NUM_BOOKS
                    && !grid_destroys_items( grd[you.x_pos][you.y_pos] ))
                {
                    if (gift == OBJ_RANDOM)
                        success = acquirement( OBJ_BOOKS, true );
                    else
                    {
                        int thing_created = make_item( 1, OBJ_BOOKS, gift,
                                                       true, 1, 250);

                        if (thing_created != NON_ITEM)
                        {
                            success = true;
                            move_item_to_grid( &thing_created,
                                               you.x_pos, you.y_pos );
                            mitm[thing_created].flags |= ISFLAG_GOD_GIFT;
                        }
                    }

                    if (success)
                    {
                        simple_god_message(" has granted you a gift!");
                        more();

                        // Vehumet gives books less readily
                        inc_gift_timeout( 40 + roll_dice(2,10) );
                        you.num_gifts[you.religion]++;

                        if (you.religion == GOD_VEHUMET)
                            inc_gift_timeout( 10 + roll_dice(1,10) );
                    }
                }                   // end of giving book
            }                       // end of book gods
            break;
        }                       // end of switch religion
    }                           // end of gift giving
}                               // end pray()

char *god_name( int which_god, bool long_name ) // mv - rewritten
{
    static char godname_buff[80];

    switch (which_god)
    {
    case GOD_NO_GOD:
        sprintf(godname_buff, "No God");
        break;
    case GOD_ZIN:
        sprintf(godname_buff, "Zin%s", long_name ? " the Law-Giver" : "");
        break;
    case GOD_SHINING_ONE:
        sprintf(godname_buff, "The Shining One");
        break;
    case GOD_KIKUBAAQUDGHA:
        strcpy(godname_buff, "Kikubaaqudgha");
        break;
    case GOD_YREDELEMNUL:
        sprintf(godname_buff, "Yredelemnul%s", long_name ? " the Dark" : "");
        break;
    case GOD_XOM:
        strcpy(godname_buff, "Xom");
        if (long_name)
        {
            strcat(godname_buff, " ");
            switch(random2(1000))
            {
            default:
                strcat(godname_buff, "of Chaos");
                break;
            case 1:
                strcat(godname_buff, "the Random");
                if (coinflip())
                    strcat(godname_buff, coinflip()?"master":" Number God");
                break;
            case 2:
                strcat(godname_buff, "the Tricky");
                break;
            case 3:
                sprintf( godname_buff, "Xom the %sredictible", coinflip() ? "Less-P"
                                                                    : "Unp" );
                break;
            case 4:
                strcat(godname_buff, "of Many Doors");
                break;
            case 5:
                strcat(godname_buff, "the Capricious");
                break;
            case 6:
                strcat(godname_buff, "of ");
                strcat(godname_buff, coinflip() ? "Bloodstained" : "Enforced");
                strcat(godname_buff, " Whimsey");
                break;
            case 7:
                strcat(godname_buff, "\"What was your username?\" *clickity-click*");
                break;
            case 8:
                strcat(godname_buff, "of Bone-Dry Humour");
                break;
            case 9:
                strcat(godname_buff, "of ");
                strcat(godname_buff, coinflip() ? "Malevolent" : "Malicious");
                strcat(godname_buff, " Giggling");
                break;
            case 10:
                strcat(godname_buff, "the Psycho");
                strcat(godname_buff, coinflip() ? "tic" : "path");
                break;
            case 11:
                strcat(godname_buff, "of ");
                switch(random2(5))
                {
                case 0: strcat(godname_buff, "Gnomic"); break;
                case 1: strcat(godname_buff, "Ineffable"); break;
                case 2: strcat(godname_buff, "Fickle"); break;
                case 3: strcat(godname_buff, "Swiftly Tilting"); break;
                case 4: strcat(godname_buff, "Unknown"); break;
                }
                strcat(godname_buff, " Intent");
                if (coinflip())
                    strcat(godname_buff, "ion");
                break;
            case 12:
                sprintf(godname_buff, "The Xom-Meister");
                if (coinflip())
                    strcat(godname_buff, ", Xom-a-lom-a-ding-dong");
                else if (coinflip())
                    strcat(godname_buff, ", Xom-o-Rama");
                else if (coinflip())
                    strcat(godname_buff, ", Xom-Xom-bo-Bom, Banana-Fana-fo-Fom");
                break;
            case 13:
                strcat(godname_buff, "the Begetter of ");
                strcat(godname_buff, coinflip() ? "Turbulence" : "Discontinuities");
                break;
            }
        }
        break;
    case GOD_VEHUMET:
        strcpy(godname_buff, "Vehumet");
        break;
    case GOD_OKAWARU:
        sprintf(godname_buff, "%sOkawaru", long_name ? "Warmaster " : "");
        break;
    case GOD_MAKHLEB:
        sprintf(godname_buff, "Makhleb%s", long_name ? " the Destroyer" : "");
        break;
    case GOD_SIF_MUNA:
        sprintf(godname_buff, "Sif Muna%s", long_name ? " the Loreminder" : "");
        break;
    case GOD_TROG:
        sprintf(godname_buff, "Trog%s", long_name ? " the Wrathful" : "");
        break;
    case GOD_NEMELEX_XOBEH:
        strcpy(godname_buff, "Nemelex Xobeh");
        break;
    case GOD_ELYVILON:
        sprintf(godname_buff, "Elyvilon%s", long_name ? " the Healer" : "");
        break;
    default:
        sprintf(godname_buff, "The Buggy One (%d)", which_god);
    }

    return (godname_buff);
}                               // end god_name()

void god_speaks( int god, const char *mesg )
{
    mpr( MSGCH_GOD, god, mesg );
}                               // end god_speaks()

void Xom_acts( bool niceness, int sever, bool force_sever )
{
    // niceness = false - bad, true - nice
    int temp_rand;              // probability determination {dlb}
    bool done_bad = false;      // flag to clarify logic {dlb}
    bool done_good = false;     // flag to clarify logic {dlb}

    struct bolt beam;

    if (sever < 1)
        sever = 1;

    if (!force_sever)
        sever = random2(sever);

    if (sever == 0)
        return;

  okay_try_again:

    if (!niceness || one_chance_in(3))
    {
        // begin "Bad Things"
        done_bad = false;

        // this should always be first - it will often be called
        // deliberately, with a low sever value
        if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "Xom notices you." :
                (temp_rand == 1) ? "Xom's attention turns to you for a moment.":
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            miscast_effect( SPTYP_RANDOM, 5 + random2(10), random2(100), 0,
                            "the capriciousness of Xom" );

            done_bad = true;
        }
        else if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Suffer!\"" :
                (temp_rand == 1) ? "Xom's malign attention turns to you for a moment." :
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            lose_stat(STAT_RANDOM, 1 + random2(3), true);

            done_bad = true;
        }
        else if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "Xom notices you." :
                (temp_rand == 1) ? "Xom's attention turns to you for a moment.":
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            miscast_effect( SPTYP_RANDOM, 5 + random2(15), random2(250), 0,
                            "the capriciousness of Xom" );

            done_bad = true;
        }
        else if (!you.is_undead && random2(sever) <= 3)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"You need some minor adjustments, mortal!\"" :
                (temp_rand == 1) ? "\"Let me alter your pitiful body.\"" :
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            mpr("Your body is suffused with distortional energy.");

            temp_rand = 1 + random2( you.hp );
            if (temp_rand < you.hp_max / 2)
                temp_rand = you.hp_max / 2;

            set_hp( temp_rand, false );

            bool failMsg = true;
            for (int i = 0; i < 4; i++)
            {
                if (!mutate( PICK_RANDOM_MUTATION, false, failMsg ))
                    failMsg = false;
            }

            done_bad = true;
        }
        else if (!you.is_undead && random2(sever) <= 3)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"You have displeased me, mortal.\"" :
                (temp_rand == 1) ? "\"You have grown too confident for your meagre worth.\"" :
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            if (one_chance_in(4))
            {
                drain_exp();
                if (random2(sever) > 3)
                    drain_exp();
                if (random2(sever) > 3)
                    drain_exp();
            }
            else
            {
                mpr("A wave of agony tears through your body!");
                set_hp(1 + (you.hp / 2), false);
            }

            done_bad = true;
        }
        else if (random2(sever) <= 3)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Time to have some fun!\"" :
                (temp_rand == 1) ? "\"Fight to survive, mortal.\"" :
                (temp_rand == 2) ? "\"Let's see if it's strong enough to survive yet.\""
                                 : "You hear Xom's maniacal laughter.");

            if (one_chance_in(4))
                dancing_weapon(100, true);      // nasty, but fun
            else
            {
                create_monster(MONS_NEQOXEC + random2(5), BEH_HOSTILE, 3);

                if (one_chance_in(3))
                    create_monster(MONS_NEQOXEC + random2(5), BEH_HOSTILE, 3);

                if (one_chance_in(4))
                    create_monster(MONS_NEQOXEC + random2(5), BEH_HOSTILE, 3);

                if (one_chance_in(3))
                    create_monster(MONS_HELLION + random2(10), BEH_HOSTILE, 3);

                if (one_chance_in(4))
                    create_monster(MONS_HELLION + random2(10), BEH_HOSTILE, 3);
            }

            done_bad = true;
        }
        else if (you.depth == 0)
        {
            // this should remain the last possible outcome {dlb}
            temp_rand = random2(3);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"You have grown too comfortable in your little world, mortal!\"" :
                (temp_rand == 1) ? "Xom casts you into the Abyss!"
                                 : "The world seems to spin as Xom's maniacal laughter rings in your ears.");

            banished( DNGN_ENTER_ABYSS );
            done_bad = true;
        }
    }                           // end "Bad Things"
    else
    {
        // begin "Good Things"
        done_good = false;

// Okay, now for the nicer stuff (note: these things are not necessarily nice):
        if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Go forth and destroy!\"" :
                (temp_rand == 1) ? "\"Go forth and destroy, mortal!\"" :
                (temp_rand == 2) ? "Xom grants you a minor favour."
                                 : "Xom smiles on you.");

            switch (random2(7))
            {
            case 0:
                potion_effect(POT_HEALING, 150);
                break;
            case 1:
                potion_effect(POT_HEAL_WOUNDS, 150);
                break;
            case 2:
                potion_effect(POT_SPEED, 150);
                break;
            case 3:
                potion_effect(POT_MIGHT, 150);
                break;
            case 4:
                potion_effect(POT_INVISIBILITY, 150);
                break;
            case 5:
                if (one_chance_in(6))
                    potion_effect(POT_EXPERIENCE, 150);
                else
                {
                    you.berserk_penalty = NO_BERSERK_PENALTY;
                    potion_effect(POT_BERSERK_RAGE, 150);
                }
                break;
            case 6:
                you.berserk_penalty = NO_BERSERK_PENALTY;
                potion_effect(POT_BERSERK_RAGE, 150);
                break;
            }

            done_good = true;
        }
        else if (random2(sever) <= 4)
        {
            temp_rand = random2(3);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Serve the mortal, my children!\"" :
                (temp_rand == 1) ? "Xom grants you some temporary aid."
                                 : "Xom opens a gate.");

            create_monster( MONS_NEQOXEC + random2(5), BEH_FRIENDLY, 3 );
            create_monster( MONS_NEQOXEC + random2(5), BEH_FRIENDLY, 3 );

            if (random2( you.xp_level ) >= 8)
                create_monster( MONS_NEQOXEC + random2(5), BEH_FRIENDLY, 3 );

            if (random2( you.xp_level ) >= 8)
                create_monster( MONS_HELLION + random2(10), BEH_FRIENDLY, 3 );

            if (random2( you.xp_level ) >= 8)
                create_monster( MONS_HELLION + random2(10), BEH_FRIENDLY, 3 );

            done_good = true;
        }
        else if (random2(sever) <= 3)
        {
            temp_rand = random2(3);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Take this token of my esteem.\"" :
                (temp_rand == 1) ? "Xom grants you a gift!"
                                 : "Xom's generous nature manifests itself.");

            if (grid_destroys_items( grd[you.x_pos][you.y_pos] ))
            {
                // How unfortunate. I'll bet Xom feels sorry for you.
                mpr("You hear a splash.");
            }
            else
            {
                int thing_created = make_item( 1, OBJ_RANDOM, OBJ_RANDOM, true,
                                               you.xp_level * 3, 250 );

                move_item_to_grid( &thing_created, you.x_pos, you.y_pos );

                if (thing_created != NON_ITEM)
                {
                    canned_msg(MSG_SOMETHING_APPEARS);
                    more();
                }
            }

            done_good = true;
        }
        else if (random2(sever) <= 4)
        {
            const int demon = (random2(you.xp_level) < 6)
                                                ? MONS_WHITE_IMP + random2(5)
                                                : MONS_NEQOXEC + random2(5);

            if (create_monster( demon, BEH_FRIENDLY ) != -1)
            {
                temp_rand = random2(3);

                god_speaks(GOD_XOM,
                    (temp_rand == 0) ? "\"Serve the mortal, my child!\"" :
                    (temp_rand == 1) ? "Xom grants you a demonic servitor."
                                     : "Xom opens a gate.");
            }

            done_good = true;   // well, for Xom, trying == doing {dlb}
        }
        else if (random2(sever) <= 4)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Take this instrument of destruction!\"" :
                (temp_rand == 1) ? "\"You have earned yourself a gift.\"" :
                (temp_rand == 2) ? "Xom grants you an implement of death."
                                 : "Xom smiles on you.");

            if (acquirement( OBJ_WEAPONS ))
                more();

            done_good = true;
        }
        else if (!you.is_undead && random2(sever) <= 5)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"You need some minor adjustments, mortal!\"" :
                (temp_rand == 1) ? "\"Let me alter your pitiful body.\"" :
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal chuckling.");

            mpr("Your body is suffused with distortional energy.");

            temp_rand = 1 + random2( you.hp );
            if (temp_rand < you.hp_max / 2)
                temp_rand = you.hp_max / 2;

            set_hp( temp_rand, false );

            if (coinflip() || give_cosmetic_mutation() == MUT_NONE)
                give_good_mutation();

            done_good = true;
        }
        else if (random2(sever) <= 2)
        {
            // this should remain the last possible outcome {dlb}
            if (!one_chance_in(8))
                you.attribute[ATTR_DIVINE_LIGHTNING_PROTECTION] = 1;

            god_speaks(GOD_XOM, "The area is suffused with divine lightning!");

            beam.type = SYM_BURST;
            beam.damage = dice_def( 3, 30 );
            beam.flavour = BEAM_ELECTRICITY;
            beam.target_x = you.x_pos;
            beam.target_y = you.y_pos;
            strcpy(beam.name, "blast of lightning");
            beam.colour = LIGHTCYAN;
            beam.beam_source = MHITYOU;
            beam.thrower = KILL_YOU;    // your explosion
            beam.aux_source = "Xom's lightning strike";
            beam.ex_size = 2;
            beam.is_tracer = false;

            explosion(beam);

            if (you.attribute[ATTR_DIVINE_LIGHTNING_PROTECTION] == 1)
            {
                mpr("Your divine protection wanes.");
                you.attribute[ATTR_DIVINE_LIGHTNING_PROTECTION] = 0;
            }

            done_good = true;
        }
    }                           // end "Good Things"

    if (done_bad || done_good || one_chance_in(4))
        return;
    else
        goto okay_try_again;
}                               // end Xom_acts()

void gain_piety( int pgn )
{
    // check to see if we owe anything first
    if (you.penance[you.religion] > 0)
    {
        dec_penance(pgn);
        return;
    }
    else if (you.gift_timeout > 0)
    {
        if (you.gift_timeout > pgn)
            you.gift_timeout -= pgn;
        else
            you.gift_timeout = 0;

        // Slow down piety gain to account for the fact that gifts
        // no longer have a piety cost for getting them
        if (!one_chance_in(8))
            return;
    }

    // slow down gain at upper levels of piety
    if (you.piety > 199
        || (you.piety > 150 && one_chance_in(3))
        || (you.piety > 100 && one_chance_in(3)))
    {
        return;
    }

    int old_piety = you.piety;

    you.piety += pgn;
    if (you.piety > 200)
        you.piety = 200;

    if (you.piety >= 30 && old_piety < 30)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_NEMELEX_XOBEH:
        case GOD_SIF_MUNA:
            break;
        default:
            strcpy(info, "You can now ");
            strcat(info,
                    (you.religion == GOD_ZIN || you.religion == GOD_SHINING_ONE)
                            ? "repel the undead" :

                    (you.religion == GOD_KIKUBAAQUDGHA)
                            ? "recall your undead slaves" :
                    (you.religion == GOD_YREDELEMNUL)
                            ? "animate corpses" :
                    (you.religion == GOD_VEHUMET)
                            ? "gain power from killing in Vehumet's name" :
                    (you.religion == GOD_MAKHLEB)
                            ? "gain power from killing in Makhleb's name" :
                        (you.religion == GOD_OKAWARU)
                                ? "give your great, but temporary, body strength" :
                    (you.religion == GOD_TROG)
                            ? "go berserk at will" :
                    (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon for minor healing"
                // Unknown god
                            : "endure this program bug @30");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 50 && old_piety < 50)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_NEMELEX_XOBEH:
            break;
        case GOD_KIKUBAAQUDGHA:
            simple_god_message(" is protecting you from some side-effects of death magic.");
            break;

        case GOD_VEHUMET:
            god_speaks(you.religion, "You can call upon Vehumet to aid your destructive magics with prayer.");
            break;

        default:
            strcpy(info, "You can now ");

            strcat(info,
                   (you.religion == GOD_ZIN)
                           ? "call upon Zin for minor healing" :
                   (you.religion == GOD_SHINING_ONE)
                           ? "smite your foes" :
                   (you.religion == GOD_YREDELEMNUL)
                           ? "recall your undead slaves" :
                   (you.religion == GOD_OKAWARU)
                           ? "call upon Okawaru for minor healing" :
                   (you.religion == GOD_MAKHLEB)
                           ? "harness Makhleb's destructive might" :
                   (you.religion == GOD_SIF_MUNA)
                           ? "freely open your mind to new spells" :
                   (you.religion == GOD_TROG)
                           ? "give your body great, but temporary, strength" :
                   (you.religion == GOD_ELYVILON)
                           ? "call upon Elyvilon for purification"
                   // Unknown god
                           : "endure this program bug @50");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 75 && old_piety < 75)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_OKAWARU:
        case GOD_NEMELEX_XOBEH:
        case GOD_SIF_MUNA:
        case GOD_TROG:
            break;
        case GOD_VEHUMET:
            god_speaks(you.religion,"During prayer you have some protection from summoned creatures.");
            break;

        default:
            strcpy(info, "You can now ");
            strcat(info,
                     (you.religion == GOD_ZIN)
                                 ? "call down a plague" :
                     (you.religion == GOD_SHINING_ONE)
                                 ? "dispel the undead" :
                     (you.religion == GOD_KIKUBAAQUDGHA)
                                 ? "permanently enslave the undead" :
                     (you.religion == GOD_YREDELEMNUL)
                                 ? "animate legions of the dead" :
                     (you.religion == GOD_MAKHLEB)
                                 ? "summon a lesser servant of Makhleb" :
                     (you.religion == GOD_ELYVILON)
                                 ? "call upon Elyvilon for moderate healing"
                     // Unknown god
                                 : "endure this program bug @75");
            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 100 && old_piety < 100)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_OKAWARU:
        case GOD_NEMELEX_XOBEH:
        case GOD_KIKUBAAQUDGHA:
            break;
        case GOD_SIF_MUNA:
            simple_god_message
                (" is protecting you from some side-effects of spellcasting.");
            break;

        default:
            strcpy(info, "You can now ");

            strcat(info,
                        (you.religion == GOD_ZIN)
                                ? "utter a Holy Word" :
                        (you.religion == GOD_SHINING_ONE)
                                ? "hurl bolts of divine anger" :
                        (you.religion == GOD_YREDELEMNUL)
                                ? "drain ambient lifeforce" :
                        (you.religion == GOD_VEHUMET)
                                ? "tap ambient magical fields" :
                        (you.religion == GOD_MAKHLEB)
                                ? "hurl Makhleb's greater destruction" :
                        (you.religion == GOD_TROG)
                                ? "haste yourself" :
                        (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon to restore your abilities"
                        // Unknown god
                                : "endure this program bug @100");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 120 && old_piety < 120)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_NEMELEX_XOBEH:
        case GOD_VEHUMET:
        case GOD_SIF_MUNA:
        case GOD_TROG:
            break;
        default:
            strcpy(info, "You can now ");

            strcat(info,
                     (you.religion == GOD_ZIN)
                                ? "summon a guardian angel" :
                     (you.religion == GOD_SHINING_ONE)
                                ? "summon a divine warrior" :
                     (you.religion == GOD_KIKUBAAQUDGHA)
                                ? "summon an emissary of Death" :
                     (you.religion == GOD_YREDELEMNUL)
                                ? "control the undead" :
                     (you.religion == GOD_OKAWARU)
                                ? "haste yourself" :
                     (you.religion == GOD_MAKHLEB)
                                ? "summon a greater servant of Makhleb" :
                     (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon for incredible healing"
                     // Unknown god
                                : "endure this program bug @120");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }
}                               // end gain_piety()

void lose_piety( int pgn )
{
    int old_piety = you.piety;

    if (you.piety - pgn < 0)
        you.piety = 0;
    else
        you.piety -= pgn;

    // Don't bother printing out these messages if you're under
    // penance, you wouldn't notice since all these abilities
    // are withheld.
    if (!player_under_penance() && you.piety != old_piety)
    {
        if (you.piety < 120 && old_piety >= 120)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_NEMELEX_XOBEH:
            case GOD_VEHUMET:
            case GOD_SIF_MUNA:
            case GOD_TROG:
                break;
            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                           (you.religion == GOD_ZIN)
                                ? "summon guardian angels" :
                           (you.religion == GOD_SHINING_ONE)
                                ? "summon divine warriors" :
                           (you.religion == GOD_KIKUBAAQUDGHA)
                                ? "summon Death's emissaries" :
                           (you.religion == GOD_YREDELEMNUL)
                                ? "control undead beings" :
                           (you.religion == GOD_OKAWARU)
                                ? "haste yourself" :
                           (you.religion == GOD_MAKHLEB)
                                ? "summon a greater servant of Makhleb" :
                           (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon for incredible healing"
                           // Unknown god
                                : "endure this program bug @120");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 100 && old_piety >= 100)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_OKAWARU:
            case GOD_NEMELEX_XOBEH:
            case GOD_KIKUBAAQUDGHA:
                break;
            case GOD_SIF_MUNA:
                god_speaks(you.religion,"Sif Muna is no longer protecting you from miscast magic.");
                break;
            default:
                strcpy(info, "You can no longer ");
                strcat(info,
                        (you.religion == GOD_ZIN)
                            ? "utter a Holy Word" :
                        (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon to restore your abilities" :
                        (you.religion == GOD_SHINING_ONE)
                            ? "hurl bolts of divine anger" :
                        (you.religion == GOD_YREDELEMNUL)
                            ? "drain ambient life force" :
                        (you.religion == GOD_VEHUMET)
                            ? "tap ambient magical fields" :
                        (you.religion == GOD_MAKHLEB)
                            ? "direct Makhleb's greater destructive powers" :
                        (you.religion == GOD_TROG)
                            ? "haste yourself"
                        // Unknown god
                            : "endure this program bug @100");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 75 && old_piety >= 75)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_OKAWARU:
            case GOD_NEMELEX_XOBEH:
            case GOD_SIF_MUNA:
            case GOD_TROG:
                break;
            case GOD_VEHUMET:
                simple_god_message(" will longer shield you from summoned creatures.");
                break;
            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                       (you.religion == GOD_ZIN)
                                ? "call down a plague" :
                       (you.religion == GOD_SHINING_ONE)
                                ? "dispel undead" :
                       (you.religion == GOD_KIKUBAAQUDGHA)
                                ? "enslave undead" :
                       (you.religion == GOD_YREDELEMNUL)
                                ? "animate legions of the dead" :
                       (you.religion == GOD_MAKHLEB)
                                ? "summon a servant of Makhleb" :
                       (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon for moderate healing"
                       // Unknown god
                                : "endure this program bug @75");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 50 && old_piety >= 50)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_NEMELEX_XOBEH:
                break;
            case GOD_KIKUBAAQUDGHA:
                simple_god_message(" is no longer shielding you from miscast death magic.");
                break;
            case GOD_VEHUMET:
                simple_god_message(" will no longer aid your destructive magics.");
                break;

            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                       (you.religion == GOD_ZIN)
                            ? "call upon Zin for minor healing" :
                       (you.religion == GOD_SHINING_ONE)
                            ? "smite your foes" :
                       (you.religion == GOD_YREDELEMNUL)
                            ? "recall your undead slaves" :
                       (you.religion == GOD_OKAWARU)
                            ? "call upon Okawaru for minor healing" :
                       (you.religion == GOD_MAKHLEB)
                            ? "hurl Makhleb's destruction" :
                       (you.religion == GOD_SIF_MUNA)
                            ? "forget spells at will" :
                       (you.religion == GOD_TROG)
                            ? "give your body great, but temporary, strength" :
                       (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon for Purification"
                       // Unknown god
                            : "endure this program bug @50");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 30 && old_piety >= 30)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_NEMELEX_XOBEH:
            case GOD_SIF_MUNA:
                break;
            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                    (you.religion == GOD_ZIN || you.religion == GOD_SHINING_ONE)
                            ? "repel the undead" :
                    (you.religion == GOD_KIKUBAAQUDGHA)
                            ? "recall your undead slaves" :
                    (you.religion == GOD_YREDELEMNUL)
                            ? "animate corpses" :
                    (you.religion == GOD_VEHUMET)
                            ? "gain power from killing in Vehumet's name" :
                    (you.religion == GOD_MAKHLEB)
                            ? "gain power from killing in Makhleb's name" :
                    (you.religion == GOD_OKAWARU)
                            ? "give your body great, but temporary, strength" :
                    (you.religion == GOD_TROG)
                            ? "go berserk at will" :
                    (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon for minor healing."
                    // Unknown god
                            : "endure this program bug @30");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }
    }
}                               // end lose_piety()

// This function is the merger of done_good() and naughty().
// Returns true if god was interested (good or bad) in conduct.
bool did_god_conduct( int thing_done, int level )
{
    bool ret = false;
    int piety_change = 0;
    int penance = 0;

    if (you.religion == GOD_NO_GOD || you.religion == GOD_XOM)
        return (false);

    switch (thing_done)
    {
    case DID_NECROMANCY:
    case DID_UNHOLY:
    case DID_ATTACK_HOLY:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_ELYVILON:
            piety_change = -level;
            penance = level * ((you.religion == GOD_ZIN) ? 2 : 1);
            ret = true;
            break;
        }
        break;

    case DID_STABBING:
    case DID_POISON:
        if (you.religion == GOD_SHINING_ONE)
        {
            ret = true;
            piety_change = -level;
            penance = level * 2;
        }
        break;

    case DID_ATTACK_FRIEND:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_ELYVILON:
        case GOD_OKAWARU:
            piety_change = -level;
            penance = level * 3;
            ret = true;
            break;
        }
        break;

    case DID_FRIEND_DIES:
        switch (you.religion)
        {
        case GOD_ELYVILON:
            penance = level;    // healer god cares more about this
            // fall through
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_OKAWARU:
            piety_change = -level;
            ret = true;
            break;
        }
        break;

    case DID_DEDICATED_BUTCHERY:  // aka field sacrifice
        switch (you.religion)
        {
        case GOD_ELYVILON:
            simple_god_message(" did not appreciate that!");
            ret = true;
            piety_change = -10;
            penance = 10;
            break;

        case GOD_OKAWARU:
        case GOD_MAKHLEB:
        case GOD_TROG:
            simple_god_message(" accepts your offering.");
            ret = true;
            if (random2(level + 10) > 5)
                piety_change = 1;
            break;
        }
        break;

    case DID_DEDICATED_KILL_LIVING:
        switch (you.religion)
        {
        case GOD_ELYVILON:
            simple_god_message(" did not appreciate that!");
            ret = true;
            piety_change = -level;
            penance = level * 2;
            break;

        case GOD_KIKUBAAQUDGHA:
        case GOD_YREDELEMNUL:
        case GOD_OKAWARU:
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
        case GOD_TROG:
            simple_god_message(" accepts your kill.");
            ret = true;
            if (random2(level + 18) > 5)
                piety_change = 1;
            break;
        }
        break;

    case DID_DEDICATED_KILL_UNDEAD:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_OKAWARU:
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
            simple_god_message(" accepts your kill.");
            ret = true;
            if (random2(level + 18) > 4)
                piety_change = 1;
            break;
        }
        break;

    case DID_DEDICATED_KILL_DEMON:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_OKAWARU:
            simple_god_message(" accepts your kill.");
            ret = true;
            if (random2(level + 18) > 3)
                piety_change = 1;
            break;
        }
        break;

    case DID_DEDICATED_KILL_WIZARD:
        if (you.religion == GOD_TROG)
        {
            // hooking this up, but is it too good?
            // enjoy it while you can -- bwr
            simple_god_message(" appreciates your killing of a magic user.");
            ret = true;
            if (random2(level + 10) > 5)
                piety_change = 1;
        }
        break;

    // Note that Angel deaths are special, they are always noticed...
    // if you or any friendly kills one you'll get the credit or the blame.
    case DID_ANGEL_KILLED_BY_SERVANT:
    case DID_KILL_ANGEL:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_ELYVILON:
            level *= 3;
            piety_change = -level;
            penance = level * ((you.religion == GOD_ZIN) ? 2 : 1);
            ret = true;
            break;

        case GOD_KIKUBAAQUDGHA:
        case GOD_YREDELEMNUL:
        case GOD_MAKHLEB:
            snprintf( info, INFO_SIZE, " accepts your %skill.",
                      (thing_done == DID_KILL_ANGEL) ? "" : "collateral " );

            simple_god_message( info );

            ret = true;
            if (random2(level + 18) > 2)
                piety_change = 1;
            break;
        }
        break;

    // Undead slave is any friendly undead... Kiku and Yred pay attention
    // to the undead and both like the death of living things.
    case DID_LIVING_KILLED_BY_UNDEAD_SLAVE:
        switch (you.religion)
        {
        case GOD_KIKUBAAQUDGHA:
        case GOD_YREDELEMNUL:
            simple_god_message(" accepts your slave's kill.");
            ret = true;
            if (random2(level + 10) > 5)
                piety_change = 1;
            break;
        }
        break;

    // Servants are currently any friendly monster under Vehumet, or
    // any god given pet for everyone else (excluding undead which are
    // handled above).
    case DID_LIVING_KILLED_BY_SERVANT:
        switch (you.religion)
        {
        case GOD_KIKUBAAQUDGHA: // note: reapers aren't undead
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
            simple_god_message(" accepts your collateral kill.");
            ret = true;
            if (random2(level + 10) > 5)
                piety_change = 1;
            break;
        }
        break;

    case DID_UNDEAD_KILLED_BY_SERVANT:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
            simple_god_message(" accepts your collateral kill.");
            ret = true;
            if (random2(level + 10) > 5)
                piety_change = 1;
            break;
        }
        break;

    case DID_DEMON_KILLED_BY_SERVANT:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
            simple_god_message(" accepts your collateral kill.");
            ret = true;
            if (random2(level + 10) > 5)
                piety_change = 1;
            break;
        }
        break;

    case DID_SPELL_MEMORISE:
        if (you.religion == GOD_TROG)
        {
            penance = level * 10;
            piety_change = -penance;
            ret = true;
        }
        break;

    case DID_SPELL_CASTING:
        if (you.religion == GOD_TROG)
        {
            piety_change = -level;
            penance = level * 5;
            ret = true;
        }
        break;

    case DID_SPELL_PRACTISE:
        // Like CAST, but for skill advancement.
        // Level is number of skill points gained... typically 10 * exerise,
        // but may be more/less if the skill is at 0 (INT adjustment), or
        // if the PC's pool is low and makes change.
        if (you.religion == GOD_SIF_MUNA)
        {
            // Old curve: random2(12) <= spell-level, this is similar,
            // but faster at low levels (to help ease things for low level
            // Power averages about (level * 20 / 3) + 10 / 3 now.  Also
            // note that spell skill practise comes just after XP gain, so
            // magical kills tend to do both at the same time (unlike melee).
            // This means high level spells probably work pretty much like
            // they used to (use spell, get piety).
            piety_change = div_rand_round( level + 10, 90 );
            ret = true;
        }
        break;

    case DID_CARDS:
        if (you.religion == GOD_NEMELEX_XOBEH)
        {
            piety_change = level;
            ret = true;
        }
        break;

    case DID_STIMULANTS:                        // unused
    case DID_EAT_MEAT:                          // unused
    case DID_CREATED_LIFE:                      // unused
    case DID_DEDICATED_KILL_NATURAL_EVIL:       // unused
    case DID_NATURAL_EVIL_KILLED_BY_SERVANT:    // unused
    case DID_SPELL_NONUTILITY:                  // unused
    default:
        break;
    }

    if (piety_change > 0)
    {
        // positive conduct only interesting out in the real world
        if (!player_in_branch( BRANCH_ECUMENICAL_TEMPLE ))
            gain_piety( piety_change );
        else if (one_chance_in(10))
        {
            simple_god_message( " says: \"Go forth into the world to show your devotion to me!\"" );
        }
    }
    else
    {
        const int piety_loss = -piety_change;

        if (piety_loss)
        {
            // output guilt message:
            mpr( "You feel%sguilty.",
                    (piety_loss == 1) ? " a little " :
                    (piety_loss <  5) ? " " :
                    (piety_loss < 10) ? " very "
                                      : " extremely " );

            lose_piety( piety_loss );
        }

        if (you.piety < 1)
            excommunication();
        else if (penance)       // only if still in religion
        {
            god_speaks( you.religion,
                        "\"You will pay for your transgression, mortal!\"" );

            inc_penance( penance );
        }
    }

#if DEBUG_DIAGNOSTICS
    if (ret)
    {
        static const char *conducts[] =
        {
          "Necromancy", "Unholy", "Attack Holy", "Attack Friend",
          "Friend Died", "Stab", "Poison", "Field Sacrifice",
          "Kill Living", "Kill Undead", "Kill Demon", "Kill Wizard",
          "Kill Priest", "Kill Angel", "Undead Slave Kill Living",
          "Servant Kill Living", "Servant Kill Undead",
          "Servant Kill Demon", "Servant Kill Angel",
          "Spell Memorise", "Spell Cast", "Spell Practise", "Spell Nonutility",
          "Cards", "Stimulants", "Eat Meat", "Create Life"
        };

        mpr( MSGCH_DIAGNOSTICS,
             "conduct: %s; piety: %d (%+d); penance: %d (%+d)",
             conducts[thing_done],
             you.piety, piety_change, you.penance[you.religion], penance );

    }
#endif

    return (ret);
}

void divine_retribution( int god )
{
    ASSERT(god != GOD_NO_GOD);

    int loopy = 0;              // general purpose loop variable {dlb}
    int temp_rand = 0;          // probability determination {dlb}
    int punisher = MONS_PROGRAM_BUG;
    bool success = false;
    int how_many = 0;
    int divine_hurt = 0;

    // Good gods don't use divine retribution on their followers, they
    // will consider it for those who have gone astray however.
    if (god == you.religion)
    {
        if (god == GOD_SHINING_ONE || god == GOD_ZIN || god == GOD_ELYVILON)
            return;
    }

    // Just the thought of retribution (getting this far) mollifies the
    // god by a point... the punishment might reduce penance further.
    int amount = (you.species == SP_MUMMY) ? 1 : 1 + random2(3);

    switch (god)
    {
    case GOD_XOM:
        {
            // One in ten chance that Xom might do something good...
            // but that isn't forced, bad things are though
            bool nice = one_chance_in(30);
            bool force = !nice;

            Xom_acts(nice, you.xp_level, force);
        }
        break;

    case GOD_SHINING_ONE:
        // daeva/smiting theme
        // Doesn't care unless you've gone over to evil/destructive gods
        if (you.religion == GOD_KIKUBAAQUDGHA || you.religion == GOD_MAKHLEB
            || you.religion == GOD_YREDELEMNUL || you.religion == GOD_VEHUMET)
        {
            if (coinflip())
            {
                success = false;
                how_many = 1 + random2(you.xp_level / 5) + random2(3);

                for (loopy = 0; loopy < how_many; loopy++)
                {
                    if (create_monster( MONS_DAEVA ) != -1)
                        success = true;
                }

                if (success)
                {
                    simple_god_message( " sends the divine host to punish you for your evil ways!", god );
                }
            }
            else
            {
                divine_hurt = 10 + random2(10);

                for (loopy = 0; loopy < 5; loopy++)
                    divine_hurt += random2( you.xp_level );

                if (!player_under_penance() && you.piety > random2(400))
                {
                    strcpy(info, "Mortal, I have averted the wrath of "
                        "the Shining One... this time.");
                    god_speaks(you.religion, info);
                }
                else
                {
                    simple_god_message( " smites you!", god );
                    ouch( divine_hurt, 0, KILLED_BY_TSO_SMITING );
                    amount++;
                }
            }
        }
        break;

    case GOD_ZIN:
        // angels/creeping doom theme:
        // Doesn't care unless you've gone over to evil
        if (you.religion == GOD_KIKUBAAQUDGHA || you.religion == GOD_MAKHLEB
            || you.religion == GOD_YREDELEMNUL || you.religion == GOD_VEHUMET)
        {
            if (random2(you.xp_level) > 7 && !one_chance_in(5))
            {
                success = false;
                how_many = 1 + (you.xp_level / 10) + random2(3);

                for (loopy = 0; loopy < how_many; loopy++)
                {
                    if (create_monster( MONS_ANGEL ) != -1)
                        success = true;
                }

                if (success)
                {
                    simple_god_message(" sends the divine host to punish you for your evil ways!", god);
                }
            }
            else
            {
                // god_gift == false gives unfriendly
                summon_swarm( you.xp_level * 20, true, false );
                simple_god_message(" sends a plague down upon you!", god);
            }
        }
        break;

    case GOD_MAKHLEB:
        // demonic servant theme
        if (random2(you.xp_level) > 7 && !one_chance_in(5))
        {
            if (create_monster( MONS_EXECUTIONER + random2(5) ) != -1)
                simple_god_message(" sends a greater servant after you!", god);
        }
        else
        {
            success = false;
            how_many = 1 + (you.xp_level / 7);

            for (loopy = 0; loopy < how_many; loopy++)
            {
                if (create_monster( MONS_NEQOXEC + random2(5) ) != -1)
                    success = true;
            }

            if (success)
                simple_god_message(" sends minions to punish you.", god);
        }
        break;

    case GOD_KIKUBAAQUDGHA:
        // death/necromancy theme
        if (random2(you.xp_level) > 7 && !one_chance_in(5))
        {
            success = false;
            how_many = 1 + (you.xp_level / 5) + random2(3);

            for (loopy = 0; loopy < how_many; loopy++)
            {
                if (create_monster( MONS_REAPER ) != -1)
                    success = true;
            }

            if (success)
                simple_god_message(" unleashes Death upon you!", god);
        }
        else
        {
            god_speaks(god, (coinflip()) ? "You hear Kikubaaqudgha cackling."
                                         : "Kikubaaqudgha's malice focuses upon you.");

            miscast_effect( SPTYP_NECROMANCY, 5 + you.xp_level, roll_dice(3,30),
                            100, "the malice of Kikubaaqudgha" );
        }
        break;

    case GOD_YREDELEMNUL:
        // undead theme
        if (random2(you.xp_level) > 4)
        {
            success = false;
            how_many = 1 + random2(1 + (you.xp_level / 5));

            for (loopy = 0; loopy < how_many; loopy++)
            {
                temp_rand = random2(100);

                punisher = ((temp_rand > 66) ? MONS_WRAITH :            // 33%
                            (temp_rand > 52) ? MONS_WIGHT :             // 12%
                            (temp_rand > 40) ? MONS_SPECTRAL_WARRIOR :  // 16%
                            (temp_rand > 31) ? MONS_ROTTING_HULK :      //  9%
                            (temp_rand > 23) ? MONS_SKELETAL_WARRIOR :  //  8%
                            (temp_rand > 16) ? MONS_VAMPIRE :           //  7%
                            (temp_rand > 10) ? MONS_GHOUL :             //  6%
                            (temp_rand >  4) ? MONS_MUMMY               //  6%
                                             : MONS_FLAYED_GHOST);      //  5%

                if (create_monster( punisher ) != -1)
                    success = true;
            }

            if (success)
                simple_god_message(" sends a servant to punish you.", god);
        }
        else
        {
            simple_god_message("'s anger turns toward you for a moment.",
                               god);

            miscast_effect( SPTYP_NECROMANCY, 5 + you.xp_level,
                            roll_dice(3,30), 100, "the anger of Yredelemnul" );
        }
        break;

    case GOD_TROG:
        // physical/berserk theme
        switch (random2(6))
        {
        case 0:
        case 1:
        case 2:
            {
                // Would be better if berserking monsters were available,
                // we just send some big bruisers for now.
                success = false;

                int points = 3 + you.xp_level * 3;

                while (points > 0)
                {
                    if (points > 20 && coinflip())
                    {
                        // quick reduction for large values
                        punisher = MONS_DEEP_TROLL;
                        points -= 15;
                        break;
                    }
                    else
                    {
                        switch (random2(10))
                        {
                        case 0:
                            punisher = MONS_IRON_TROLL;
                            points -= 10;
                            break;

                        case 1:
                            punisher = MONS_ROCK_TROLL;
                            points -= 10;
                            break;

                        case 2:
                            punisher = MONS_TROLL;
                            points -= 6;
                            break;

                        case 3:
                        case 4:
                            punisher = MONS_MINOTAUR;
                            points -= 3;
                            break;

                        case 5:
                        case 6:
                            punisher = MONS_TWO_HEADED_OGRE;
                            points -= 4;
                            break;

                        case 7:
                        case 8:
                        case 9:
                        default:
                            punisher = MONS_OGRE;
                            points -= 3;
                        }
                    }

                    if (create_monster( punisher ) != -1)
                        success = true;
                }

                if (success)
                    simple_god_message(" sends monsters to punish you.", god);
            }
            break;

        case 3:
        case 4:
            simple_god_message("'s voice booms out, \"Feel my wrath!\"", god );

            // A collection of physical effects that might be better
            // suited to Trog than wild fire magic... messages could
            // be better here... something more along the lines of apathy
            // or loss of rage to go with the anti-berzerk effect-- bwr
            switch (random2(6))
            {
            case 0:
                potion_effect( POT_DECAY, 100 );
                break;

            case 1:
            case 2:
                lose_stat(STAT_STRENGTH, 1 + random2(you.str / 5), true);
                break;

            case 3:
                if (!you.paralysis)
                {
                    mpr( MSGCH_WARN, "You suddenly pass out!" );
                    paralyse_player( 2 + random2(6), true );
                    amount += 3;
                }
                break;

            case 4:
            case 5:
                fatigue_player( 60 );
                amount++;
                break;
            };
            break;
        //jmf: returned Trog's old Fire damage
        // -- actually, this function partially exists to remove that,
        //    we'll leave this effect in, but we'll remove the wild
        //    fire magic. -- bwr
        case 5:
            mpr( MSGCH_WARN, "You feel Trog's fiery rage upon you!" );
            miscast_effect( SPTYP_FIRE, 8 + you.xp_level,
                            roll_dice(3,33), 100, "the fiery rage of Trog" );
            amount += 2;
            break;
        }
        break;

    case GOD_OKAWARU:
        {
            // warrior theme:
            success = false;
            how_many = 1 + (you.xp_level / 5);

            for (loopy = 0; loopy < how_many; loopy++)
            {
                temp_rand = random2(100);

                punisher = ((temp_rand > 84) ? MONS_ORC_WARRIOR :
                            (temp_rand > 69) ? MONS_ORC_KNIGHT :
                            (temp_rand > 59) ? MONS_NAGA_WARRIOR :
                            (temp_rand > 49) ? MONS_CENTAUR_WARRIOR :
                            (temp_rand > 39) ? MONS_STONE_GIANT :
                            (temp_rand > 29) ? MONS_FIRE_GIANT :
                            (temp_rand > 19) ? MONS_FROST_GIANT :
                            (temp_rand >  9) ? MONS_CYCLOPS :
                            (temp_rand >  4) ? MONS_HILL_GIANT
                                             : MONS_TITAN);

                if (create_monster( punisher ) != -1)
                    success = true;
            }

            if (success)
                simple_god_message(" sends forces against you!", god);
        }
        break;

    case GOD_VEHUMET:
        // conjuration and summoning theme
        simple_god_message("'s vengence finds you.", god);
        miscast_effect( coinflip() ? SPTYP_CONJURATION : SPTYP_SUMMONING,
                        8 + you.xp_level, roll_dice(3,33), 100,
                        "the wrath of Vehumet" );
        break;

    case GOD_NEMELEX_XOBEH:
        // like Xom, this might actually help the player -- bwr`
        simple_god_message(" makes you draw from the Deck of Punishment.", god);
        deck_of_cards( DECK_OF_PUNISHMENT );
        break;

    case GOD_SIF_MUNA:
        simple_god_message("'s wrath finds you.", god);

        // magic and intelligence theme:
        switch (random2(10))
        {
        case 0:
        case 1:
            lose_stat(STAT_INTELLIGENCE, 1 + random2( you.intel / 5 ), true);
            break;

        case 2:
        case 3:
        case 4:
            confuse_player( 3 + random2(10), false );
            break;

        case 5:
        case 6:
            miscast_effect( SK_CONJURATIONS + random2(12),
                            9, 90, 100, "the will of Sif Muna" );
            break;

        case 7:
        case 8:
            if (you.magic_points)
            {
                dec_mp( 100 );  // this should zero it.
                mpr( MSGCH_WARN, "You suddenly feel drained of magical energy!" );
            }
            break;

        case 9:
            // This will set all the extendable duration spells to
            // a duration of one round, thus potentially exposing
            // the player to real danger.
            antimagic();
            mpr( MSGCH_WARN, "You sense a dampening of magic." );
            break;
        }
        break;

    case GOD_ELYVILON:  // Elyvilon doesn't seek revenge
    default:
        return;
    }

    // penance reduction is less in less interesting areas
    if (you.level_type == LEVEL_LABYRINTH
        || player_in_branch( BRANCH_ECUMENICAL_TEMPLE )
        || player_in_branch( BRANCH_VESTIBULE_OF_HELL )
        || (god != you.religion
            && you.level_type == LEVEL_DUNGEON
            && roll_dice(2, you.xp_level) - 1 > you.depth ))
    {
        amount /= 2;
    }

    if (amount)
        dec_penance( god, amount );

    // Sometimes divine experiences are overwelming...
    if (one_chance_in(5) && you.xp_level < random2(37))
    {
        if (coinflip())
            confuse_player( 3 + random2(10) );
        else
            fatigue_player( 5 + random2(20) );
    }

    return;
}                               // end divine_retribution()

void excommunication(void)
{
    const int old_god = you.religion;

    you.duration[DUR_PRAYER] = 0;
    you.religion = GOD_NO_GOD;
    you.piety = 0;
    set_redraw_status( REDRAW_SKILL );

    mpr("You have lost your religion!");
    more();

    switch (old_god)
    {
    case GOD_XOM:
        Xom_acts( false, (you.xp_level * 2), true );
        inc_penance( old_god, 50 );
        break;

    case GOD_KIKUBAAQUDGHA:
        simple_god_message( " does not appreciate desertion!", old_god );
        miscast_effect( SPTYP_NECROMANCY, 9 + you.xp_level,
                        90, 100, "the malice of Kikubaaqudgha" );
        inc_penance( old_god, 30 );
        break;

    case GOD_YREDELEMNUL:
        simple_god_message( " does not appreciate desertion!", old_god );
        miscast_effect( SPTYP_NECROMANCY, 9 + you.xp_level,
                        90, 100, "the anger of Yredelemnul" );
        inc_penance( old_god, 30 );
        break;

    case GOD_VEHUMET:
        simple_god_message( " does not appreciate desertion!", old_god );
        miscast_effect( (coinflip() ? SPTYP_CONJURATION : SPTYP_SUMMONING),
                        8 + you.xp_level, 90, 100,
                        "the wrath of Vehumet" );
        inc_penance( old_god, 25 );
        break;

    case GOD_MAKHLEB:
        simple_god_message( " does not appreciate desertion!", old_god );
        miscast_effect( (coinflip() ? SPTYP_CONJURATION : SPTYP_SUMMONING),
                        8 + you.xp_level, 90, 100,
                        "the fury of Makhleb" );
        inc_penance( old_god, 25 );
        break;

    case GOD_TROG:
        simple_god_message( " does not appreciate desertion!", old_god );

        // Penence has to come before retribution to prevent "mollify"
        inc_penance( old_god, 50 );
        divine_retribution( old_god );
        break;

    // these like to haunt players for a bit more than the standard
    case GOD_NEMELEX_XOBEH:
        simple_god_message( " does not appreciate desertion!", old_god );
        simple_god_message( " makes you draw from the Deck of Punishment.",
                            old_god );

        deck_of_cards( DECK_OF_PUNISHMENT );
        inc_penance( old_god, 50 );
        break;

    case GOD_SIF_MUNA:
        simple_god_message( " does not appreciate desertion!", old_god );
        miscast_effect( SPTYP_CONJURATION + random2(12),
                        8 + you.xp_level, 90, 100,
                        "the will of Sif Muna" );

        lose_stat( STAT_INTELLIGENCE, roll_dice(2, 3), true );
        forget_spells();

        inc_penance( old_god, 50 );
        break;

    default:
        inc_penance( old_god, 25 );
        break;

    case GOD_ELYVILON:  // never seeks revenge
        break;
    }
}                               // end excommunication()

static bool bless_weapon( god_type god, brand_type brand, int colour )
{
    const int wpn = get_inv_wielded();

    // Assuming the type of weapon is correct, we only need to check
    // to see if it's an artefact we can successfully clobber:
    if (!is_fixed_artefact( you.inv[wpn] )
        && (!is_random_artefact( you.inv[wpn] )
            || unmake_item_randart( you.inv[wpn] )))
    {
        you.duration[DUR_WEAPON_BRAND] = 0;     // just in case

        set_equip_desc( you.inv[wpn], ISFLAG_GLOWING );
        set_item_ego_type( you.inv[wpn], OBJ_WEAPONS, brand );
        you.inv[wpn].colour = colour;
        you.inv[wpn].flags |= ISFLAG_GOD_GIFT;

        do_uncurse_item( you.inv[wpn] );
        enchant_weapon( ENCHANT_TO_HIT, true );
        enchant_weapon( ENCHANT_TO_DAM, true );

        set_redraw_status( REDRAW_WIELD );
        you.num_gifts[god]++;

        you.flash_colour = colour;
        viewwindow( true, false );

        mpr( MSGCH_GOD, "Your weapon shines brightly!" );
        simple_god_message( " booms: Use this gift wisely!" );

        // as currently only Zin and TSO do this is our permabrand effect:
        holy_word( 100, true );

        more();
        mesclr();

        return (true);
    }

    return (false);
}

void altar_prayer(void)
{
    int  i, j, next;
    char subst_id[NUM_IDTYPE][MAX_SUBTYPES];
    char str_pass[ITEMNAME_SIZE];

    mpr( "You kneel at the altar and pray." );

    // Xom does nothing here
    if (you.religion == GOD_XOM)
        return;

    // TSO blesses long swords with holy wrath
    if (you.religion == GOD_SHINING_ONE
        && !you.num_gifts[GOD_SHINING_ONE]
        && !player_under_penance()
        && you.piety > 160)
    {
        const int wpn = get_inv_wielded();

        if (wpn != -1
            && weapon_skill( you.inv[wpn] ) == SK_LONG_SWORDS
            && get_weapon_brand( you.inv[wpn] ) != SPWPN_HOLY_WRATH)
        {
            if (bless_weapon( GOD_SHINING_ONE, SPWPN_HOLY_WRATH, YELLOW ))
            {
                // convert those demon blades if blessed:
                if (you.inv[wpn].sub_type == WPN_DEMON_BLADE)
                    you.inv[wpn].sub_type = WPN_BLESSED_BLADE;
            }
        }
    }

    // Zin blesses maces with disruption
    if (you.religion == GOD_ZIN
        && !you.num_gifts[GOD_ZIN]
        && !player_under_penance()
        && you.piety > 160)
    {
        const int wpn = get_inv_wielded();

        if (wpn != -1
            && (you.inv[wpn].base_type == OBJ_WEAPONS
                && (you.inv[wpn].sub_type == WPN_MACE
                    || you.inv[wpn].sub_type == WPN_GREAT_MACE))
            && get_weapon_brand( you.inv[wpn] ) != SPWPN_DISRUPTION)
        {
            bless_weapon( GOD_ZIN, SPWPN_DISRUPTION, WHITE );
        }
    }

    // prepare the id array for getting true item values
    for (i = 0; i < NUM_IDTYPE; i++)
    {
        for (j = 0; j < MAX_SUBTYPES; j++)
        {
            subst_id[i][j] = ID_KNOWN_TYPE;
        }
    }

    // Now we do item sacrifices:
    i = igrd[you.x_pos][you.y_pos];
    j = 0;

    while (i != NON_ITEM && j < 100)
    {
        next = mitm[i].link;  // in case we can't get it later.

        const int value = item_value( mitm[i], subst_id, true );

        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_OKAWARU:
        case GOD_MAKHLEB:
        case GOD_NEMELEX_XOBEH:
            it_name(i, DESC_CAP_THE, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            if (mitm[i].base_type == OBJ_CORPSES
                || random2(value) >= 50
                || player_under_penance())
            {
                gain_piety(1);
            }

            destroy_item(i);
            break;

        case GOD_SIF_MUNA:
            it_name(i, DESC_CAP_THE, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            if (value >= 150)
                gain_piety(1 + random2(3));

            destroy_item(i);
            break;

        case GOD_KIKUBAAQUDGHA:
        case GOD_TROG:
            if (mitm[i].base_type != OBJ_CORPSES)
                break;

            it_name(i, DESC_CAP_THE, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            gain_piety(1);
            destroy_item(i);
            break;

        case GOD_ELYVILON:
            if (mitm[i].base_type != OBJ_WEAPONS
                && mitm[i].base_type != OBJ_MISSILES)
            {
                break;
            }

            it_name(i, DESC_CAP_THE, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            if (random2(value) >= random2(50)
                || (mitm[i].base_type == OBJ_WEAPONS
                    && (you.piety < 30 || player_under_penance())))
            {
                gain_piety(1);
            }

            destroy_item(i);
            break;

        default:
            break;
        }

        i = next;
        j++;
    }
}                               // end altar_prayer()

void god_pitch( unsigned char which_god )
{
    strcpy(info, "You kneel at the altar of ");
    strcat(info, god_name(which_god));
    strcat(info, ".");
    mpr(info);

    more();

    // Note: using worship we could make some gods not allow followers to
    // return, or not allow worshippers from other religions.  -- bwr

    if ((you.is_undead || you.species == SP_DEMONSPAWN)
        && (which_god == GOD_ZIN || which_god == GOD_SHINING_ONE
            || which_god == GOD_ELYVILON))
    {
        simple_god_message(" does not accept worship from those such as you!",
                           which_god);
        return;
    }

    describe_god( which_god, false );

    if (you.religion == which_god)
    {
        redraw_screen();
        return;
    }

    snprintf( info, INFO_SIZE, "Do you wish to %sjoin this religion?",
              (you.worshipped[which_god]) ? "re" : "" );

    if (!yesno( info ))
    {
        redraw_screen();
        return;
    }

    if (!yesno("Are you sure?"))
    {
        redraw_screen();
        return;
    }

    redraw_screen();
    if (you.religion != GOD_NO_GOD)
        excommunication();

    you.religion = which_god;  //jmf: moved up so god_speaks gives right colour
    you.piety = 15;            // to prevent near instant excommunication
    you.gift_timeout = 0;
    dec_penance(10);
    set_god_ability_slots();   // remove old god's slots, reserve new god's

    snprintf( info, INFO_SIZE, " welcomes you%s!",
              (you.worshipped[which_god]) ? " back" : "" );

    simple_god_message( info );
    more();

    if (you.worshipped[you.religion] < 100)
        you.worshipped[you.religion]++;

    if (you.religion == GOD_KIKUBAAQUDGHA || you.religion == GOD_YREDELEMNUL
        || you.religion == GOD_VEHUMET || you.religion == GOD_MAKHLEB)
    {
        if (you.worshipped[GOD_SHINING_ONE] > 0)
        {
            inc_penance(GOD_SHINING_ONE, 30);
            god_speaks(GOD_SHINING_ONE, "\"You will pay for your evil ways, mortal!\"");
        }
    }

    set_redraw_status( REDRAW_SKILL );
}                               // end god_pitch()

// returns true if god accepted the sacrifice
bool offer_corpse( int corpse )
{
    ASSERT( is_valid_item( mitm[corpse] )
            && mitm[corpse].base_type == OBJ_CORPSES );

    if (you.duration[DUR_PRAYER]
        && did_god_conduct( DID_DEDICATED_BUTCHERY,
                            mons_class_hit_dice( mitm[corpse].plus ) ))
    {
        if (you.religion != GOD_ELYVILON) // doesn't really accept
        {
            char str_pass[ ITEMNAME_SIZE ];

            it_name( corpse, DESC_CAP_THE, str_pass );
            strcpy( info, str_pass );
            strcat( info, sacrifice[you.religion - 1] );
            mpr( info );
        }

        destroy_item( corpse );

        return (true);
    }

    return (false);
}                               // end offer_corpse()

//jmf: moved stuff from items::handle_time()
void handle_god_time(void)
{
    if (one_chance_in(75))
    {
        // Choose a god randomly from those to whom we owe penance.
        //
        // Proof: (By induction)
        //
        // 1) n = 1, probability of choosing god is one_chance_in(1)
        // 2) Asuume true for n = k (ie. prob = 1 / n for all n)
        // 3) For n = k + 1,
        //
        //      P:new-found-god = 1 / n (see algorithm)
        //      P:other-gods = (1 - P:new-found-god) * P:god-at-n=k
        //                             1        1
        //                   = (1 - -------) * ---
        //                           k + 1      k
        //
        //                          k         1
        //                   = ----------- * ---
        //                        k + 1       k
        //
        //                       1       1
        //                   = -----  = ---
        //                     k + 1     n
        //
        // Therefore, by induction the probability is uniform.  As for
        // why we do it this way... it requires only one pass and doesn't
        // require an array.

        int which_god = GOD_NO_GOD;
        unsigned int count = 0;

        for (int i = GOD_NO_GOD; i < NUM_GODS; i++)
        {
            if (you.penance[i])
            {
                count++;
                if (one_chance_in(count))
                    which_god = i;
            }
        }

        if (which_god != GOD_NO_GOD)
            divine_retribution(which_god);
    }

    // Update the god's opinion of the player
    if (you.religion != GOD_NO_GOD)
    {
        switch (you.religion)
        {
        case GOD_XOM:
            if (one_chance_in(75))
                Xom_acts(true, you.xp_level + random2(15), true);
            break;

        case GOD_ZIN:           // These gods like long-standing worshippers
        case GOD_ELYVILON:
            if (you.piety < 150 && one_chance_in(20))
                gain_piety(1);
            break;

        case GOD_SHINING_ONE:
            if (you.piety < 150 && one_chance_in(15))
                gain_piety(1);
            break;

        case GOD_OKAWARU: // These gods accept corpses, so they time-out faster
        case GOD_TROG:
            if (one_chance_in(14))
                lose_piety(1);
            break;

        case GOD_MAKHLEB:
        case GOD_SIF_MUNA:
            if (one_chance_in(16))
                lose_piety(1);
            break;

        case GOD_KIKUBAAQUDGHA:
            if (you.deaths_door) // to avoid falling below allowed HP
                break;
            // intentional fall-through
        case GOD_YREDELEMNUL:
        case GOD_VEHUMET:
            if (one_chance_in(17))
                lose_piety(1);
            break;

        case GOD_NEMELEX_XOBEH: // relatively patient
            if (one_chance_in(35))
                lose_piety(1);
            break;

        default:
            DEBUGSTR("Bad god, no bishop!");
        }

        if (you.religion != GOD_XOM && you.piety < 1)
            excommunication();
    }
}                               // end handle_god_time()

// yet another wrapper for mpr() {dlb}:
void simple_god_message(const char *event, int which_deity)
{
    char buff[ INFO_SIZE ];

    if (which_deity == GOD_NO_GOD)
        which_deity = you.religion;

    snprintf( buff, sizeof(buff), "%s%s", god_name( which_deity ), event );

    god_speaks( which_deity, buff );
}

char god_colour( char god ) //mv - added
{
    switch (god)
    {
    case GOD_SHINING_ONE:
    case GOD_ZIN:
    case GOD_ELYVILON:
    case GOD_OKAWARU:
        return(CYAN);

    case GOD_YREDELEMNUL:
    case GOD_KIKUBAAQUDGHA:
    case GOD_MAKHLEB:
    case GOD_VEHUMET:
    case GOD_TROG:
        return(LIGHTRED);

    case GOD_XOM:
        return(YELLOW);

    case GOD_NEMELEX_XOBEH:
        return(LIGHTMAGENTA);

    case GOD_SIF_MUNA:
        return(LIGHTBLUE);

    case GOD_NO_GOD:
    default:
        break;
    }

    return(YELLOW);
}
