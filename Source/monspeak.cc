/*
 *  File:       monspeak.cc
 *  Summary:    Functions to handle speaking monsters
 *
 *  Change History (most recent first):
 *
 *      <1>    01/09/00        BWR     Created
 */

#include "AppHdr.h"
#include "monspeak.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef DOS
#include <conio.h>
#endif

#include "externs.h"

#include "beam.h"
#include "debug.h"
#include "fight.h"
#include "itemname.h"
#include "misc.h"
#include "monplace.h"
#include "mon-util.h"
#include "mstuff2.h"
#include "player.h"
#include "spells2.h"
#include "stuff.h"
#include "view.h"

// returns true if something is said
bool mons_speaks(struct monsters *monster)
{
    int temp_rand;              // probability determination

    // This function is a little bit of a problem for the message channels
    // since some of the messages it generates are "fake" warning to
    // scare the player.  In order to accomidate this intent, we're
    // falsely categorizing various things in the function as spells and
    // danger warning... everything else just goes into the talk channel -- bwr
    int msg_type = MSGCH_TALK;

    const char *m_name = ptr_monam(monster, 0);
    strcpy(info, m_name);

    if (monster->enchantment[2] == ENCH_INVIS)
        return false;
    // invisible monster tries to remain unnoticed

    //mv: if it's also invisible, program never gets here
    if (silenced(monster->x, monster->y))
    {
        if (!one_chance_in(3))
            return false;       // while silenced, don't bother so often

        switch (monster->behavior)
        {
          case BEH_FLEE:
          case BEH_FLEE_FRIEND:
            temp_rand = random2(10);
            strcat(info,
                     (temp_rand <  3) ? " glances furtively about." :
                     (temp_rand == 3) ? " opens its mouth, as if shouting." :
                     (temp_rand == 4) ? " looks around." :
                     (temp_rand == 5) ? " appears indicisive." :
                     (temp_rand == 6) ? " ponders situation."
                                      : " seems to says something.");
            break;

          case BEH_ENSLAVED:
            temp_rand = random2(10);
            strcat(info, (temp_rand <  4) ? " gives you a thumbs up." :
                         (temp_rand == 4) ? " looks at you." :
                         (temp_rand == 5) ? " waves at you." :
                         (temp_rand == 6) ? " smiles happily."
                             : " says something but you don't hear anything.");
            break;

          case BEH_CONFUSED:
          case BEH_CONFUSED_FRIEND:
            temp_rand = random2(10);
            strcat(info, (temp_rand <  4) ? " wildly gestures." :
                         (temp_rand == 4) ? " looks confused." :
                         (temp_rand == 5) ? " grins evily." :
                         (temp_rand == 6) ? " smiles happily." :
                         (temp_rand == 7) ? " cries."
                             : " says something but you don't hear anything.");
            break;

        default:
            temp_rand = random2(10);
            strcat(info, (temp_rand <  3) ? " gestures." :
                         (temp_rand == 3) ? " gestures obscenely." :
                         (temp_rand == 4) ? " grins." :
                         (temp_rand == 5) ? " looks angry." :
                         (temp_rand == 6) ? " seems to be listening."
                             : " says something but you don't hear anything.");
            break;
        }                       //end switch silenced monster's behaviour

        mpr(info, MSGCH_TALK);
        return true;
    }                           // end silenced monster

    switch (monster->behavior)
    {
    case BEH_FLEE:
        {
            switch (random2(14))    // speaks for unfriendly fleeing monsters
            {
            case 0:
                sprintf(info, "%s %s \"Help!\"", m_name, coinflip()? "yells" : "wails");
                break;
            case 1:
                sprintf(info, "%s %s \"Help!\"", m_name,
                        coinflip() ? "crys" : "screams"); break;
            case 2:
                sprintf(info, "%s %s \"Why can't we all just get along?\"",
                        m_name, coinflip() ? "begs" : "pleads");
                break;
            case 3:
                sprintf(info, "%s %s trips in trying to escape.", m_name,
                        coinflip() ? "nearly" : "almost");
                break;
            case 4:
                sprintf(info, "%s %s, \"Of all the rotten luck!\"", m_name,
                        coinflip() ? "mutters" : "mumbles");
                break;
            case 5:
                sprintf(info, "%s %s, \"Oh dear! Oh dear!\"", m_name,
                        coinflip() ? "moans" : "wails");
            case 6:
                sprintf(info, "%s %s, \"Damn and blast!\"", m_name,
                        coinflip() ? "mutters" : "mumbles");
                break;
            case 7:
                strcat(info, " prays for help.");
                break;
            case 8:
                strcat(info, " shouts \"No! I'll never do that again!\"");
                break;
            case 9:
                sprintf(info, "%s %s \"Mercy!\"", m_name,
                        coinflip() ? "begs for" : "cries");
                break;
            case 10:
                sprintf(info, "%s %s \"%s!\"", m_name,
                        coinflip() ? "blubbers" : "cries",
                        coinflip() ? "Mommeee" : "Daddeee");
                break;
            case 11:
                sprintf(info, "%s %s \"Please don't kill me!\"", m_name,
                        coinflip() ? "begs" : "pleads");
                break;
            case 12:
                sprintf(info, "%s %s \"Please don't hurt me!\"", m_name,
                        coinflip() ? "begs" : "pleads");
                break;
            case 13:
                sprintf(info, "%s %s \"Please, I have a lot of children...\"",
                        m_name, coinflip() ? "begs" : "pleads");
                break;
            }
        }
        break;                  // end BEH_FLEE

    case BEH_FLEE_FRIEND:       // speaks for friendly fleeing monsters
        {
            switch (random2(11))
            {
            case 0:
                sprintf(info, "%s %s \"WAIT FOR ME!\"", m_name,
                        coinflip() ? "shouts" : "yells");
                strcat(info, you.your_name);
                strcat(info, ", could you help me?\"");
                break;
            case 1:
                strcat(info, " screams \"Help!\"");
                break;
            case 2:
                strcat(info, " shouts \"Cover me!\"");
                break;
            case 3:
                strcat(info, " screams \"");
                strcat(info, you.your_name);
                strcat(info, "! Help me!\"");
                break;
            case 4:
            case 5:
            case 6:
                strcat(info, " tries to hide somewhere.");
                break;
            case 7:
                strcat(info, " prays for help.");
                break;
            case 8:
                strcat(info, " looks at you beseechingly.");
                break;
            case 9:
                strcat(info, " shouts \"Protect me!\"");
                break;
            case 10:
                strcat(info, " cries \"Don't forget your friends!\"");
                break;
            }
        }
        break;                  //end BEH_FLEE_FRIEND

    case BEH_ENSLAVED:          // speaks for charmed monsters
        {
            // friendly imps are too common so they speak very very rarely
            if ((monster->type == MONS_IMP) && (random2(10)))
                return false;

            switch (random2(16))
            {
            case 0:
                strcat(info, " yells \"Run! I'll cover you!\"");
                break;
            case 1:
                strcat(info, " shouts \"Die, monster!\"");
                break;
            case 2:
                strcat(info, " says \"It's nice to have friends.\"");
                break;

            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                strcat(info, " looks at you.");
                break;
            case 9:
                strcat(info, " smiles at you.");
                break;
            case 10:
                strcat(info, " says \"");
                strcat(info, you.your_name);
                strcat(info, ", you are my only friend.\"");
                break;
            case 11:
                strcat(info, " says \"");
                strcat(info, you.your_name);
                strcat(info, ", I like you.\"");
                break;

            case 12:
                strcat(info, " waves at you.");
                break;
            case 13:
                strcat(info, " says \"Be careful!\"");
                break;
            case 14:
                strcat(info, " says \"Don't worry. I'm here with you.\"");
                break;
            case 15:
                strcat(info, " smiles happily.");
                break;
            case 16:
                strcat(info, " shouts \"No mercy! Kill them all!");
                break;
            case 17:
                strcat(info, " winks at you.");
                break;
            case 18:
                strcat(info, " says \"Me and you. It sounds cool.\"");
                break;
            case 19:
                strcat(info, " says \"I'll never leave you.\"");
                break;
            case 20:
                strcat(info, " says \"I would die for you.\"");
                break;
            case 21:
                strcat(info, " shouts \"Beware of monsters!\"");
                break;
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
                strcat(info, " looks friendly.");
                break;
            }
        }
        break;                  //end BEH_ENSLAVED

    case BEH_CONFUSED:
        {
            switch (random2(15))  // speaks for unfriendly confused monsters
            {
            case 0:
                strcat(info, " yells \"Get them off of me!\"");
                break;
            case 1:
                strcat(info, " screams \"I will kill you anyway!\"");
                break;
            case 2:
                strcat(info, " shouts \"What's happening?\"");
                break;
            case 3:
            case 4:
            case 5:
                strcat(info, " wildly gestures.");
                break;
            case 6:
                strcat(info, " cries.");
                break;
            case 7:
                strcat(info, " shouts \"NO!\"");
                break;
            case 8:
                strcat(info, " shouts \"YES!\"");
                break;
            case 9:
                strcat(info, " laughs crazily.");
                break;
            case 10:
                strcat(info, " ponders situation.");
                break;
            case 11:
                strcat(info, " grins madly.");
                break;
            case 12:
                strcat(info, " looks very confused.");
                break;
            case 13:
                strcat(info, " mumbles something.");
                break;
            case 14:
                strcat(info, " says \"I'm little bit confused.\"");
                break;
            }
        }
        break;                  // end BEH_CONFUSED

    case BEH_CONFUSED_FRIEND:
        {
            switch (random2(18))        // speaks for friendly confused monsters
            {
            case 0:
                strcat(info, " prays for help.");
                break;
            case 1:
                strcat(info, " screams \" Help!\"");
                break;
            case 2:
                strcat(info, " shouts \"I'm losing control!\"");
                break;
            case 3:
                strcat(info, " shouts \"What's happening?\"");
                break;
            case 4:
            case 5:
                strcat(info, " wildly gestures.");
                break;
            case 6:
                strcat(info, " cries.");
                break;
            case 7:
                strcat(info, " shouts \"Yeah!\"");
                break;
            case 8:
                strcat(info, " sings.");
                break;
            case 9:
                strcat(info, " laughs crazily.");
                break;
            case 10:
                strcat(info, " ponders situation.");
                break;
            case 11:
                strcat(info, " grins madly.");
                break;
            case 12:
                strcat(info, " looks very confused.");
                break;
            case 13:
                strcat(info, " mumbles something.");
                break;
            case 14:
                strcat(info, " giggles crazily.");
                break;
            case 15:
                strcat(info, " screams \"");
                strcat(info, you.your_name);
                strcat(info, "! Help!\"");
                break;
            case 16:
                strcat(info, " screams \"");
                strcat(info, you.your_name);
                strcat(info, "! What's going on?\"");
                break;
            case 17:
                strcat(info, " says \"");
                strcat(info, you.your_name);
                strcat(info, ", I'm little bit confused.\"");
                break;
            }
        }
        break;                  // end BEH_CONFUSED_FRIEND

    default:                    // normal monster shouts
        {
            switch (monster->type)
            {
            case MONS_TERENCE:  // fighter who likes to kill
                switch (random2(9))
                {
                case 0:
                    strcat(info, " screams \"I'm going to kill you! \"");
                    break;
                case 1:
                    strcat(info, " shouts \"Now you die.\"");
                    break;
                case 2:
                    strcat(info, " says \"Rest in peace.\"");
                    break;
                case 3:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 4:
                    strcat(info, " says \"How do you enjoy it?\"");
                    break;
                case 5:
                    strcat(info, " shouts \"Get ready for death!\"");
                    break;
                case 6:
                    strcat(info, " says \"You are history.\"");
                    break;
                case 7:
                    strcat(info, " says \"Do you want it fast or slow?.\"");
                    break;
                case 8:
                    strcat(info, " says \"Did you write a testament? You should...\"");
                    break;
                }
                break;          // end Terence

            case MONS_EDMUND:   // mercenaries guarding dungeon
            case MONS_LOUISE:
            case MONS_FRANCES:
            case MONS_DUANE:
            case MONS_ADOLF:
                switch (random2(15))
                {
                case 0:
                    strcat(info, " screams \"I'm going to kill you! Now!\"");
                    break;
                case 1:
                    strcat(info,
                           " shouts \"Return immediately or I'll kill you!\"");
                    break;
                case 2:
                    strcat(info,
                           " says \"Now you've reached the end of your way.\"");
                    break;
                case 3:
                    strcat(info,
                           " screams \"One false step and I'll kill you!\"");
                    break;
                case 4:
                    strcat(info, " says \"Drop everything you've found here and return home.\"");
                    break;
                case 5:
                    strcat(info, " shouts \"You will never get the Orb.\"");
                    break;
                case 6:
                    strcat(info, " looks very unfriendly.");
                    break;
                case 7:
                    strcat(info, " looks very cold.");
                    break;
                case 8:
                    strcat(info, " shouts \"It's the end of the party!\"");
                    break;
                case 9:
                    strcat(info, " says \"Return every stolen item!\"");
                    break;
                case 10:
                    strcat(info, " says \"No trespassing is allowed here.\"");
                        break;
                case 11:
                    strcat(info, " grins evilly.");
                    break;
                case 12:
                    strcat(info, " screams \"You must be punished!\"");
                    break;
                case 13:
                    strcat(info, " says \"It's nothing personal...\"");
                    break;
                case 14:
                    strcat(info, " says \"A dead adventurer is good adventurer.\"");
                    break;
                }
                break;          // end Edmund & Co

            case MONS_JOSEPH:
                switch (random2(16))
                {
                case 0:
                    strcat(info, " smiles happily.");
                    break;
                case 1:
                    strcat(info, " says \"I'm happy to see you. And I'll be happy to kill you.\"");
                    break;
                case 2:
                    strcat(info, " says \"I've wait for this moment for such a long time.\"");
                    break;
                case 3:
                    strcat(info,
                           " says \"It's nothing personal but I have kill you.\"");
                    break;
                case 5:
                    strcat(info, " says \"You will never get the Orb, sorry.\"");
                    break;
                case 9:
                    strcat(info, " shouts \"I love to fight! I love killing!\"");
                    break;
                case 10:
                    strcat(info,
                           " says \"I'm here to kill trespassers. I like my job.\"");
                    break;
                case 11:
                    strcat(info, " tries to grin evilly.");
                    break;
                case 12:
                    strcat(info,
                           " says \"You must be punished! Or... I want to punish you!\"");
                    break;
                case 13:
                    strcat(info,
                           " sighs \"Being guard is usually so boring...\"");
                    break;
                case 14:
                    strcat(info, " shouts \"At last some action!\"");
                    break;
                case 15:
                    strcat(info, " shouts \"Wow!\"");
                    break;
                }
                break;          // end Joseph

            case MONS_ORC_HIGH_PRIEST:  // priest, servants of dark ancient god
            case MONS_DEEP_ELF_HIGH_PRIEST:
                switch (random2(10))
                {
                case 0:
                case 1:
                    strcat(info, " prays.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                /*
                // removed -- shouldn't really assume that orcs and elves
                // worship the same god -- bwr
                case 1:
                    strcat(info, " says \"My Lord BOG, aid me.\"");
                    break;
                */
                case 2:
                case 4:
                    strcat(info, " mumbles some strange prayers.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 3:
                    strcat(info,
                           " shouts \"You are a heretic and must be destroyed.\"");
                    break;
                /*
                // the orc god probably doesn't care about "sin" -- bwr
                case 4:
                    strcat(info, " shouts \"Confess!\"");
                    break;
                case 5:
                    strcat(info, " says \"All sinners must die.\"");
                    break;
                */
                case 6:
                    strcat(info, " looks excited.");
                    break;
                case 7:
                    strcat(info, " says \"You will make a fine sacrifice.\"");
                    break;
                case 5:
                case 8:
                    strcat(info, " starts to sing a prayer.");
                    break;
                case 9:
                    strcat(info, " shouts \"You must be punished.\"");
                    break;
                }
                break;          // end priests

            case MONS_ORC_SORCEROR:   // hateful wizards, using strange powers
            case MONS_WIZARD:
                switch (random2(18))
                {
                case 0:
                case 1:
                case 2:
                    strcat(info, " wildly gestures.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 3:
                case 4:
                case 5:
                    strcat(info, " mumbles some strange words.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 6:
                    strcat(info, " shouts \"You can't face my power!\"");
                    break;
                case 7:
                    strcat(info, " shouts \"You are history.\"");
                    break;
                case 8:
                    strcat(info, " becomes transparent for a moment.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 9:
                    strcat(info, " throws some strange powder towards you.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 10:
                    strcat(info, " glows brightly for a moment.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 11:
                    strcat(info, " says \"argatax netranoch dertex\"");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 12:
                    strcat(info, " says \"dogrw nutew berg\"");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 13:
                    strcat(info, " shouts \"Entram moth deg ulag!\"");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 14:
                    strcat(info, " becomes larger for a moment.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 15:
                    strcat(info, "'s fingertips starts to glow.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 16:
                    strcat(info, "'s eyes starts to glow.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 17:
                    strcat(info, " tries to paralyze you with his gaze.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                }
                break;          // end wizards

            case MONS_JESSICA:  // sorceress disturbed by player
                switch (random2(10))
                {
                case 0:
                    strcat(info, " grins evilly.");
                    break;
                case 1:
                    strcat(info, " says \"I'm really upset.\"");
                    break;
                case 2:
                    strcat(info, " shouts \"I don't like beings like you.\"");
                    break;
                case 3:
                    strcat(info,
                           " shouts \"Stop bothering me, or I'll kill you!\"");
                    break;
                case 4:
                    strcat(info, " very coldly says \"I hate your company.\"");
                    break;
                case 5:
                    strcat(info, " mumbles something strange.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 6:
                    strcat(info, " looks very angry.");
                    break;
                case 7:
                    strcat(info,
                            " shouts \"You're disturbing me so now I'll have kill you.\"");
                    break;
                case 8:
                    strcat(info, " screams \"You are ghastly nuisance!\"");
                    break;
                case 9:
                    strcat(info, " gestures wildly.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                }
                break;          // end Jessica

            case MONS_SIGMUND:  // mad old wizard
                switch (random2(17))
                {
                case 0:
                case 1:
                case 2:
                    strcat(info, " laughs crazily.");
                    break;
                case 3:
                    strcat(info, " says \"Don't worry, I'll kill you fast.\"");
                    break;
                case 4:
                    strcat(info, " grinds his teeth.");
                    break;
                case 5:
                    strcat(info, " asks \"Do you like me?\"");
                    break;
                case 6:
                    strcat(info, " screams \"Die, monster!\"");
                    break;
                case 7:
                    strcat(info, " says \"You will forget everthing soon.\"");
                    break;
                case 8:
                    strcat(info, " screams \"You will never... NEVER!\"");
                    break;
                case 9:
                    strcat(info, "'s eyes starts to glow with a red light. ");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 10:
                    strcat(info, " says \"Look in to my eyes.\"");
                    break;
                case 11:
                    strcat(info, " says \"I'm your fate.\"");
                    break;
                case 12:
                    strcat(info, " is suddenly surrounded by pale blue light.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 13:
                    strcat(info, " tries to bite you.");
                    break;
                case 14:
                    strcat(info, " is suddenly surrounded by pale green light.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 15:
                    strcat(info, " screams \"I am the angel of Death!\"");
                    break;
                case 16:
                    strcat(info, " screams \"Only death can liberate you!\"");
                    break;
                }
                break;          // end Sigmund

            case MONS_IMP:      // small demon
                if (coinflip())
                    return false;       // imps are too common to bother so often
                switch (random2(11))
                {
                case 0:
                    strcat(info, " laughs crazily.");
                    break;
                case 1:
                    strcat(info, " grins evilly.");
                    break;
                case 2:
                    strcat(info, " breathes a bit of smoke at you.");
                    break;
                case 3:
                    strcat(info, " lashes with his tail.");
                    break;
                case 4:
                    strcat(info, " grinds his teeth.");
                    break;
                case 5:
                    strcat(info, " sputters.");
                    break;
                case 6:
                    strcat(info, " breathes some steam toward you.");
                    break;
                case 7:
                    strcat(info, " spits at you.");
                    break;
                case 8:
                    strcat(info, " disappears for a moment.");
                    break;
                case 9:
                    strcat(info, " summons swarm of flyies.");
                    break;
                case 10:
                    strcat(info, " picks up some beetle and eats it.");
                    break;
                }
                break;          // end imp

            case MONS_TORMENTOR:        // cruel devil
                switch (random2(18))
                {
                case 0:
                    strcat(info, " laughs crazily.");
                    break;
                case 1:
                    strcat(info, " grins evilly.");
                    break;
                case 2:
                    strcat(info, " says \"I am all your nightmares come true.\"");
                    break;
                case 3:
                    strcat(info, " says \"I will show you what pain is.\"");
                    break;
                case 4:
                    strcat(info, " shouts \"I'll tear you apart.\"");
                    break;
                case 5:
                    strcat(info,
                           " says \"You will wish to die when I get to you.\"");
                    break;
                case 6:
                    strcat(info, " says \"I will drown you in you blood.\"");
                    break;
                case 7:
                    strcat(info,
                           " says \"You will die in a very very ugly way.\"");
                    break;
                case 8:
                    strcat(info, " says \"I will eat your liver.\"");
                    break;
                case 9:
                    strcat(info, " grins madly.");
                    break;
                case 10:
                    strcat(info, " shouts \"Prepare for my thousand needles of pain!\"");
                    break;
                case 11:
                    strcat(info,
                           " says \"I know thousand and one way to kill you.\"");
                    break;
                case 12:
                    strcat(info,
                           " says \"I'll show you my torture chamber!\"");
                    break;
                case 13:
                case 14:
                    strcat(info,
                           " says \"I'll crush all your bones. One by one.\"");
                    break;
                case 15:
                    strcat(info, " says \"I know your fate. It's pain.\"");
                    break;
                case 16:
                    strcat(info, " says \"Get ready ! Throes await you.\"");
                    break;
                case 17:
                    strcat(info, " grins malevolently.");
                    break;
                }
                break;          // end tormentor

            case MONS_PLAYER_GHOST:     // ghost of unsuccesful player
                switch (random2(24))
                {
                case 0:
                    strcat(info, " laughs crazily.");
                    break;
                case 1:
                    strcat(info, " grins evilly.");
                    break;
                case 2:
                    strcat(info, " shouts \"You will never get the ORB!\"");
                    break;
                case 3: // mv: ghosts are usually wailing, aren't ?
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    strcat(info, " wails.");
                    break;
                case 12:
                    strcat(info, " stares at you . You feel cold.");
                    break;
                case 13:
                    strcat(info, " screams \"You will join me soon!\"");
                    break;
                case 14:
                    strcat(info, " wails \"To die, to sleep, no more.\"");
                    break;      //Hamlet
                case 15:
                    strcat(info,
                           " screams \"You must not succeed where I failed.\"");
                    break;
                case 16:
                    strcat(info,
                           " screams \"I'll kill anyone who wants the ORB.\"");
                    break;
                case 17:
                    strcat(info, " whispers \"Meet emptiness of death!\"");
                    break;
                case 18:
                    strcat(info, " whispers \"Death is liberation.\"");
                    break;
                case 19:
                    strcat(info,
                           " whispers \"Everlasting silence awaits you.\"");
                    break;
                case 20:
                    strcat(info,
                           " screams \"Don't try to defend. You have no chance!\"");
                    break;
                case 21:
                    strcat(info,
                           " whispers \"Death doesn't hurt. What you feel is life.\"");
                    break;
                case 22:
                    strcat(info, " whispers \"The ORB doesn't exist.\"");
                    break;
                case 23:
                    strcat(info, " wails \"Death is your only future.\"");
                    break;
                }
                break;          // end players ghost

            case MONS_PSYCHE:   // insane girl
                switch (random2(20))
                {
                case 0:
                    strcat(info, " smiles happily.");
                    break;
                case 1:
                    strcat(info, " giggles crazily.");
                    break;
                case 2:
                    strcat(info, " cries.");
                    break;
                case 3:
                    strcat(info, " stares at you for a moment.");
                    break;
                case 4:
                    strcat(info, " sings.");
                    break;
                case 5:
                    strcat(info,
                           " says \"Please, could you die a little faster?\"");
                    break;
                case 6:
                    strcat(info,
                           " says \"I'm bad girl. But I can't to anything about it.\"");
                    break;
                case 7:
                    strcat(info,
                           " screams \"YOU ARE VIOLATING AREA SECURITY!\"");
                    break;
                case 8:
                    strcat(info, " cries \"I hate blood and violence.\"");
                    break;
                case 9:
                    strcat(info,
                           " screams \"Peace! Flowers! Freedom! Dead adventurers!\"");
                    break;
                case 10:
                    strcat(info,
                           " says \"I'm so lonely. Only corpses are my friends.\"");
                    break;
                case 11:
                    strcat(info, " cries \"You've killed my pet.\"");
                    break;
                case 12:
                    strcat(info,
                           " cries \"You want to steal my collection of the orbs.\"");
                    break;
                case 13:
                    strcat(info, " sings some strange song.");
                    break;
                case 14:
                    strcat(info, " bursts in tears.");
                    break;
                case 15:
                    strcat(info, " sucks her thumb.");
                    break;
                case 16:
                    strcat(info,
                           " whispers \"Hold me, thrill me, kiss me, kill me.\"");
                    break;      //(c) U2 ?
                case 17:
                    strcat(info, " says \"I'll kill you and take you home.\"");
                    break;
                case 18:
                    strcat(info,
                           " shouts \"Well, maybe I'm nutty, but who cares?\"");
                    break;
                case 19:
                    strcat(info,
                           " shouts \"I hope that you are sorry for that.\"");
                    break;
                }
                break;          // end Psyche

            case MONS_DONALD:   // adventurers hating competition
            case MONS_WAYNE:
                switch (random2(11))
                {
                case 0:
                    strcat(info, " screams \"Return home!\"");
                    break;
                case 1:
                    strcat(info, " screams \"The Orb is mine!\"");
                    break;
                case 2:
                    strcat(info, " screams \"Give me all your treasure!\"");
                    break;
                case 3:
                    strcat(info, " screams \"You will never get the Orb!\"");
                    break;
                case 4:
                    strcat(info, " screams \"I was here first!\"");
                    break;
                case 5:
                    strcat(info, " frowns.\"");
                    break;
                case 6:
                    strcat(info, " looks very upset.");
                    break;
                case 7:
                    strcat(info, " screams \"Get away or die!\"");
                    break;
                case 8:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 9:
                    strcat(info, " screams \"First you have to pass me!\"");
                    break;
                case 10:
                    strcat(info, " screams \"I hate you!\"");
                    break;
                }
                break;          // end Donald

            case MONS_MICHAEL:  // spellcaster who wanted to be alone
                switch (random2(11))
                {
                case 0:
                    strcat(info, " looks very angry.");
                    break;
                case 1:
                    strcat(info, " frowns.");
                    break;
                case 2:
                    strcat(info, " screams \"I want to be alone!\"");
                    break;
                case 3:
                    strcat(info, " says \"You are really nuisance.\"");
                    break;
                case 4:
                    strcat(info,
                           " screams \"I wanted to be alone. And you...\"");
                    break;
                case 5:
                    strcat(info, " screams \"Get away! Or better yet, die!\"");
                    break;
                case 6:
                    strcat(info, " mumbles some strange words.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 7:
                    strcat(info, " points at you.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 8:
                    strcat(info, " shakes with wrath.");
                    break;
                case 9:
                    strcat(info, " drinks a potion.");
                    break;
                case 10:
                    strcat(info, " gestures wildly.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                }
                break;          // end Michael

            case MONS_ERICA:    // wild tempered adventuress
                switch (random2(12))
                {
                case 0:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 1:
                    strcat(info, " screams \"Do you want it fast or slow?\"");
                    break;
                case 2:
                    strcat(info, " looks angry.");
                    break;
                case 3:
                    strcat(info, " drinks a potion.");
                    break;
                case 4:
                    strcat(info, " says \"I'm so much better than you.\"");
                    break;
                case 5:
                    strcat(info,
                           " says \"Fast and perfect. Such is my way of killing.\"");
                    break;
                case 6:
                    strcat(info, " screams \"Hurry! Death awaits!\"");
                    break;
                case 7:
                    strcat(info, " laughs wildly.");
                    break;
                case 8:
                    strcat(info, " screams \"I'll never tell where it is.\"");
                    break;
                case 9:
                    strcat(info, " screams \"You'll never get it.\"");
                    break;
                case 10:
                    strcat(info, " screams \"Coming here was suicide.\"");
                    break;
                case 11:
                    strcat(info,
                           " says \"I love fight. I also love... Erm, killing is better.\"");
                    break;
                }
                break;          // end Erica

            case MONS_JOSEPHINE:        // ugly old witch looking for somone to kill
                switch (random2(13))
                {
                case 0:
                case 1:
                case 2:
                    strcat(info, " grins evilly.");
                    break;
                case 3:
                case 4:
                    strcat(info, " screams \"I will kill you!\"");
                    break;
                case 5:
                    strcat(info, " grinds her teeth.");
                    break;
                case 6:
                    strcat(info, " grins malevolently.");
                    break;
                case 7:
                    strcat(info, " laughs insanely.");
                    break;
                case 8:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 9:
                    strcat(info,
                           " screams \"I have something special for you!\"");
                    break;
                case 10:
                    strcat(info,
                           " screams \"I'll use your head as decoration in my hut.\"");
                    break;
                case 11:
                    strcat(info, " says \"I'll make a rug of your skin.\"");
                    break;
                case 12:
                    strcat(info, " says \"What about some decapitation?\"");
                    break;
                }
                break;          // end Josephine

            case MONS_HAROLD:   // middle aged man, hired to kill you. He is in a hurry.
                switch (random2(11))
                {
                case 0:
                    strcat(info, " looks nervous.");
                    break;
                case 1:
                    strcat(info, " screams \"Hurry up!\"");
                    break;
                case 2:
                    strcat(info, " screams \"Could you die faster?\"");
                    break;
                case 3:
                    strcat(info,
                           " says \"Stand still. I'd like to kill you.\"");
                    break;
                case 4:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 5:
                    strcat(info, " says \"I hope you die soon!\"");
                    break;
                case 6:
                    strcat(info,
                           " says \"Only few hits and and its over.\".");
                    break;
                case 7:
                    strcat(info, " says \"You know, I'm in a hurry.\"");
                    break;
                case 8:
                    strcat(info, " screams \"I'll finish you soon!\"");
                    break;
                case 9:
                    strcat(info, " screams \"Don't delay it.\"");
                    break;
                case 11:
                    strcat(info, " says \"Mine is not to reason why.  Mine is to do, your's to die.\"" );
                }
                break;          // end Harold

            // skilled warrior looking for some fame. More deads = more fame
            case MONS_NORBERT:
                switch (random2(13))
                {
                case 0:
                    strcat(info, " smiles happily.");
                    break;
                case 1:
                    strcat(info, " screams \"Die, monster!\"");
                    break;
                case 2:
                    strcat(info, " screams \"I'm hero!\"");
                    break;
                case 3:
                    strcat(info, " shouts \"YES! Another frag!\"");
                    break;
                case 4:
                    strcat(info, " says \"I hope I'll get many XP for you.\"");
                    break;
                case 5:
                    strcat(info,
                           " screams \"Pray, because you'll die soon!\"");
                    break;
                case 6:
                    strcat(info,
                          " asks \" Did you write a testament? You should.\".");
                    break;
                case 7:
                    strcat(info,
                           " says \"I love killing ugly monsters like you.\"");
                    break;
                case 8:
                    strcat(info, " screams \"Blood and destruction!\"");
                    break;
                case 9:
                    strcat(info, " says \"It's honour to die by my hand.\"");
                    break;
                case 10:
                    strcat(info, " shouts \"Your time has come!\"");
                    break;
                case 11:
                    strcat(info,
                           " says \"I'm sorry but you don't have a chance.\"");
                    break;
                case 12:
                    strcat(info,
                           " says \"Another dead monster... It must be my lucky day!\"");
                    break;
                }
                break;          // end Norbert

            case MONS_JOZEF:    // bounty hunter
                switch (random2(14))
                {
                case 0:
                    strcat(info, " looks satisfied.");
                    break;
                case 1:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 2:
                    strcat(info, " screams \"At last I found you!\"");
                    break;
                case 3:
                    strcat(info, " shouts \"I'll get 500 for your head!\"");
                    break;
                case 4:
                    strcat(info,
                           " says \"You don't look worth for that money.\"");
                    break;
                case 5:
                    strcat(info,
                           " says \"It's nothing personal. I'm paid for it!\"");
                    break;
                case 6:
                    strcat(info,
                           " asks \" Did you write a testament? You should.\".");
                    break;
                case 7:
                    strcat(info, " says \"You are ");
                    strcat(info, you.your_name);
                    strcat(info, ", aren't you ?.\"");
                    break;
                case 8:
                    strcat(info, " says \"I suppose that you are ");
                    strcat(info, you.your_name);
                    strcat(info, ". Sorry, if I'm wrong.\"");
                    break;
                case 9:
                    strcat(info, " says \"One dead ");
                    strcat(info, you.your_name);
                    strcat(info, ", 500 gold pieces. It's in my contract.\"");
                    break;
                case 10:
                    strcat(info, " shouts \"Your time has come!\"");
                    break;
                case 11:
                    strcat(info,
                           " says \"My job is sometimes very exciting. Sometimes...\"");
                    break;
                case 12:
                    strcat(info, " says \"I think I deserve my money.\"");
                    break;
                case 13:
                    strcat(info,
                           " screams \"Die! I've got more contracts today.\"");
                    break;
                }
                break;          // end Jozef

            case MONS_AGNES:    // she is trying to get money and treasure
                switch (random2(10))
                {
                case 0:
                    strcat(info, " screams \"Give me all your money!\"");
                    break;
                case 1:
                    strcat(info, " screams \"All treasure is mine!\"");
                    break;
                case 2:
                    strcat(info, " screams \"You'll never get my money!\"");
                    break;
                case 3:
                    strcat(info, " grins evilly.");
                    break;
                case 4:
                    strcat(info,
                           " screams \"Give me everything and get away!\"");
                    break;
                case 5:
                    strcat(info,
                           " says \"I need new robe. I'll buy it from your money.\"");
                    break;
                case 6:
                    strcat(info,
                           " screams \"I want your rings! And amulets! And... EVERYTHING!\"");
                    break;
                case 7:
                    strcat(info,
                           " screams \"I hate dirty adventurers like you.\"");
                    break;
                case 8:
                    strcat(info, " says \"How can you wear that ugly dress?\"");
                    break;
                case 9:
                    strcat(info, " screams \"Die, beast!\"");
                    break;
                }
                break;          // end Agnes

            case MONS_MAUD:     // warrior princess looking for sword "Entarex"
                switch (random2(11))
                {
                case 0:
                    strcat(info, " screams \" Submit or die!\"");
                    break;
                case 1:
                    strcat(info, " screams \"Give me \" Entarex\"!\"");
                    break;
                case 2:
                    strcat(info,
                           " screams \"If you give me \"Entarex\", I'll let you live!\"");
                    break;
                case 3:
                    strcat(info, " frowns.");
                    break;
                case 4:
                    strcat(info, " looks upset.");
                    break;
                case 5:
                    strcat(info, " screams \"You can't face my power!\"");
                    break;
                case 6:
                    strcat(info,
                           " screams \"Give me that sword! Immediately!\"");
                    break;
                case 7:
                    strcat(info,
                           " screams \"Your life or \"Entarex\"! You must choose.\"");
                    break;
                case 8:
                    strcat(info, " screams \"I want it!\"");
                    break;
                case 9:
                    strcat(info, " screams \"Die, you thief!\"");
                    break;
                case 10:
                    // needed at least one in here to tie to the amnesia
                    // scroll reference -- bwr
                    strcat(info, " asks \"Will you think of me as you die?\"");
                    break;
                }
                break;          // end Maud

            // wizard looking for bodyparts as spell components
            case MONS_FRANCIS:
                switch (random2(15))
                {
                case 0:
                    strcat(info,
                           " says \" You've nice eyes. I could use them.\"");
                    break;
                case 1:
                    strcat(info, " says \"Excuse me, but I need your head.\"");
                        break;
                case 2:
                    strcat(info, " says \"I only need a few of your organs!\"");
                        break;
                case 3:
                    strcat(info, " ponders situation.");
                    break;
                case 4:
                    strcat(info, " looks for scalpel.");
                    break;
                case 5:
                    strcat(info, "'s hands started to glow with soft light.");
                    break;
                case 6:
                    strcat(info, " says \"It won't hurt.\"");
                    break;
                case 7:
                    strcat(info, " throws something at you.");
                    break;
                case 8:
                    strcat(info, " says \"I want you in my laboratory!\"");
                    break;
                case 9:
                    strcat(info, " says \"What about little dissection?\"");
                    break;
                case 10:
                    strcat(info,
                           " says \"I have something special for you.\"");
                        break;
                case 11:
                    strcat(info,
                           " screams \"Don't move! I want to cut your ear!\"");
                    break;
                case 12:
                    strcat(info,
                           " says \"What about your heart? Do you need it?\"");
                    break;
                case 13:
                    strcat(info,
                           " says \"Did you know that corpses are an important natural resource?\"");
                    break;
                case 14:
                    strcat(info,
                           " says \"Don't worry, I'll cut only few pieces.\"");
                    break;
                }
                break;          // end Francis

            case MONS_RUPERT:   // crazy adventurer
                switch (random2(11))
                {
                case 0:
                    strcat(info, " says \" You are a monster, aren't you?\"");
                    break;
                case 1:
                    strcat(info, " screams \"Die, monster!\"");
                    break;
                case 2:
                    strcat(info, " screams \"Give me Holy Grail!\"");
                    break;
                case 3:
                    strcat(info, " screams \"Where is Holy Trail?\"");
                    break;
                case 4:
                    strcat(info, " looks confused.");
                    break;
                case 5:
                    strcat(info, " looks excited.");
                    break;
                case 6:
                    strcat(info, " shouts \"I'm great and powerful hero!\"");
                    break;
                case 7:
                    strcat(info,
                           " screams \"Get ready! I'll kill you! Or something like it...\"");
                    break;
                case 8:
                    strcat(info,
                           " says \"My Mom always said, kill them all.\"");
                    break;
                case 9:
                    strcat(info,
                           " screams \"You killed all those lovely monsters, you murderer!\"");
                    break;
                case 10:
                    strcat(info, " screams \"Hurray!\"");
                    break;
                }
                break;          // end Rupert

            case MONS_NORRIS:   // enlighten but crazy man
                switch (random2(21))
                {
                case 0:
                    strcat(info, " sings \"Hare Rama, Hare Krishna!\"");
                    break;
                case 1:
                    strcat(info, " smiles at you.");
                    break;
                case 2:
                    strcat(info,
                           " says \"After death you'll find inner peace.\"");
                    break;
                case 3:
                    strcat(info,
                           " says \"Life is just suffering. I'll help you.\"");
                    break;
                case 4:
                    strcat(info, " is surrounded with aura of peace.");
                    break;
                case 5:
                    strcat(info, " looks very balanced.");
                    break;
                case 6:
                    strcat(info, " says \"Don't resist. I'll do it for you.\"");
                    break;
                case 7:
                    strcat(info, " screams \"Enter NIRVANA! Now!\"");
                    break;
                case 8:
                    strcat(info, " says \"Death is just a liberation!\"");
                    break;
                case 9:
                    strcat(info,
                           " says \"Feel free to die. It's great thing.\"");
                    break;
                case 10:
                    strcat(info, " says \"OHM MANI PADME HUM!\"");
                    break;
                case 11:
                    strcat(info, " mumbles some mantras.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 12:
                    strcat(info, " says \"Breath deep.\"");
                    break;
                case 13:
                    strcat(info, " screams \"Love! Eternal love!\"");
                    break;
                case 14:
                    strcat(info,
                           " screams \"Peace! I bring you eternal peace!\"");
                    break;
                case 15:
                    strcat(info,
                           " sighs \"Enlightenment is such a responsibility.\"");
                    break;
                case 16:
                    strcat(info, " looks relaxed.");
                    break;
                case 17:
                    strcat(info, " screams \"Free your soul! Die!\"");
                    break;
                case 18:
                    strcat(info, " screams \"Blow your mind!\"");
                    break;
                case 19:
                    strcat(info,
                           " says \"The Orb is only your imagination. Forget it.\"");
                    break;
                case 20:
                    strcat(info, " says \"It's all maya.\"");
                    break;
                }
                break;          // end Norris

            case MONS_MARGERY:  // powerful sorceress, guarding the ORB
                switch (random2(22))
                {
                case 0:
                    strcat(info, " says \"You are dead.\"");
                    break;
                case 1:
                    strcat(info, " looks very self-confident.");
                    break;
                case 2:
                    strcat(info, " screams \"You must be punished!\"");
                    break;
                case 3:
                    strcat(info, " screams \"You can't face my power!\"");
                    break;
                case 4:
                    strcat(info, " is surrounded with aura of power.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 5:
                    strcat(info, "'s eyes starts to glow with a red light.");
                    break;
                case 6:
                    strcat(info,
                           "'s eyes starts to glow with a green light.");
                        break;
                case 7:
                    strcat(info, "'s eyes starts to glow with a blue light.");
                    break;
                case 8:
                    strcat(info, " screams \"All trespassers must die.\"");
                    break;
                case 9:
                    strcat(info, " says \"Die!\"");
                    break;
                case 10:
                    strcat(info, " screams \"You'll have to get pass me!\"");
                    break;
                case 11:
                    strcat(info, " becomes transparent for a moment.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 12:
                    strcat(info, " gestures.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 13:
                    strcat(info, "'s hands start to glow.");
                    msg_type = MSGCH_MONSTER_SPELL;
                    break;
                case 14:
                    strcat(info, " screams \"Ergichanteg reztahaw!\"");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel really bad.", MSGCH_WARN);
                    return true;
                    break;
                case 15:
                    strcat(info, " screams \"You are doomed!\"");
                    break;
                case 16:
                    strcat(info, " screams \"Nothing can help you.\"");
                    break;
                case 17:
                    strcat(info, " screams \"Death is my second name!\"");
                    break;
                case 18:
                    strcat(info, " gestures.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel doomed.", MSGCH_WARN);
                    return true;
                    break;
                case 19:
                    strcat(info, " gestures.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel weakened.", MSGCH_WARN);
                    return true;
                    break;
                case 20:
                    strcat(info, " throws some weird powder towards you.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel cursed.", MSGCH_WARN);
                    return true;
                    break;
                case 21:
                    strcat(info,
                           " screams \"The ORB is only a tale, but I will kill you anyway!");
                    break;
                }
                break;          // end Margery

            case MONS_IJYB:     // twisted goblin
                switch (random2(11))
                {
                case 0:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 1:
                    strcat(info, " screams \"Me kill you!\"");
                    break;
                case 2:
                    strcat(info, " screams \"Me stronger than you!\"");
                    break;
                case 3:
                case 4:
                    strcat(info, " grins evilly.");
                    break;
                case 5:
                    strcat(info, " screams \"It's all mine!\"");
                    break;
                case 6:
                    strcat(info, " screams \"Get away!\"");
                    break;
                case 7:
                    strcat(info, " screams \"This level is mine! Whole!\"");
                    break;
                case 8:
                    strcat(info, " screams \"I'll cut your head away!\"");
                    break;
                case 9:
                    strcat(info,
                           " screams \"I'll be skip on your corpse!\"");
                        break;
                case 10:
                    strcat(info, " screams \"Me very upset!\"");
                    break;
                }
                break;          // end IJYB

            case MONS_BLORK_THE_ORC:    // unfriendly orc
                switch (random2(20))
                {
                case 0:
                    strcat(info, " screams \"I don't like you!\"");
                    break;
                case 1:
                    strcat(info, " screams \"I'm going to kill you!\"");
                    break;
                case 2:
                    strcat(info,
                           " screams \"I'm much stronger than you!\"");
                    break;
                case 3:
                case 4:
                    strcat(info, " grins evilly.");
                    break;
                case 5:
                    strcat(info, " frowns.");
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                    strcat(info, " looks angry.");
                    break;
                case 10:
                    strcat(info,
                           " screams \"I'll eat your brain! And then I'll vomit it back up!\"");
                    break;
                case 11:
                    strcat(info,
                           " screams \"You are the ugliest creature I've ever seen!\"");
                    break;
                case 12:
                    strcat(info, " screams \"I'll cut your head off!\"");
                    break;
                case 13:
                    strcat(info, " screams \"I'll break your legs!\"");
                    break;
                case 14:
                    strcat(info, " screams \"I'll break your arms!\"");
                    break;
                case 15:
                    strcat(info,
                           " screams \"I'll crush all your ribs! One by one!\"");
                    break;
                case 16:
                    strcat(info,
                           " screams \"I'll make a cloak from your skin!\"");
                        break;
                case 17:
                    strcat(info,
                           " screams \"I'll decorate my home with your organs!\"");
                    break;
                case 18:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 19:
                    strcat(info,
                           " screams \"I'll cover dungeon with your blood!\"");
                    break;
                }
                break;          // end Blork

            case MONS_EROLCHA:  // ugly ogre
                switch (random2(11))
                {
                case 0:
                    strcat(info, " tries to grin evilly.");
                    break;
                case 1:
                    strcat(info, " screams \"Eat!\"");
                    break;
                case 2:
                    strcat(info,
                           " screams \"Stand! Erolcha want hit you!\"");
                        break;
                case 3:
                    strcat(info, " screams \"Blood!\"");
                    break;
                case 4:
                    strcat(info, " screams \"Erolcha kill you!\"");
                    break;
                case 5:
                    strcat(info,
                           " screams \"Erolcha want your head crushed!\"");
                    break;
                case 6:
                    strcat(info, " roars.");
                    break;
                case 7:
                    strcat(info, " growls.");
                    break;
                case 8:
                    strcat(info, " screams \"You lunch!\"");
                    break;
                case 9:
                    strcat(info,
                           " screams \"Erolcha be happy to kill you!\"");
                        break;
                case 10:
                    strcat(info, " screams \"Erolcha be angry!\"");
                    break;
                }
                break;          // end Erolcha

            case MONS_URUG:     // orc hired to kill you
                switch (random2(11))
                {
                case 0:
                    strcat(info, " grins evilly.");
                    break;
                case 1:
                    strcat(info, " screams \"Die!\"");
                    break;
                case 2:
                    strcat(info,
                           " screams \"I'm going to kill you! Now!\"");
                        break;
                case 3:
                    strcat(info, " screams \"Blood and destruciton!\"");
                    break;
                case 4:
                    strcat(info,
                           " screams \"Are you innocent? I'll kill you anyway.\"");
                    break;
                case 5:
                    strcat(info,
                           " screams \"I'll get 30 silver pieces for your head!\"");
                    break;
                case 6:
                    strcat(info, " roars.");
                    break;
                case 7:
                    strcat(info, " howls with blood-lust.");
                    break;
                case 8:
                    strcat(info, " screams \"You are already dead.\"");
                    break;
                case 9:
                    strcat(info, " says \"Maybe you aren't ");
                    strcat(info, you.your_name);
                    strcat(info, ". It doesn't matter.\"");
                    break;
                case 10:
                    strcat(info, " screams \"I love blood!\"");
                    break;
                }
                break;          // end Urug

            case MONS_SNORG:    // troll
                switch (random2(14))
                {
                case 0:
                    strcat(info, " grins.");
                    break;
                case 1:
                case 2:
                case 3:
                    strcat(info, " smells terribly.");
                    break;
                case 4:
                case 5:
                case 6:
                    strcat(info, " looks very hungry.");
                    break;
                case 7:
                    strcat(info, " screams \"Snack!\"");
                    break;
                case 8:
                    strcat(info, " roars.");
                    break;
                case 9:
                    strcat(info, " says \"Food!\"");
                    break;
                case 10:
                    strcat(info, " screams \"Snorg hungry!\"");
                    break;
                case 11:
                    strcat(info, " screams \"Snorg very very hungry!\"");
                case 12:
                    strcat(info, " says \"Snorg will eat you.\"");
                    break;
                case 13:
                    strcat(info, " says \"Snorg happy to meet food!\"");
                    break;
                }
                break;          // end Snorg

            case MONS_XTAHUA:   // ancient dragon
                switch (random2(13))
                {
                case 0:
                    strcat(info, " roars \"DIE!\"");
                    break;
                case 1:
                    strcat(info, " roars \"YOU BORE ME SO!\"");
                    break;
                case 2:
                    strcat(info, " roars \"YOU WILL DIE SOON!\"");
                    break;
                case 3:
                    strcat(info, " roars \"I HATE BEING BOTHERED!\"");
                    break;
                case 4:
                    strcat(info, " roars \"YOU WILL END UP LIKE A SNACK!\"");
                    break;
                case 5:
                    strcat(info, " roars \"I HATE ADVENTURERS!\"");
                    break;
                case 6:
                    strcat(info, " roars \"FACE MY WRATH!\"");
                    break;
                case 7:
                    strcat(info, " looks upset.");
                    break;
                case 8:
                    strcat(info,
                           " roars \"COMING HERE WAS YOUR LAST MISTAKE!\"");
                    break;
                case 9:
                    strcat(info,
                           " roars \"I'VE KILLED HUNDREDS OF ADVENTURERS LIKE YOU!\"");
                    break;
                case 10:
                case 11:
                case 12:
                    strcat(info, " roars horribly.");
                    mpr(info, MSGCH_TALK);
                    mpr("You are afraid.", MSGCH_WARN);
                    return true;
                    break;
                }
                break;          // end Xtahua

            case MONS_BORIS:    // ancient lich
                switch (random2(18))
                {
                case 0:
                    strcat(info, " says \"I didn't invite you.\"");
                    break;
                case 1:
                    strcat(info, " says \"You can't imagine my power.\"");
                    break;
                case 2:
                    strcat(info,
                           " says \"Orb? You want the Orb? You'll never get it.\"");
                    break;
                case 3:
                    strcat(info,
                           " says \"The world, the flesh, and the devil.\"");
                    break;
                case 4:
                    strcat(info, " gestures.");
                    break;
                case 5:
                    strcat(info, " stares at you.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel drained.", MSGCH_WARN);
                    return true;
                    break;
                case 6:
                    strcat(info, " stares at you.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel weakened.", MSGCH_WARN);
                    return true;
                    break;
                case 7:
                    strcat(info, " stares at you.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You feel troubles.", MSGCH_WARN);
                    return true;
                    break;
                case 8:
                    strcat(info,
                           " say \"Magic. You know nothing about it.\"");
                        break;
                case 9:
                    strcat(info, " says \"My power is unlimited.\"");
                    break;
                case 10:
                    strcat(info,
                           " says \"You can not kill me. I'm immortal.\"");
                    break;
                case 11:
                    strcat(info, " casts spell.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You equipment sudddenly weights more.", MSGCH_WARN);
                    return true;
                    break;
                case 12:
                    strcat(info,
                           " says \"I know the secret of eternal life. You don't.\"");
                    break;
                case 13:
                    strcat(info, " smiles crazily.");
                    break;
                case 14:
                    strcat(info, " casts spell.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You resist.");
                    return true;
                    break;
                case 15:
                    strcat(info, " casts spell.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("Suddenly you are surrounded with pale green light.", MSGCH_WARN);
                    return true;
                    break;
                case 16:
                    strcat(info, " casts spell.");
                    mpr(info, MSGCH_MONSTER_SPELL);
                    mpr("You have terrible head-ache.", MSGCH_WARN);
                    return true;
                    break;
                case 17:
                    strcat(info,
                           " says \"I know your future. Your future is death.\"");
                    break;
                }
                break;          // end BORIS


            case MONS_DEATH_COB:
                if (one_chance_in(2000))
                {
                    mpr("The death cob makes a corny joke.", MSGCH_TALK);
                    return true;
                }
                return false;
                break;          // mv: I don't know what could _cob_ say
                                // any suggestions?
                                // this is "good" enough -- bwr

            case MONS_KILLER_KLOWN:     // Killer Klown - guess!
                switch (random2(10))
                {
                case 0:
                    strcat(info, " giggles crazily.");
                    break;
                case 1:
                    strcat(info, " laughs merrily.");
                    break;
                case 2:
                    strcat(info, " beckons to you.");
                    break;
                case 3:
                    strcat(info, " does a flip.");
                    break;
                case 4:
                    strcat(info, " does a somersault.");
                    break;
                case 5:
                    strcat(info, " smiles at you.");
                    break;
                case 6:
                    strcat(info, " grins with merry abandon.");
                    break;
                case 7:
                    strcat(info, " howls with blood-lust!");
                    break;
                case 8:
                    strcat(info, " pokes out its tongue.");
                    break;
                case 9:
                    strcat(info, " says \"Come and play with me!\"");
                    break;
                }
                break;          // end Killer Klown
            default:
                strcat(info,
                       " says \"I don't know what to say. It's a bug.\"");
                break;
            }                   // end monster->type - monster type switch
        }                       // end default
    }                           // end switch monster->behavior

    mpr(info, msg_type);
    return true;
}                               // end mons_speaks = end of routine
