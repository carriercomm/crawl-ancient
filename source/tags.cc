/*
 *  File:       tags.cc
 *  Summary:    Auxilary functions to make savefile versioning simpler.
 *  Written by: Gordon Lipford
 *
 *  Change History (most recent first):
 *
 *   <2>   16 Mar 2001      GDL    Added TAG_LEVEL_ATTITUDE
 *   <1>   27 Jan 2001      GDL    Created
 */

/* ------------------------- how tags work ----------------------------------

1. Tag types are enumerated in enum.h, from TAG_VERSION (more a placeholder
   than anything else, it is not actually saved as a tag) to TAG_XXX. NUM_TAGS
   is equal to the actual number of defined tags.

2. Tags are created with tag_construct(),  which forwards the construction
   request appropriately.   tag_write() is then used to write the tag to an
   output stream.

3. Tags are parsed with tag_read(), which tries to read a tag header and then
   forwards the request appropriately, returning the ID of the tag it found,
   or zero if no tag was found.

4. In order to know which tags are used by a particular file type,  a client
   calls tag_set_expected( fileType ), which sets up an array of chars.
   Within the array, a value of 1 means the tag is expected; -1 means that
   the tag is not expected.  A client can then set values in this array to
   anything other than 1 to indicate a successful tag_read() of that tag.

5. A case should be provided in tag_missing() for any tag which might be
   missing from a tagged save file.  For example,  if a developer adds
   TAG_YOU_NEW_STUFF to the player save file,  he would have to provide a
   case in tag_missing() for this tag since it might not be there in
   earlier savefiles.   The tags defined with the original tag system (and
   so not needing cases in tag_missing()) are as follows:

    TAG_YOU = 1,                        // 'you' structure
    TAG_YOU_ITEMS,                      // your items
    TAG_YOU_DUNGEON,                    // dungeon specs (stairs, branches, features)
    TAG_LEVEL,                          // various grids & clouds
    TAG_LEVEL_ITEMS,                    // items/traps
    TAG_LEVEL_MONSTERS,                 // monsters
    TAG_GHOST,                          // ghost

6. The marshalling and unmarshalling of data is done in network order and
   is meant to keep savefiles cross-platform.  They are non-ascii - always
   FTP in binary mode.  Note also that the marshalling sizes are 1,2, and 4
   for byte, short, and long - assuming that 'int' would have been
   insufficient on 16 bit systems (and that Crawl otherwise lacks
   system-indepedent data types).

 */

#include <string.h>            // for memcpy

#ifdef LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef USE_EMX
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef MAC
#include <stat.h>
#else
#include <sys/stat.h>
#endif

#include "AppHdr.h"
#include "enum.h"
#include "externs.h"
#include "files.h"
#include "itemname.h"
#include "monstuff.h"
#include "randart.h"
#include "stuff.h"
#include "spells4.h"
#include "tags.h"

// THE BIG IMPORTANT TAG CONSTRUCTION/PARSE BUFFER
static char *tagBuffer = NULL;

// These three are defined in overmap.cc
extern FixedArray < unsigned char, MAX_LEVELS, MAX_BRANCHES > altars_present;
extern FixedVector < char, MAX_BRANCHES > stair_level;
extern FixedArray < unsigned char, MAX_LEVELS, MAX_BRANCHES > feature;

extern unsigned char your_sign; /* these two are defined in view.cc */
extern unsigned char your_colour;

// temp file pairs used for file level cleanup
FixedArray < bool, MAX_LEVELS, MAX_BRANCHES > tmp_file_pairs;


// static helpers
static void tag_construct_you(struct tagHeader &th);
static void tag_construct_you_items(struct tagHeader &th);
static void tag_construct_you_dungeon(struct tagHeader &th);
static void tag_read_you(struct tagHeader &th);
static void tag_read_you_items(struct tagHeader &th);
static void tag_read_you_dungeon(struct tagHeader &th);

static void tag_construct_level(struct tagHeader &th);
static void tag_construct_level_items(struct tagHeader &th);
static void tag_construct_level_monsters(struct tagHeader &th);
static void tag_construct_level_attitude(struct tagHeader &th);
static void tag_read_level(struct tagHeader &th);
static void tag_read_level_items(struct tagHeader &th);
static void tag_read_level_monsters(struct tagHeader &th, char minorVersion);
static void tag_read_level_attitude(struct tagHeader &th);
static void tag_missing_level_attitude();

static void tag_construct_ghost(struct tagHeader &th);
static void tag_read_ghost(struct tagHeader &th);

// provide a wrapper for file writing, just in case.
int write2(FILE * file, char *buffer, unsigned int count)
{
    return fwrite(buffer, 1, count, file);
}

// provide a wrapper for file reading, just in case.
int read2(FILE * file, char *buffer, unsigned int count)
{
    return fread(buffer, 1, count, file);
}

void marshallByte(struct tagHeader &th, char data)
{
    tagBuffer[th.offset] = data;
    th.offset += 1;
}

char unmarshallByte(struct tagHeader &th)
{
    char data = tagBuffer[th.offset];
    th.offset += 1;
    return data;
}

// marshall 2 byte short in network order
void marshallShort(struct tagHeader &th, short data)
{
    char b2 = (char)(data & 0x00FF);
    char b1 = (char)((data & 0xFF00) >> 8);

    tagBuffer[th.offset] = b1;
    tagBuffer[th.offset + 1] = b2;

    th.offset += 2;
}

// unmarshall 2 byte short in network order
short unmarshallShort(struct tagHeader &th)
{
    short b1 = tagBuffer[th.offset];
    short b2 = tagBuffer[th.offset + 1];

    short data = (b1 << 8) | (b2 & 0x00FF);

    th.offset += 2;
    return data;
}

// marshall 4 byte int in network order
void marshallLong(struct tagHeader &th, long data)
{
    char b4 = (char) (data & 0x000000FF);
    char b3 = (char)((data & 0x0000FF00) >> 8);
    char b2 = (char)((data & 0x00FF0000) >> 16);
    char b1 = (char)((data & 0xFF000000) >> 24);

    tagBuffer[th.offset] = b1;
    tagBuffer[th.offset + 1] = b2;
    tagBuffer[th.offset + 2] = b3;
    tagBuffer[th.offset + 3] = b4;

    th.offset += 4;
}

// unmarshall 4 byte int in network order
long unmarshallLong(struct tagHeader &th)
{
    long b1 = tagBuffer[th.offset];
    long b2 = tagBuffer[th.offset + 1];
    long b3 = tagBuffer[th.offset + 2];
    long b4 = tagBuffer[th.offset + 3];

    long data = (b1 << 24) | ((b2 & 0x000000FF) << 16);
    data |= ((b3 & 0x000000FF) << 8) | (b4 & 0x000000FF);

    th.offset += 4;
    return data;
}

// single precision float -- marshall in network order.
void marshallFloat(struct tagHeader &th, float data)
{
    long intBits = *((long *)(&data));
    marshallLong(th, intBits);
}

// single precision float -- unmarshall in network order.
float unmarshallFloat(struct tagHeader &th)
{
    long intBits = unmarshallLong(th);

    return *((float *)(&intBits));
}

// string -- marshall length & string data
void marshallString(struct tagHeader &th, char *data, int maxSize)
{
    // allow for very long strings.
    short len = strlen(data);
    if (maxSize > 0 && len > maxSize)
        len = maxSize;
    marshallShort(th, len);

    // put in the actual string -- we'll null terminate on
    // unmarshall.
    memcpy(&tagBuffer[th.offset], data, len);

    th.offset += len;
}

// string -- unmarshall length & string data
void unmarshallString(struct tagHeader &th, char *data, int maxSize)
{
    // get length
    short len = unmarshallShort(th);
    int copylen = len;
    if (len > maxSize && maxSize > 0)
        copylen = maxSize;

    // read the actual string and null terminate
    memcpy(data, &tagBuffer[th.offset], copylen);
    data[copylen] = '\0';

    th.offset += len;
}

// boolean (to avoid system-dependant bool implementations)
void marshallBoolean(struct tagHeader &th, bool data)
{
    char charRep = 0;       // for false
    if (data)
        charRep = 1;

    tagBuffer[th.offset] = charRep;
    th.offset += 1;
}

// boolean (to avoid system-dependant bool implementations)
bool unmarshallBoolean(struct tagHeader &th)
{
    bool data;

    if (tagBuffer[th.offset] == 1)
        data = true;
    else
        data = false;

    th.offset += 1;
    return data;
}

// PUBLIC TAG FUNCTIONS
void tag_init(long largest_tag)
{
    if (tagBuffer != NULL)
        return;

    tagBuffer = (char *)malloc(largest_tag);
}

void tag_construct(struct tagHeader &th, int tagID)
{
    th.offset = 0;
    th.tagID = tagID;

    switch(tagID)
    {
        case TAG_YOU:
            tag_construct_you(th);
            break;
        case TAG_YOU_ITEMS:
            tag_construct_you_items(th);
            break;
        case TAG_YOU_DUNGEON:
            tag_construct_you_dungeon(th);
            break;
        case TAG_LEVEL:
            tag_construct_level(th);
            break;
        case TAG_LEVEL_ITEMS:
            tag_construct_level_items(th);
            break;
        case TAG_LEVEL_MONSTERS:
            tag_construct_level_monsters(th);
            break;
        case TAG_LEVEL_ATTITUDE:
            tag_construct_level_attitude(th);
            break;
        case TAG_GHOST:
            tag_construct_ghost(th);
            break;
        default:
            // I don't know how to make that!
            break;
    }
}

void tag_write(struct tagHeader &th, FILE *saveFile)
{
    const int tagHdrSize = 6;
    int tagSize=0;

    char swap[tagHdrSize];

    // make sure there is some data to write!
    if (th.offset == 0)
        return;

    // special case: TAG_VERSION.  Skip tag header.
    if (th.tagID != TAG_VERSION)
    {
        // swap out first few bytes
        memcpy(swap, tagBuffer, tagHdrSize);

        // save tag size
        tagSize = th.offset;

        // swap in the header
        th.offset = 0;
        marshallShort(th, th.tagID);
        marshallLong(th, tagSize);

        // write header
        write2(saveFile, tagBuffer, th.offset);

        // swap real data back in
        memcpy(tagBuffer, swap, tagHdrSize);

        // reset tag size
        th.offset = tagSize;
    }

    // write tag data
    write2(saveFile, tagBuffer, th.offset);
    return;
}

// minorVersion is available for any sub-readers that need it
// (like TAG_LEVEL_MONSTERS)
int tag_read(FILE *fp, char minorVersion)
{
    const int tagHdrSize = 6;
    struct tagHeader hdr, th;
    th.offset = 0;

    // read tag header
    if (read2(fp, tagBuffer, tagHdrSize) != tagHdrSize)
        return 0;

    // unmarshall tag type and length (not including header)
    hdr.tagID = unmarshallShort(th);
    hdr.offset = unmarshallLong(th);

    // sanity check
    if (hdr.tagID <= 0 || hdr.offset <= 0)
        return 0;

    // now reset th and read actual data
    th.offset = 0;
    if (read2(fp, tagBuffer, hdr.offset) != hdr.offset)
        return 0;

    // ok, we have data now.
    switch(hdr.tagID)
    {
        case TAG_YOU:
            tag_read_you(th);
            break;
        case TAG_YOU_ITEMS:
            tag_read_you_items(th);
            break;
        case TAG_YOU_DUNGEON:
            tag_read_you_dungeon(th);
            break;
        case TAG_LEVEL:
            tag_read_level(th);
            break;
        case TAG_LEVEL_ITEMS:
            tag_read_level_items(th);
            break;
        case TAG_LEVEL_MONSTERS:
            tag_read_level_monsters(th, minorVersion);
            break;
        case TAG_LEVEL_ATTITUDE:
            tag_read_level_attitude(th);
            break;
        case TAG_GHOST:
            tag_read_ghost(th);
            break;
        default:
            // I don't know how to read that!
            return 0;
    }

    return hdr.tagID;
}


// older savefiles might want to call this to get a tag
// properly initialized if it wasn't part of the savefile.
// For now, none are supported.

// This function will be called AFTER all other tags for
// the savefile are read,  so everything that can be
// initialized should have been by now.

// minorVersion is available for any child functions that need
// it (currently none)
void tag_missing(int tag, char minorVersion)
{
    switch(tag)
    {
        case TAG_LEVEL_ATTITUDE:
            tag_missing_level_attitude();
            break;
        default:
            perror("Tag %d is missing;  file is likely corrupt.");
            end(-1);
    }
}

// utility
void tag_set_expected(char tags[], int fileType)
{
    int i;

    for(i=0; i<NUM_TAGS; i++)
    {
        tags[i] = -1;
        switch(fileType)
        {
            case TAGTYPE_PLAYER:
                if (i >= TAG_YOU && i <=TAG_YOU_DUNGEON)
                    tags[i] = 1;
                break;
            case TAGTYPE_LEVEL:
                if (i >= TAG_LEVEL && i <= TAG_LEVEL_ATTITUDE)
                    tags[i] = 1;
                break;
            case TAGTYPE_GHOST:
                if (i == TAG_GHOST)
                    tags[i] = 1;
            default:
                // I don't know what kind of file that is!
                break;
        }
    }
}

// NEVER _MODIFY_ THE CONSTRUCT/READ FUNCTIONS,  EVER.  THAT IS THE WHOLE POINT
// OF USING TAGS.  Apologies for the screaming.

// Note anyway that the formats are somewhat flexible;  you could change map
// size,  the # of slots in player inventory,  etc.  Constants like GXM,
// NUM_EQUIP, and NUM_DURATIONS are saved,  so the appropriate amount will
// be restored even if a later version increases these constants.

// --------------------------- player tags (foo.sav) -------------------- //
static void tag_construct_you(struct tagHeader &th)
{
    int i,j;

    marshallString(th, you.your_name, 30);

    marshallByte(th,you.religion);
    marshallByte(th,you.piety);
    marshallByte(th,you.invis);
    marshallByte(th,you.conf);
    marshallByte(th,you.paralysis);
    marshallByte(th,you.slow);
    marshallByte(th,you.shock_shield);
    marshallByte(th,you.rotting);
    marshallByte(th,you.exhausted);
    marshallByte(th,you.deaths_door);
    marshallByte(th,your_sign);
    marshallByte(th,your_colour);
    marshallByte(th,you.pet_target);

    marshallByte(th,you.max_level);
    marshallByte(th,you.where_are_you);
    marshallByte(th,you.char_direction);
    marshallByte(th,you.your_level);
    marshallByte(th,you.is_undead);
    marshallByte(th,you.special_wield);
    marshallByte(th,you.berserker);
    marshallByte(th,you.berserk_penalty);
    marshallByte(th,you.level_type);
    marshallByte(th,you.synch_time);
    marshallByte(th,you.disease);
    marshallByte(th,you.species);

    marshallShort(th, you.hp);

    if (you.haste > 215)
        you.haste = 215;

    marshallByte(th,you.haste);

    if (you.might > 215)
        you.might = 215;

    marshallByte(th,you.might);

    if (you.levitation > 215)
        you.levitation = 215;

    marshallByte(th,you.levitation);

    if (you.poison > 215)
        you.poison = 215;

    marshallByte(th,you.poison);

    marshallShort(th, you.hunger);

    // how many you.equip?
    marshallByte(th, NUM_EQUIP);
    for (i = 0; i < NUM_EQUIP; ++i)
        marshallByte(th,you.equip[i]);

    marshallByte(th,you.magic_points);
    marshallByte(th,you.max_magic_points);
    marshallByte(th,you.strength);
    marshallByte(th,you.intel);
    marshallByte(th,you.dex);
    marshallByte(th,you.confusing_touch);
    marshallByte(th,you.sure_blade);
    marshallByte(th,you.hit_points_regeneration);
    marshallByte(th,you.magic_points_regeneration);

    marshallShort(th, you.hit_points_regeneration * 100);
    marshallLong(th, you.experience);
    marshallLong(th, you.gold);

    marshallByte(th,you.char_class);
    marshallByte(th,you.experience_level);
    marshallLong(th, you.exp_available);

    /* max values */
    marshallByte(th,you.max_strength);
    marshallByte(th,you.max_intel);
    marshallByte(th,you.max_dex);

    marshallShort(th, you.base_hp);
    marshallShort(th, you.base_hp2);
    marshallShort(th, you.base_magic_points);
    marshallShort(th, you.base_magic_points2);

    marshallShort(th, you.x_pos);
    marshallShort(th, you.y_pos);

    marshallString(th, you.class_name, 30);

    marshallShort(th, you.burden);

    // how many spells?
    marshallByte(th, 25);
    for (i = 0; i < 25; ++i)
        marshallByte(th,you.spells[i]);

    // how many skills?
    marshallByte(th, 50);
    for (j = 0; j < 50; ++j)
    {
        marshallByte(th,you.skills[j]);   /* skills! */
        marshallByte(th,you.practise_skill[j]);   /* skills! */
        marshallLong(th,you.skill_points[j]);
    }

    // how many durations?
    marshallByte(th, NUM_DURATIONS);
    for (j = 0; j < NUM_DURATIONS; ++j)
        marshallByte(th,you.duration[j]);

    // how many attributes?
    marshallByte(th, 30);
    for (j = 0; j < 30; ++j)
        marshallByte(th,you.attribute[j]);

    // how many mutations/demon powers?
    marshallShort(th, 100);
    for (j = 0; j < 100; ++j)
    {
        marshallByte(th,you.mutation[j]);
        marshallByte(th,you.demon_pow[j]);
    }

    // how many penances?
    marshallByte(th, 21);
    for (i = 0; i < 21; i++)
        marshallByte(th, you.penance[i]);

    marshallByte(th, you.gift_timeout);
    marshallByte(th, you.normal_vision);
    marshallByte(th, you.current_vision);
    marshallByte(th, you.hell_exit);

    // elapsed time
    marshallFloat(th, (float)you.elapsed_time);
}

static void tag_construct_you_items(struct tagHeader &th)
{
    int i,j;

    // how many inventory slots?
    marshallByte(th, ENDOFPACK);
    for (i = 0; i < ENDOFPACK; ++i)
    {
        marshallByte(th,you.inv_class[i]);
        marshallByte(th,you.inv_type[i]);
        marshallByte(th,you.inv_plus[i]);
        marshallByte(th,you.inv_dam[i]);
        marshallByte(th,you.inv_colour[i]);
        marshallByte(th,you.inv_ident[i]);
        marshallShort(th,you.inv_quantity[i]);
        marshallByte(th,you.inv_plus2[i]);
    }

    // item descrip for each type & subtype
    // how many types?
    marshallByte(th, 5);
    // how many subtypes?
    marshallByte(th, 50);
    for (i = 0; i < 5; ++i)
    {
        for (j = 0; j < 50; ++j)
            marshallByte(th, you.item_description[i][j]);
    }

    // identification status
    // how many types?
    marshallByte(th, 4);
    // how many subtypes?
    marshallByte(th, 50);

    // this is really dumb. We copy the id[] array from itemname
    // to the stack,  for no good reason that I can see.
    char identy[4][50];

    save_id(identy);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 50; ++j)
            marshallByte(th, identy[i][j]);
    }

    // how many unique items?
    marshallByte(th, 50);
    for (j = 0; j < 50; ++j)
    {
        marshallByte(th,you.unique_items[j]);     /* unique items */
        marshallByte(th,you.had_item[j]);
    }

    // how many unrandarts?
    marshallShort(th, NO_UNRANDARTS);
    for (j = 0; j < NO_UNRANDARTS; ++j)
        marshallBoolean(th, does_unrandart_exist(j));
}

static void tag_construct_you_dungeon(struct tagHeader &th)
{
    int i,j;

    // how many unique creatures?
    marshallByte(th, 50);
    for (j = 0; j < 50; ++j)
        marshallByte(th,you.unique_creatures[j]); /* unique beasties */

    // how many branches?
    marshallByte(th, MAX_BRANCHES);
    for (j = 0; j < 30; ++j)
    {
        marshallByte(th,you.branch_stairs[j]);
        marshallByte(th,stair_level[j]);
    }

    // how many levels?
    marshallShort(th, MAX_LEVELS);
    for (i = 0; i < MAX_LEVELS; ++i)
    {
        for (j = 0; j < MAX_BRANCHES; ++j)
        {
            marshallByte(th,altars_present[i][j]);
            marshallByte(th,feature[i][j]);
            marshallBoolean(th,tmp_file_pairs[i][j]);
        }
    }
}

static void tag_read_you(struct tagHeader &th)
{
    int i,j;
    char count_c;
    short count_s;

    unmarshallString(th, you.your_name, 30);

    you.religion = unmarshallByte(th);
    you.piety = unmarshallByte(th);
    you.invis = unmarshallByte(th);
    you.conf = unmarshallByte(th);
    you.paralysis = unmarshallByte(th);
    you.slow = unmarshallByte(th);
    you.shock_shield = unmarshallByte(th);
    you.rotting = unmarshallByte(th);
    you.exhausted = unmarshallByte(th);
    you.deaths_door = unmarshallByte(th);
    your_sign = unmarshallByte(th);
    your_colour = unmarshallByte(th);
    you.pet_target = unmarshallByte(th);

    you.max_level = unmarshallByte(th);
    you.where_are_you = unmarshallByte(th);
    you.char_direction = unmarshallByte(th);
    you.your_level = unmarshallByte(th);
    you.is_undead = unmarshallByte(th);
    you.special_wield = unmarshallByte(th);
    you.berserker = unmarshallByte(th);
    you.berserk_penalty = unmarshallByte(th);
    you.level_type = unmarshallByte(th);
    you.synch_time = unmarshallByte(th);
    you.disease = unmarshallByte(th);
    you.species = unmarshallByte(th);
    you.hp = unmarshallShort(th);
    you.haste = unmarshallByte(th);
    you.might = unmarshallByte(th);
    you.levitation = unmarshallByte(th);
    you.poison = unmarshallByte(th);
    you.hunger = unmarshallShort(th);

    // how many you.equip?
    count_c = unmarshallByte(th);
    for (i = 0; i < count_c; ++i)
        you.equip[i] = unmarshallByte(th);

    you.magic_points = unmarshallByte(th);
    you.max_magic_points = unmarshallByte(th);
    you.strength = unmarshallByte(th);
    you.intel = unmarshallByte(th);
    you.dex = unmarshallByte(th);
    you.confusing_touch = unmarshallByte(th);
    you.sure_blade = unmarshallByte(th);
    you.hit_points_regeneration = unmarshallByte(th);
    you.magic_points_regeneration = unmarshallByte(th);

    you.hit_points_regeneration = unmarshallShort(th) / 100;
    you.experience = unmarshallLong(th);
    you.gold = unmarshallLong(th);

    you.char_class = unmarshallByte(th);
    you.experience_level = unmarshallByte(th);
    you.exp_available = unmarshallLong(th);

    /* max values */
    you.max_strength = unmarshallByte(th);
    you.max_intel = unmarshallByte(th);
    you.max_dex = unmarshallByte(th);

    you.base_hp = unmarshallShort(th);
    you.base_hp2 = unmarshallShort(th);
    you.base_magic_points = unmarshallShort(th);
    you.base_magic_points2 = unmarshallShort(th);

    you.x_pos = unmarshallShort(th);
    you.y_pos = unmarshallShort(th);

    unmarshallString(th, you.class_name, 30);

    you.burden = unmarshallShort(th);

    // how many spells?
    you.spell_no = 0;
    count_c = unmarshallByte(th);
    for (i = 0; i < count_c; ++i)
    {
        you.spells[i] = unmarshallByte(th);
        if (you.spells[i] != SPELL_NO_SPELL)
            you.spell_no ++;
    }

    // how many skills?
    count_c = unmarshallByte(th);
    for (j = 0; j < count_c; ++j)
    {
        you.skills[j] = unmarshallByte(th);
        you.practise_skill[j] = unmarshallByte(th);
        you.skill_points[j] = unmarshallLong(th);
    }

    // how many durations?
    count_c = unmarshallByte(th);
    for (j = 0; j < count_c; ++j)
        you.duration[j] = unmarshallByte(th);

    // how many attributes?
    count_c = unmarshallByte(th);
    for (j = 0; j < count_c; ++j)
        you.attribute[j] = unmarshallByte(th);

    // how many mutations/demon powers?
    count_s = unmarshallShort(th);
    for (j = 0; j < count_s; ++j)
    {
        you.mutation[j] = unmarshallByte(th);
        you.demon_pow[j] = unmarshallByte(th);
    }

    // how many penances?
    count_c = unmarshallByte(th);
    for (i = 0; i < count_c; i++)
        you.penance[i] = unmarshallByte(th);

    you.gift_timeout = unmarshallByte(th);
    you.normal_vision = unmarshallByte(th);
    you.current_vision = unmarshallByte(th);
    you.hell_exit = unmarshallByte(th);

    // elapsed time
    you.elapsed_time = (double)unmarshallFloat(th);
}

static void tag_read_you_items(struct tagHeader &th)
{
    int i,j;
    char count_c, count_c2;
    short count_s;

    // how many inventory slots?
    count_c = unmarshallByte(th);
    for (i = 0; i < count_c; ++i)
    {
        you.inv_class[i] = unmarshallByte(th);
        you.inv_type[i] = unmarshallByte(th);
        you.inv_plus[i] = unmarshallByte(th);
        you.inv_dam[i] = unmarshallByte(th);
        you.inv_colour[i] = unmarshallByte(th);
        you.inv_ident[i] = unmarshallByte(th);
        you.inv_quantity[i] = unmarshallShort(th);
        you.inv_plus2[i] = unmarshallByte(th);
    }

    // item descrip for each type & subtype
    // how many types?
    count_c = unmarshallByte(th);
    // how many subtypes?
    count_c2 = unmarshallByte(th);
    for (i = 0; i < count_c; ++i)
    {
        for (j = 0; j < count_c2; ++j)
            you.item_description[i][j] = unmarshallByte(th);
    }

    // identification status
    // how many types?
    count_c = unmarshallByte(th);
    // how many subtypes?
    count_c2 = unmarshallByte(th);

    // argh.. this is awful.
    for (i = 0; i < count_c; ++i)
    {
        for (j = 0; j < count_c2; ++j)
        {
            char ch;
            ch = unmarshallByte(th);

            switch (i)
            {
                case 0:
                    set_id(OBJ_WANDS, j, ch);
                    break;
                case 1:
                    set_id(OBJ_SCROLLS, j, ch);
                    break;
                case 2:
                    set_id(OBJ_JEWELLERY, j, ch);
                    break;
                case 3:
                    set_id(OBJ_POTIONS, j, ch);
                    break;
            }
        }
    }

    // how many unique items?
    count_c = unmarshallByte(th);
    for (j = 0; j < count_c; ++j)
    {
        you.unique_items[j] = unmarshallByte(th);
        you.had_item[j] = unmarshallByte(th);
    }

    // how many unrandarts?
    count_s = unmarshallShort(th);
    for (j = 0; j < count_s; ++j)
        set_unrandart_exist(j, unmarshallBoolean(th));
}

static void tag_read_you_dungeon(struct tagHeader &th)
{
    int i,j;
    char count_c;
    short count_s;

    // how many unique creatures?
    count_c = unmarshallByte(th);
    for (j = 0; j < count_c; ++j)
        you.unique_creatures[j] = unmarshallByte(th);

    // how many branches?
    count_c = unmarshallByte(th);
    for (j = 0; j < count_c; ++j)
    {
        you.branch_stairs[j] = unmarshallByte(th);
        stair_level[j] = unmarshallByte(th);
    }

    // how many levels?
    count_s = unmarshallShort(th);
    for (i = 0; i < count_s; ++i)
    {
        for (j = 0; j < count_c; ++j)
        {
            altars_present[i][j] = unmarshallByte(th);
            feature[i][j] = unmarshallByte(th);
            tmp_file_pairs[i][j] = unmarshallBoolean(th);
        }
    }
}

// ------------------------------- level tags ---------------------------- //

static void tag_construct_level(struct tagHeader &th)
{
    int i;
    int count_x, count_y;

    marshallFloat(th, (float)you.elapsed_time);

    // map grids
    // how many X?
    marshallShort(th, GXM);
    // how many Y?
    marshallShort(th, GYM);
    for (count_x = 0; count_x < GXM; count_x++)
    {
        for (count_y = 0; count_y < GYM; count_y++)
        {
            marshallByte(th, grd[count_x][count_y]);
            marshallByte(th, env.map[count_x][count_y]);
            marshallByte(th, env.cgrid[count_x][count_y]);
        }
    }

    marshallShort(th, env.cloud_no);

    // how many clouds?
    marshallShort(th, MAX_CLOUDS);
    for (i = 0; i < MAX_CLOUDS; i++)
    {
        marshallByte(th, env.cloud_x[i]);
        marshallByte(th, env.cloud_y[i]);
        marshallByte(th, env.cloud_type[i]);
        marshallShort(th, env.cloud_decay[i]);
    }

    // how many shops?
    marshallByte(th, 5);
    for (i = 0; i < 5; i++)
    {
        marshallByte(th, env.keeper_name[i][0]);
        marshallByte(th, env.keeper_name[i][1]);
        marshallByte(th, env.keeper_name[i][2]);
        marshallByte(th, env.shop_x[i]);
        marshallByte(th, env.shop_y[i]);
        marshallByte(th, env.shop_greed[i]);
        marshallByte(th, env.shop_type[i]);
        marshallByte(th, env.shop_level[i]);
    }
}

static void tag_construct_level_items(struct tagHeader &th)
{
    int i;

    // how many traps?
    marshallShort(th, MAX_TRAPS);
    for (i = 0; i < MAX_TRAPS; ++i)
    {
        marshallByte(th, env.trap_type[i]);
        marshallByte(th, env.trap_x[i]);
        marshallByte(th, env.trap_y[i]);
    }

    // how many items?
    marshallShort(th, MAX_ITEMS);
    for (i = 0; i < MAX_ITEMS; ++i)
    {
        marshallByte(th, mitm.base_type[i]);
        marshallByte(th, mitm.sub_type[i]);
        marshallByte(th, mitm.pluses[i]);
        marshallByte(th, mitm.pluses2[i]);
        marshallByte(th, mitm.special[i]);
        marshallLong(th, mitm.quantity[i]);

        marshallByte(th, mitm.colour[i]);
        marshallByte(th, mitm.x[i]);
        marshallByte(th, mitm.y[i]);
        marshallByte(th, mitm.id[i]);

        marshallShort(th, mitm.link[i]);                //  unused
        marshallShort(th, igrd[mitm.x[i]][mitm.y[i]]);  //  unused
    }
}

static void tag_construct_level_monsters(struct tagHeader &th)
{
    int i,j;

    // how many mons_alloc?
    marshallByte(th, 20);
    for (i = 0; i < 20; ++i)
        marshallShort(th, env.mons_alloc[i]);

    // how many monsters?
    marshallShort(th, MAX_MONSTERS);
    // how many monster enchantments?
    marshallByte(th, NUM_MON_ENCHANTS);
    // how many monster inventory slots?
    marshallByte(th, NUM_MONSTER_SLOTS);

    for (i = 0; i < MAX_MONSTERS; i++)
    {
        marshallByte(th, menv[i].armor_class);
        marshallByte(th, menv[i].evasion);
        marshallByte(th, menv[i].hit_dice);
        marshallByte(th, menv[i].speed);
        marshallByte(th, menv[i].speed_increment);
        marshallByte(th, menv[i].behavior);
        marshallByte(th, menv[i].x);
        marshallByte(th, menv[i].y);
        marshallByte(th, menv[i].target_x);
        marshallByte(th, menv[i].target_y);
        marshallByte(th, menv[i].flags);

        for (j = 0; j < NUM_MON_ENCHANTS; j++)
            marshallByte(th, menv[i].enchantment[j]);

        marshallShort(th, menv[i].type);
        marshallShort(th, menv[i].hit_points);
        marshallShort(th, menv[i].max_hit_points);
        marshallShort(th, menv[i].number);

        for (j = 0; j < NUM_MONSTER_SLOTS; j++)
            marshallShort(th, menv[i].inv[j]);
    }
}

void tag_construct_level_attitude(struct tagHeader &th)
{
    int i;

    // how many monsters?
    marshallShort(th, MAX_MONSTERS);

    for (i = 0; i < MAX_MONSTERS; i++)
    {
        marshallByte(th, menv[i].attitude);
        marshallShort(th, menv[i].foe);
    }
}


static void tag_read_level(struct tagHeader &th)
{
    int i,j;
    int gx, gy;

    env.elapsed_time = (double)unmarshallFloat(th);

    // map grids
    // how many X?
    gx = unmarshallShort(th);
    // how many Y?
    gy = unmarshallShort(th);
    for (i = 0; i < gx; i++)
    {
        for (j = 0; j < gy; j++)
        {
            grd[i][j] = unmarshallByte(th);
            env.map[i][j] = unmarshallByte(th);
            if (env.map[i][j] == 201)       // what is this??
                env.map[i][j] = 239;

            mgrd[i][j] = NON_MONSTER;
            env.cgrid[i][j] = unmarshallByte(th);
        }
    }

    env.cloud_no = unmarshallShort(th);

    // how many clouds?
    gx = unmarshallShort(th);
    for (i = 0; i < gx; i++)
    {
        env.cloud_x[i] = unmarshallByte(th);
        env.cloud_y[i] = unmarshallByte(th);
        env.cloud_type[i] = unmarshallByte(th);
        env.cloud_decay[i] = unmarshallShort(th);
    }

    // how many shops?
    gx = unmarshallByte(th);
    for (i = 0; i < gx; i++)
    {
        env.keeper_name[i][0] = unmarshallByte(th);
        env.keeper_name[i][1] = unmarshallByte(th);
        env.keeper_name[i][2] = unmarshallByte(th);
        env.shop_x[i] = unmarshallByte(th);
        env.shop_y[i] = unmarshallByte(th);
        env.shop_greed[i] = unmarshallByte(th);
        env.shop_type[i] = unmarshallByte(th);
        env.shop_level[i] = unmarshallByte(th);
    }
}

static void tag_read_level_items(struct tagHeader &th)
{
    int i;
    int count;

    // how many traps?
    count = unmarshallShort(th);
    for (i = 0; i < count; ++i)
    {
        env.trap_type[i] = unmarshallByte(th);
        env.trap_x[i] = unmarshallByte(th);
        env.trap_y[i] = unmarshallByte(th);
    }

    // how many items?
    count = unmarshallShort(th);
    for (i = 0; i < count; ++i)
    {
        mitm.base_type[i] = unmarshallByte(th);
        mitm.sub_type[i] = unmarshallByte(th);
        mitm.pluses[i] = unmarshallByte(th);
        mitm.pluses2[i] = unmarshallByte(th);
        mitm.special[i] = unmarshallByte(th);
        mitm.quantity[i] = unmarshallLong(th);

        mitm.colour[i] = unmarshallByte(th);
        mitm.x[i] = unmarshallByte(th);
        mitm.y[i] = unmarshallByte(th);
        mitm.id[i] = unmarshallByte(th);

        // pre 4.2 files had monster items stacked at (2,2) -- moved to (0,0)
        if (mitm.x[i] == 2 && mitm.y[i] == 2)
        {
            mitm.x[i] = 0;
            mitm.y[i] = 0;
        }

        unmarshallShort(th);            // mitm.link[] -- unused
        unmarshallShort(th);            // igrd[mitm.x[i]][mitm.y[i]] -- unused
    }
}

static void tag_read_level_monsters(struct tagHeader &th, char minorVersion)
{
    int i,j;
    int count, ecount, icount, turns;

    // how many mons_alloc?
    count = unmarshallByte(th);
    for (i = 0; i < count; ++i)
        env.mons_alloc[i] = unmarshallShort(th);

    // how many monsters?
    count = unmarshallShort(th);
    // how many monster enchantments?
    ecount = unmarshallByte(th);
    // how many monster inventory slots?
    icount = unmarshallByte(th);

    for (i = 0; i < count; i++)
    {
        menv[i].armor_class = unmarshallByte(th);
        menv[i].evasion = unmarshallByte(th);
        menv[i].hit_dice = unmarshallByte(th);
        menv[i].speed = unmarshallByte(th);
        menv[i].speed_increment = unmarshallByte(th);
        menv[i].behavior = unmarshallByte(th);
        menv[i].x = unmarshallByte(th);
        menv[i].y = unmarshallByte(th);
        menv[i].target_x = unmarshallByte(th);
        menv[i].target_y = unmarshallByte(th);
        menv[i].flags = unmarshallByte(th);

        // VERSION NOTICE:  for pre 4.2 files,  flags was either 0
        // or 1.  Now,  we can transfer ENCH_CREATED_FRIENDLY over
        // from the enchantments array to flags.
        // Also need to take care of ENCH_FRIEND_ABJ_xx flags

        for (j = 0; j < ecount; j++)
            menv[i].enchantment[j] = unmarshallByte(th);

        if (minorVersion < 2)
        {
            menv[i].flags = 0;
            for(j=0; j<NUM_MON_ENCHANTS; j++)
            {
                if (j>=ecount)
                    menv[i].enchantment[j] = ENCH_NONE;
                else
                {
                    if (menv[i].enchantment[j] == 71) // old ENCH_CREATED_FRIENDLY
                    {
                        menv[i].enchantment[j] = ENCH_NONE;
                        menv[i].flags |= MF_CREATED_FRIENDLY;
                    }
                    if (menv[i].enchantment[j] >= 65 // old ENCH_FRIEND_ABJ_I
                      && menv[i].enchantment[j] <= 70) // old ENCH_FRIEND_ABJ_VI
                    {
                        menv[i].enchantment[j] -= (65 - ENCH_ABJ_I);
                        menv[i].flags |= MF_CREATED_FRIENDLY;
                    }
                }
            }
        } // end  minorversion < 2

        menv[i].type = unmarshallShort(th);
        menv[i].hit_points = unmarshallShort(th);
        menv[i].max_hit_points = unmarshallShort(th);
        menv[i].number = unmarshallShort(th);

        for (j = 0; j < icount; j++)
            menv[i].inv[j] = unmarshallShort(th);

        // place monster & heal based on time gone (nice touch)
        if (menv[i].type != -1)
        {
            mgrd[menv[i].x][menv[i].y] = i;

            if (menv[i].hit_points < menv[i].max_hit_points)
            {
                if (env.elapsed_time != 0.0)
                {
                    turns = (int) (you.elapsed_time - env.elapsed_time) / 10;

                    if (turns > 0)
                    {
                        // player ghosts included here because they cannot
                        // leave the level to follow the player.
                        if (monster_descriptor(menv[i].type,
                                               MDSC_REGENERATES)
                            || menv[i].type == MONS_PLAYER_GHOST)
                        {
                            heal_monster(&menv[i], turns, false);
                        }
                        else
                            heal_monster(&menv[i], (turns / 25), false);
                    }
                }
            }
        }
    }
}

void tag_read_level_attitude(struct tagHeader &th)
{
    int i, count;

    // how many monsters?
    count = unmarshallShort(th);

    for (i = 0; i < count; i++)
    {
        menv[i].attitude = unmarshallByte(th);
        menv[i].foe = unmarshallShort(th);
    }
}

void tag_missing_level_attitude()
{
    // we don't really have to do a lot here.
    // just set foe to MHITNOT;  they'll pick up
    // a foe first time through monster() if
    // there's one around.

    // as for attitude,  a couple simple checks
    // can be used to determine friendly/neutral/
    // hostile.
    int i;
    bool isFriendly;
    unsigned int new_beh = BEH_WANDER;

    for(i=0; i<MAX_MONSTERS; i++)
    {
        // only do actual monsters
        if (menv[i].type < 0)
            continue;

        isFriendly = testbits(menv[i].flags, MF_CREATED_FRIENDLY);

        menv[i].foe = MHITNOT;

        switch(menv[i].behavior)
        {
            case 0:         // old BEH_SLEEP
                new_beh = BEH_SLEEP;    // don't wake sleepers
                break;
            case 3:         // old BEH_FLEE
            case 10:        // old BEH_FLEE_FRIEND
                new_beh = BEH_FLEE;
                break;
            case 1:         // old BEH_CHASING_I
            case 6:         // old BEH_FIGHT
                new_beh = BEH_SEEK;
                break;
            case 7:         // old BEH_ENSLAVED
                if (!mons_has_ench(&menv[i], ENCH_CHARM))
                    isFriendly = true;
                break;
            default:
                break;
        }

        menv[i].attitude = (isFriendly)?ATT_FRIENDLY : ATT_HOSTILE;
        menv[i].behavior = new_beh;
        menv[i].foe_memory = 0;
    }
}


// ------------------------------- ghost tags ---------------------------- //

static void tag_construct_ghost(struct tagHeader &th)
{
    int i;

    marshallString(th, ghost.name, 20);

    // how many ghost values?
    marshallByte(th, 20);

    for(i=0; i<20; i++)
        marshallByte(th, ghost.values[i]);
}

static void tag_read_ghost(struct tagHeader &th)
{
    char i, count_c;

    unmarshallString(th, ghost.name, 20);

    // how many ghost values?
    count_c = unmarshallByte(th);
    for(i=0; i<count_c; i++)
        ghost.values[i] = unmarshallByte(th);
}

// ----------------------------------------------------------------------- //
