/*
 *  File:     delay.cc
 *  Summary:  Functions for handling multi-turn actions.
 *
 *  Change History (most recent first):
 *
 * <1> Sept 09, 2001     BWR             Created
 */

#include "AppHdr.h"
#include "externs.h"

#include <stdio.h>
#include <string.h>

#include "delay.h"
#include "enum.h"
#include "fight.h"
#include "food.h"
#include "items.h"
#include "itemname.h"
#include "item_use.h"
#include "it_use2.h"
#include "message.h"
#include "misc.h"
#include "monstuff.h"
#include "ouch.h"
#include "output.h"
#include "player.h"
#include "randart.h"
#include "stuff.h"

void start_delay( int type, int turns, int parm1, int parm2 )
/***********************************************************/
{
    delay_queue_item  item;

    item.type = type;
    item.duration = turns;
    item.parm1 = parm1;
    item.parm2 = parm2;

    you.delay_queue.push( item );
}

void stop_delay( void )
/*********************/
{
    delay_queue_item  item = you.delay_queue.front();

    // At the very least we can remove any queued delays, right
    // now there is no problem with doing this... note that
    // any queuing here can only happen from a single command,
    // as the effect of a delay doesn't normally allow interaction
    // until it is done... it merely chains up individual actions
    // into a single action.  -- bwr
    if (you.delay_queue.size() > 1)
    {
        while (you.delay_queue.size())
            you.delay_queue.pop();

        you.delay_queue.push( item );
    }

    switch (item.type)
    {
    case DELAY_BUTCHER:         // lost work here... should be fixed
        mpr( "You stop butchering the corpse." );
        you.delay_queue.pop();
        break;

    case DELAY_MEMORIZE:        // losing work here is okay
        mpr( "Your memorization is interupted." );
        you.delay_queue.pop();
        break;

    case DELAY_PASSWALL:
        // The lost work here is okay since this spell requires
        // the player to "attune to the rock".  If changed, the
        // the delay should be increased to reduce the power of
        // this spell. -- bwr
        mpr( "Your mediation is interupted." );
        you.delay_queue.pop();
        break;

    case DELAY_INTERUPTABLE:  // always stopable
        you.delay_queue.pop();
        break;

    case DELAY_EAT:
        // XXX: Large problems with object destruction here... food can
        // be from in the inventory or on the ground and these are
        // still handled quite differently.  So for now, eating cannot
        // be stopped.  Would eventually like this to be stoppable,
        // with partial food items implimented. -- bwr
        break;

    case DELAY_ARMOUR_ON:
    case DELAY_ARMOUR_OFF:
        // These two have the default action of not being interuptable,
        // although they will often be chained (remove cloak, remove
        // armour, wear new armour, replace cloak), all of which can
        // be stopped when complete.  This is a fairly reasonable
        // behaviour, although perhaps the character should have
        // option of reversing the current action if it would take
        // less time to get out of the plate mail that's half on
        // than it would take to continue.  Probably too much trouble,
        // and would have to have a prompt... this works just fine. -- bwr
        break;

    case DELAY_AUTOPICKUP:      // one turn... too much trouble
    case DELAY_WEAPON_SWAP:     // one turn... too much trouble
    case DELAY_DROP_ITEM:       // only used for easy armour drops
    case DELAY_UNINTERUPTABLE:  // never stopable
    default:
        break;
    }
}

bool you_are_delayed( void )
/**************************/
{
    return (!you.delay_queue.empty());
}

int current_delay_action( void )
/******************************/
{
    return (you_are_delayed() ? you.delay_queue.front().type
                              : DELAY_NOT_DELAYED);
}

void handle_delay( void )
/***********************/
{
    char  str_pass[80];
    int   i;

    if (you_are_delayed())
    {
        delay_queue_item &item = you.delay_queue.front();

        if (item.duration > 0)
        {
#if DEBUG_DIAGNOSTICS
            snprintf( info, INFO_SIZE, "Delay type: %d   duration: %d",
                            item.type, item.duration );
            mpr( info );
#endif
            item.duration--;
        }
        else if (item.duration <= 0)
        {
            switch (item.type)
            {
            case DELAY_AUTOPICKUP:
                break;

            case DELAY_WEAPON_SWAP:
                weapon_switch( item.parm1 );
                break;

            case DELAY_ARMOUR_ON:
                set_ident_flags( you.inv[item.parm1], ISFLAG_EQ_ARMOUR_MASK );

                in_name( item.parm1, DESC_NOCAP_YOUR, str_pass );
                snprintf( info, INFO_SIZE, "You finish putting on %s.", str_pass );
                mpr(info);

                if (you.inv[item.parm1].sub_type < ARM_SHIELD
                    || you.inv[item.parm1].sub_type > ARM_LARGE_SHIELD)
                {
                    you.equip[EQ_BODY_ARMOUR] = item.parm1;

                    if (you.duration[DUR_ICY_ARMOUR] != 0)
                    {
                        mpr( "Your icy armour melts away.", MSGCH_DURATION );
                        you.redraw_armour_class = 1;
                        you.duration[DUR_ICY_ARMOUR] = 0;
                    }
                }
                else
                {
                    switch (you.inv[item.parm1].sub_type)
                    {
                    case ARM_BUCKLER:
                    case ARM_LARGE_SHIELD:
                    case ARM_SHIELD:
                        if (you.duration[DUR_CONDENSATION_SHIELD])
                        {
                            mpr( "Your icy shield evaporates.", MSGCH_DURATION );
                            you.duration[DUR_CONDENSATION_SHIELD] = 0;
                        }
                        you.equip[EQ_SHIELD] = item.parm1;
                        break;
                    case ARM_CLOAK:
                        you.equip[EQ_CLOAK] = item.parm1;
                        break;
                    case ARM_HELMET:
                        you.equip[EQ_HELMET] = item.parm1;
                        break;
                    case ARM_GLOVES:
                        you.equip[EQ_GLOVES] = item.parm1;
                        break;
                    case ARM_BOOTS:
                        you.equip[EQ_BOOTS] = item.parm1;
                        break;
                    }
                }

                if (you.inv[item.parm1].special != SPARM_NORMAL)
                {
                    switch (you.inv[item.parm1].special)
                    {
                    case SPARM_RUNNING:
                        strcpy(info, "You feel quick");
                        strcat(info, (you.species == SP_NAGA
                                || you.species == SP_CENTAUR) ? "." : " on your feet.");
                        mpr(info);
                        break;

                    case SPARM_FIRE_RESISTANCE:
                        mpr("You feel resistant to fire.");
                        break;

                    case SPARM_COLD_RESISTANCE:
                        mpr("You feel resistant to cold.");
                        break;

                    case SPARM_POISON_RESISTANCE:
                        mpr("You feel healthy.");
                        break;

                    case SPARM_SEE_INVISIBLE:
                        mpr("You feel perceptive.");
                        break;

                    case SPARM_DARKNESS:
                        if (!you.invis)
                            mpr("You become transparent for a moment.");
                        break;

                    case SPARM_STRENGTH:
                        modify_stat(STAT_STRENGTH, 3, false);
                        break;

                    case SPARM_DEXTERITY:
                        modify_stat(STAT_DEXTERITY, 3, false);
                        break;

                    case SPARM_INTELLIGENCE:
                        modify_stat(STAT_INTELLIGENCE, 3, false);
                        break;

                    case SPARM_PONDEROUSNESS:
                        mpr("You feel rather ponderous.");
                        // you.speed += 2;
                        you.redraw_evasion = 1;
                        break;

                    case SPARM_LEVITATION:
                        mpr("You feel rather light.");
                        break;

                    case SPARM_MAGIC_RESISTANCE:
                        mpr("You feel resistant to magic.");
                        break;

                    case SPARM_PROTECTION:
                        mpr("You feel protected.");
                        break;

                    case SPARM_STEALTH:
                        mpr("You feel stealthy.");
                        break;

                    case SPARM_RESISTANCE:
                        mpr("You feel resistant to extremes of temperature.");
                        break;

                    case SPARM_POSITIVE_ENERGY:
                        mpr("Your life-force is being protected.");
                        break;

                    case SPARM_ARCHMAGI:
                        if (!you.skills[SK_SPELLCASTING])
                            mpr("You feel strangely numb.");
                        else
                            mpr("You feel extremely powerful.");
                        break;
                    }
                }

                if (is_random_artefact( you.inv[item.parm1] ))
                    use_randart(item.parm1);

                you.redraw_armour_class = 1;
                you.redraw_evasion = 1;
                break;

            case DELAY_ARMOUR_OFF:
                in_name( item.parm1, DESC_NOCAP_YOUR, str_pass );
                snprintf( info, INFO_SIZE, "You finish taking off %s.", str_pass );
                mpr(info);

                if (you.inv[item.parm1].sub_type < ARM_SHIELD
                    || you.inv[item.parm1].sub_type > ARM_LARGE_SHIELD)
                {
                    you.equip[EQ_BODY_ARMOUR] = -1;
                }
                else
                {
                    switch (you.inv[item.parm1].sub_type)
                    {
                    case ARM_BUCKLER:
                    case ARM_LARGE_SHIELD:
                    case ARM_SHIELD:
                        if (item.parm1 == you.equip[EQ_SHIELD])
                            you.equip[EQ_SHIELD] = -1;
                        break;

                    case ARM_CLOAK:
                        if (item.parm1 == you.equip[EQ_CLOAK])
                            you.equip[EQ_CLOAK] = -1;
                        break;

                    case ARM_HELMET:
                        if (item.parm1 == you.equip[EQ_HELMET])
                            you.equip[EQ_HELMET] = -1;
                        break;


                    case ARM_GLOVES:
                        if (item.parm1 == you.equip[EQ_GLOVES])
                            you.equip[EQ_GLOVES] = -1;
                        break;

                    case ARM_BOOTS:
                        if (item.parm1 == you.equip[EQ_BOOTS])
                            you.equip[EQ_BOOTS] = -1;
                        break;
                    }
                }

                unwear_armour( item.parm1 );

                you.redraw_armour_class = 1;
                you.redraw_evasion = 1;
                break;

            case DELAY_EAT:
                mpr( "You finish eating." );
                break;

            case DELAY_MEMORIZE:
                mpr( "You finish memorising." );

                for (i = 0; i < 25; i++)
                {
                    if (you.spells[i] == SPELL_NO_SPELL)
                        break;
                }

                you.spells[i] = item.parm1;
                you.spell_no++;
                break;

            case DELAY_PASSWALL:
                {
                    mpr( "You finish merging with the rock." );
                    more();  // or the above message won't be seen

                    const int pass_x = item.parm1;
                    const int pass_y = item.parm2;

                    if (pass_x != 0 && pass_y != 0)
                    {

                        switch (grd[ pass_x ][ pass_y ])
                        {
                        case DNGN_ROCK_WALL:
                        case DNGN_STONE_WALL:
                        case DNGN_METAL_WALL:
                        case DNGN_GREEN_CRYSTAL_WALL:
                        case DNGN_WAX_WALL:
                        case DNGN_SILVER_STATUE:
                        case DNGN_ORANGE_CRYSTAL_STATUE:
                            ouch(1 + you.hp, 0, KILLED_BY_PETRIFICATION);
                            break;

                        case DNGN_SECRET_DOOR:      // oughtn't happen
                        case DNGN_CLOSED_DOOR:      // open the door
                            grd[ pass_x ][ pass_y ] = DNGN_OPEN_DOOR;
                            break;

                        default:
                            break;
                        }

                //jmf: hmm, what to do. kill the monster? (seems too powerful)
                //     displace the monster? randomly teleport the monster?
                //     This seems fair: try to move the monster, but if not
                //     able to, then kill it.
                        int mon = mgrd[ pass_x ][ pass_y ];
                        if (mon != NON_MONSTER)
                        {
                            monster_blink( &menv[ mon ] );

                            // recheck square for monster
                            mon = mgrd[ pass_x ][ pass_y ];
                            if (mon != NON_MONSTER)
                                monster_die( &menv[ mon ], KILL_YOU, 0 );
                        }

                        you.x_pos = pass_x;
                        you.y_pos = pass_y;
                        redraw_screen();

                        const unsigned char grid = grd[ you.x_pos ][ you.y_pos ];
                        if ((grid == DNGN_LAVA || grid == DNGN_DEEP_WATER)
                            && !you.levitation)
                        {
                            if (you.species == SP_MERFOLK && grid == DNGN_DEEP_WATER)
                            {
                                mpr("You fall into the water and return "
                                    "to your normal form.");
                                merfolk_start_swimming();
                            }
                            else
                            {
                                fall_into_a_pool( true, grid );
                                redraw_screen();
                            }
                        }
                    }
                }
                break;

            case DELAY_BUTCHER:
                strcpy( info, "You finish " );
                strcat( info, (you.species == SP_TROLL
                                || you.species == SP_GHOUL) ? "ripping"
                                                            : "chopping" );

                strcat( info, " the corpse into pieces." );
                mpr( info );

                turn_corpse_into_chunks( mitm[item.parm1] );

                if (you.berserker && you.berserk_penalty != NO_BERSERK_PENALTY)
                {
                    mpr("You enjoyed that.");
                    you.berserk_penalty = 0;
                }
                break;

            case DELAY_DROP_ITEM:
                // Note:  checking if item is dropable is assumed to
                // be done before setting up this delay... this includes
                // quantity (item.parm2). -- bwr

                // Make sure item still exists.
                if (!is_valid_item( you.inv[item.parm1] ))
                    break;

                if (!copy_item_to_grid( you.inv[item.parm1],
                                        you.x_pos, you.y_pos, item.parm2 ))
                {
                    mpr("Too many items on this level, not dropping the item.");
                }
                else
                {
                    quant_name( you.inv[item.parm1], item.parm2, DESC_NOCAP_A,
                                str_pass );

                    snprintf( info, INFO_SIZE, "You drop %s.", str_pass );
                    mpr(info);

                    if (item.parm1 == you.equip[EQ_WEAPON])
                    {
                        unwield_item(item.parm1);
                        you.equip[EQ_WEAPON] = -1;
                        canned_msg( MSG_EMPTY_HANDED );
                    }

                    dec_inv_item_quantity( item.parm1, item.parm2 );
                }
                break;

            case DELAY_INTERUPTABLE:
            case DELAY_UNINTERUPTABLE:
                // these are simple delays that have no effect when complete
                break;

            default:
                mpr( "You finish doing something." );
                break;
            }

            you.wield_change = true;
            print_stats();  // force redraw of the stats
            you.turn_is_over = 1;
            you.delay_queue.pop();
        }
    }
}
