/*-----------------------------------------------------------------
  LOG
  GEM - Graphics Environment for Multimedia

  Interface for the window manager

  Copyright (c) 1997-200 Mark Danks.
  Copyright (c) Günther Geiger. geiger@epy.co.at
  Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
  For information on usage and redistribution, and for a DISCLAIMER OF ALL
  WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

  -----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_CONTROLS_GEMWIN_H_
#define _INCLUDE__GEM_CONTROLS_GEMWIN_H_

#include "Base/CPPExtern.h"
#include <vector>

/*-----------------------------------------------------------------
  -------------------------------------------------------------------
  CLASS
  gemwin

  The window manager

  DESCRIPTION

  Access to GemMan.

  "int"   - turn on/off the rendering (in double buffered mode)
  "bang"  - swap the buffers
  "render" - render a frame now
  "title" - set a title for the graphics window
  "create" - create a graphics window
  "destroy" - destroy the graphics window
  "buffer" - single or double buffering
  "fullscreen" - fullscreen mode
  "topmost" - set the window to stay on top
  "dimen" - the window dimensions
  "offset" - the window offset
  "frame" - the frame rate
  "lighting" - turn lighting on/off
  "ambient" - the ambient light color
  "specular" - the specular light color
  "shininess" - the shininess value
  "stereo" - select stereo-mode (0=off; 1=normal)
  "stereoSep" - the stereo separation
  "stereoFoc" - the distance to the focal point
  "stereoLine" - draw a line between two stereo screens...
  "perspective" - set the perspective viewing
  "reset" - reset the graphics manager to the initial state
  "color" - rgb color for clearing
  "profile #" - turn on/off profiling (time per frame)
  - # == 0 : turn off
  - # == 1 : turn on
  - # == 2 : turn on without image caching

  "view" - set the viewpoint
  "fogmode"     - set the fog mode
  - 0 : FOG_OFF
  - 1 : FOG_LINEAR (from begin to end)
  - 2 : FOG_EXP (density) (default)
  - 3 : FOG_EXP2 (density)
  "fog" - set the fog density or begin/end of the fog
  "fogcolor" - the fog color

  -----------------------------------------------------------------*/
class GEM_EXTERN gemwin : public CPPExtern
{
  CPPEXTERN_HEADER(gemwin, CPPExtern);

public:

  //////////
  // Constructor
  gemwin(t_floatarg framespersecond);

private:

  //////////
  // Destructor
  virtual       ~gemwin();

  void          bangMess();
  void          intMess(int state);
  void                  renderMess();
  void          titleMess(t_symbol* s);
  void          createMess(t_symbol* s);
  void          stereoMess(int mode);
  void          bufferMess(int buf);
  void          dimensionsMess(int width, int height);
  void          fullscreenMess(int on);
  void                  menuBarMess(int on);
  void          secondscreenMess(int on);
  void          offsetMess(int x, int y);
  void          colorMess(float red, float green, float blue, float alpha);
  void          clearmaskMess(float bitmask);
  void          ambientMess(float red, float green, float blue, float alpha);
  void          specularMess(float red, float green, float blue,
                             float alpha);
  void          shininessMess(float val);

  void          fogModeMess(int mode);
  void          fogDensityMess(float val);
  void          fogRangeMess(float start, float end);
  void          fogColorMess(float red, float green, float blue,
                             float alpha);
  void          cursorMess(float setting);
  void          topmostMess(float setting);
  void          blurMess(float setting);
  void          fpsMess();
  void          fsaaMess(int value);
  t_outlet      *m_FrameRate;

public:
  
  void   mouseMotion(int x, int y);
  void   mouseButton(int which, int state, int x, int y);
  void   mouseWheel(int axis, int value);
  
private:
  
  void info(std::vector<t_atom>);
  void info(t_symbol*s, int, t_atom*);
  void info(const std::string&);
  void info(const std::string&, t_float);
  void info(const std::string&, int i);
  void info(const std::string&, const std::string&);
  
  //////////
  // Static member functions
  static void   titleMessCallback(void *data, t_symbol* s);
  static void   createMessCallback(void *data, t_symbol* s);
  static void   createStereoMessCallback(void *data);
  static void   colorMessCallback(void *data, t_symbol*,int,t_atom*);
  static void   ambientMessCallback(void *data, t_symbol*,int,t_atom*);
  static void   specularMessCallback(void *data, t_symbol*,int,t_atom*);
  static void   fogMessCallback(void *, t_symbol*, int argc, t_atom *argv);
  static void   fogColorMessCallback(void *, t_symbol*,int,t_atom*);

  // just call GemMan directly
  static void   destroyMessCallback(void *);
  static void   printMessCallback(void *);
  static void   profileMessCallback(void *, t_float state);
  static void   resetMessCallback(void *);
  static void   lightingMessCallback(void *, t_float state);
  static void   borderMessCallback(void *, t_float state);
  static void   frameMessCallback(void *, t_float framesPerSecond);
  static void   perspectiveMessCallback(void *, t_symbol*, int argc,
                                        t_atom *argv);
  static void   viewMessCallback(void *, t_symbol*, int argc, t_atom *argv);
  static void   stereoMessCallback(void *data, t_float state);
  static void   stereoFocMessCallback(void *, t_float state);
  static void   stereoSepMessCallback(void *, t_float state);
  static void   stereoLineMessCallback(void *, t_float state);
};

#endif  // for header file
