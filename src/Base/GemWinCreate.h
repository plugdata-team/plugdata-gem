/*-----------------------------------------------------------------

GEM - Graphics Environment for Multimedia

create a window

Copyright (c) 1997-1999 Mark Danks. mark@danks.org
Copyright (c) Günther Geiger. geiger@epy.co.at
Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_BASE_GEMWINCREATE_H_
#define _INCLUDE__GEM_BASE_GEMWINCREATE_H_
#include "GemJUCEContext.h"
#include "Gem/GemConfig.h"
#include "Gem/GemGL.h"

#include "Gem/ExportDef.h"

// I hate Microsoft...I shouldn't have to do this!
#ifdef _WIN32
# pragma warning( disable : 4244 )
# pragma warning( disable : 4305 )
# pragma warning( disable : 4091 )
#endif

#include <string.h>

/*-----------------------------------------------------------------
  -------------------------------------------------------------------
  CLASS
  WindowInfo

  All of the relevant information about an OpenGL window

  DESCRIPTION

  -----------------------------------------------------------------*/

// Create a new window
GEM_EXTERN extern int createGemWindow(WindowInfo &info,
                                      WindowHints &hints);

//////////
// Destroy a window
GEM_EXTERN extern void destroyGemWindow(WindowInfo &info);

//////////
// Set the cursor
GEM_EXTERN extern int cursorGemWindow(WindowInfo &info, int state);

GEM_EXTERN extern void gemWinResize(WindowInfo& info, int width, int height);

//////////
// Set the topmost position
GEM_EXTERN extern int topmostGemWindow(WindowInfo &info, int state);

//////////
// Update window title
GEM_EXTERN extern void titleGemWindow(WindowInfo &info, const char* title);

//////////
// swap the buffers (gets called in double-buffered mode)
GEM_EXTERN extern void gemWinSwapBuffers(WindowInfo &nfo);
/////////
// reestablish a context
GEM_EXTERN extern void gemWinMakeCurrent(WindowInfo &nfo);
GEM_EXTERN extern bool gemWinSetCurrent();
GEM_EXTERN extern void gemWinUnsetCurrent();
/////////
// init OS-specific stuff
GEM_EXTERN extern bool initGemWin(void);

/////////
// prepare a WindowInfo for context-sharing
GEM_EXTERN void initWin_sharedContext(WindowInfo &info,
                                      WindowHints &hints);

#endif  // for header file
