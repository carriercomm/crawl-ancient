/*
 *  File:       AppHdr.h
 *  Summary:    Precompiled header used by Crawl.
 *  Written by: Jesse Jones
 *
 *  Abstract:   CodeWarrior and MSVC both support precompiled headers which can
 *              significantly speed up compiles. Unlike CodeWarrior MSVC imposes
 *              some annoying restrictions on precompiled headers: the precompiled
 *              header *must* be the first include in all cc files. Any includes or
 *              other statements that occur before the pch include are ignored. This
 *              is really stupid and can lead to bizarre errors, but it does mean
 *              that we shouldn't run into any problems on systems without precompiled
 *              headers.
 *
 *  Copyright © 1999 Jesse Jones.
 *
 *  Change History (most recent first):
 *
 *       <3>     6/18/99    BCR     Moved the CHARACTER_SET #define here from
 *                          linuxlib.cc.  Also wrapped the #define
 *                          MACROS to prevent it from being used by
 *                          Linux.
 *       <2>     6/17/99    BCR     Removed 'linux' check, replaced it with
 *                          'LINUX' check.  Now need to be -DLINUX
 *                          during compile.  Also moved
 *                          CHARACTER_SET #define here from
 *                          linuxlib.cc
 *       <1>     5/30/99    JDJ     Created (from config.h)
 */

#ifndef APPHDR_H
#define APPHDR_H

#if _MSC_VER >= 1100        // note that we can't just check for _MSC_VER: most compilers will wind up defining this in order to work with the SDK headers...
    #pragma message("Compiling AppHeader.h (this message should only appear once)")
#endif


// =========================================================================
//  System Defines
// =========================================================================

#ifdef SOLARIS
    // Most of the linux stuff applies, and so we want it
    #define LINUX
    #define PLAIN_TERM
    #include "linuxlib.h"
    // The ALTCHARSET may come across as DEC characters/JIS on non-ibm platforms
    #define CHARACTER_SET           0

    #define USE_CURSES
    #define EOL "\n"

    // This is used for Posix termios.
    #define USE_POSIX_TERMIOS

    // This is used for BSD tchars type ioctl, use this if you can't
    // use the Posix support above.
    // #define USE_TCHARS_IOCTL
    //
    // This uses Unix signal control to block some things, may be
    // useful in conjunction with USE_TCHARS_IOCTL, but not required
    // with USE_POSIX_TERMIOS
    //
    #define USE_UNIX_SIGNALS

    // This is for systems with no usleep... uncomment if you have it.
    #define USE_SELECT_FOR_DELAY

    // Default to non-ibmn character set
    #define USE_ASCII_CHARACTERS

    // This defines the chmod permissions for score and bones files.
    #define SHARED_FILES_CHMOD_VAL  0664

// Define plain_term for linux and similar, and dos_term for DOS and EMX.
#elif defined(LINUX)
    #define PLAIN_TERM
    #define CHARACTER_SET           0
    #define USE_ASCII_CHARACTERS

    #define USE_CURSES
    #define EOL "\n"

    #include <string>
    #include "linuxlib.h"

// To compile with EMX for OS/2 define USE_EMX macro with compiler command line
// (already defined in supplied makefile.emx)
#elif defined(USE_EMX)
    #define DOS_TERM
    #define EOL "\n"
    #define CHARACTER_SET           A_ALTCHARSET

    #include <string>
    #include "libemx.h"

#elif _MSC_VER >= 1100
    #include <string>
    #include "WinHdr.h"
    #error MSVC isn''t supported yet
    #define CHARACTER_SET           A_ALTCHARSET

// macintosh is predefined on all the common Mac compilers
#elif defined(macintosh)
    #define MAC 1
    #define PLAIN_TERM
    #define HAS_NAMESPACES  1
    #define EOL "\r"
    #define CHARACTER_SET           A_ALTCHARSET

    #include <string>
    #include "MacHdr.h"
    #include "libmac.h"

#elif defined(DOS)
    #define DOS_TERM
    #define SHORT_FILE_NAMES
    #define EOL "\n\r"
    #define CHARACTER_SET           A_ALTCHARSET

    #include <string>

#else
    #error unsupported compiler
#endif


// =========================================================================
//  Debugging Defines
// =========================================================================
#ifdef _DEBUG                                   // this is how MSVC signals a debug build
    #define DEBUG                   1
#else
//  #define DEBUG                   0           // leave this undefined for those lamers who use #ifdef
#endif

#if DEBUG
    #if __MWERKS__
        #define MSIPL_DEBUG_MODE
    #endif
#else
    #if !defined(NDEBUG)
        #define NDEBUG                          // used by <assert.h>
    #endif
#endif


// =========================================================================
//  Game Play Defines
// =========================================================================
#ifdef USE_CURSES
  #define NUMBER_OF_LINES   LINES
#elif MAC
  #define NUMBER_OF_LINES   30
#else
  #define NUMBER_OF_LINES   25
#endif

// Uncomment this line to separate the elf and dwarf races from then
// species list.
// #define SEPARATE_SELECTION_SCREENS_FOR_SUBSPECIES

// Uncomment this line to allow the player to select his draconian's colour.
// #define ALLOW_DRACONIAN_TYPE_SELECTION


// Uncomment this if you find the labyrinth to be buggy and what to
// remove it from the game.
// #define SHUT_LABYRINTH

// Define MACRO if you want to use the macro patch in macro.cc.
// *BCR* Macros aren't working in Linux right now...
#define MACROS

// Set this to the number of runes that will be required to enter Zot's
// domain.  You shouldn't set this really high unless you want to
// make players spend far too much time in Pandemonium/The Abyss.
//
// Traditional setting of this is one rune.
#define NUMBER_OF_RUNES_NEEDED    3

// Number of top scores to keep.
#define SCORE_FILE_ENTRIES      100

#ifdef SOLARIS
    // Define SAVE_DIR to the directory where saves, bones, and score file
    // will go... end it with a '\'.  Since all player files will be in the
    // same directory, the players UID will be appended when this option
    // is set.
    //
    // Setting it to nothing or not setting it will cause all game files to
    // be dumped in the current directory.
    //
    #define SAVE_DIR_PATH       "/opt/crawl/lib/"

    // This is very kludgy for now... hopefully, a new save file system
    // will make this little thing go away.  Define SAVE_PACKAGE_CMD
    // to a command to compress and bundle the save game files into a
    // single unit... the two %s will be replaced with the players
    // save file name.  Define LOAD_UNPACKAGE_CMD to undo this process
    // the %s is the same as above.
    //
    // PACKAGE_SUFFIX is used when the package file name is needed
    //
    // Comment these lines out if you want to leave the save files uncompressed.
    //
    #define SAVE_PACKAGE_CMD    "/opt/bin/zip -m -q -j -1 %s.zip %s.*"

    #define LOAD_UNPACKAGE_CMD  "/opt/bin/unzip -q -o %s.zip -d" SAVE_DIR_PATH

    #define PACKAGE_SUFFIX      ".zip"

    // This provides some rudimentary protection against people using
    // save file cheats on multi-user systems.
    #define DO_ANTICHEAT_CHECKS

#endif


// ===================================================================================
//  Misc
// ===================================================================================
#if HAS_NAMESPACES
    using namespace std;
#endif

template < class T >
inline void UNUSED(const volatile T &)
{
}                               // Note that this generates no code with CodeWarrior or MSVC (if inlining is on).


#endif  // APPHDR_H