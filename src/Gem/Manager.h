/*-----------------------------------------------------------------
  LOG
  GEM - Graphics Environment for Multimedia

  The base functions and structures
  Also includes gemwin header file

  Copyright (c) 1997-2000 Mark Danks.mark@danks.org
  Copyright (c) Günther Geiger. geiger@epy.co.at
  Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
  For information on usage and redistribution, and for a DISCLAIMER OF ALL
  WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

  -----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_GEM_MANAGER_H_
#define _INCLUDE__GEM_GEM_MANAGER_H_

#include "Gem/GemGL.h"
#include "Utils/GLUtil.h"

#include "Gem/ExportDef.h"

#ifndef GEM_MULTICONTEXT
# include "Base/GemWinCreate.h"
#endif

#include <string>
#include <map>

#include <m_pd.h>

struct _symbol;

class gemhead;
class GemState;
class WindowInfo;

namespace gem
{
class Context;
};

/*-----------------------------------------------------------------
  -------------------------------------------------------------------
  CLASS
  GemMan

  A class to create windows, etc.

  DESCRIPTION

  -----------------------------------------------------------------*/
class GEM_EXTERN GemMan
{
public:

  //////////
  // Should only be called once (usually by GemSetup)
  void       initGem(void);

  //////////
  void       addObj(gemhead *obj, float priority);

  //////////
  void       removeObj(gemhead *obj, float priority);

  //////////
  // Is there a window.
  int        windowExists(void);

  //////////
  // Are we rendering.
  int        getRenderState(void);

  //////////
  // is there a context (has its meaning under X)
  void         createContext(const char* disp);
  int        contextExists(void);

  //////////
  // If an object needs to know if the window changed.
  // This is important for display lists.
  int        windowNumber(void);

  //////////
  // reset to the initial state
  void       resetState(void);
    
  //////////
  // Just send out one frame (if double buffered, will swap buffers)
  static void       render(void *);

  void       renderChain(struct _symbol *head, bool start);
  void       renderChain(struct _symbol *head, GemState *state);


    
  void       pauseRendering();
  static void resumeRendering(void *);
    
  //////////
  // Start a clock to do rendering.
  void       startRendering();

  //////////
  // Stop the clock to do rendering.
  void       stopRendering();

  //////////
  // Create the window with the current parameters
  int        createWindow(const char* disp = 0);

  //////////
  // Destroy the window
  void       destroyWindow(void);
  // Destroy the window after a minimal delay
  void       destroyWindowSoon(void);

  //////////
  // Swap the buffers.
  // If single buffered, just clears the window
  void       swapBuffers(void);

  //////////
  // Set the frame rate
  void       frameRate(float framespersecond);
  //////////
  // Get the frame rate
  float      getFramerate(void);

  int        getProfileLevel(void);

  void getDimen(int*width, int*height);
  void getRealDimen(int*width, int*height);
  void getOffset(int*x, int*y);
  void setDimen(int width, int height);

  //////////
  // Turn on/off lighting
  void       lightingOnOff(int state);

  //////////
  // Turn on/off cursor
  void         cursorOnOff(int state);

  //////////
  // Turn on/off topmost position
  void         topmostOnOff(int state);

  void       windowInit(void);
    
  bool       stillHaveGemWin(bool up);
    
  //////////
  // Request a lighting value - it is yours until you free it.
  // The return can be 0, in which there are too many lights
  // [in] specific - If you want a specific light.  == 0 means that you don't care.
  GLenum     requestLight(int specific = 0);

  //////////
  // Free a lighting value
  void       freeLight(GLenum lightNum);

  //////////
  // Print out information
  void       printInfo(void);

  //////////
  void       fillGemState(GemState &);

  int       texture_rectangle_supported = 0;

  enum GemStackId { STACKMODELVIEW, STACKCOLOR, STACKTEXTURE, STACKPROJECTION };
  GLint     maxStackDepth[4]; // for push/pop of matrix-stacks


  float     m_perspect[6];       // values for the perspective matrix
  float     m_lookat[9]; // values for the lookat matrix

  // LATER make this private (right now it is needed in gem2pdp)
  int       m_buffer;            // single(1) or double(2)

private:

  //////////
  // computer and window information
  std::string m_title = "Gem";             // title to be displayed
  int       m_fullscreen = 0;        // fullscreen (1) or not (0!)
  int
  m_menuBar;           // hide (0), show(1), hide but autoshow(-1)
  int       m_secondscreen = 0;      // set the second screen
  int       m_height = 500;            // window height
  int       m_width = 500;             // window width
  int
  m_w = 500;                 // the real window width (reported by gemCreateWindow())
  int       m_h = 500;                 // the real window height
  int       m_xoffset = 0;           // window offset (x)
  int       m_yoffset = 0;           // window offset (y)

  int       m_border = 1;            // window border
  int       m_stereo = 0;            // stereoscopic

  int
  m_profile = 0;           // off(0), on(1), w/o image caching(2)
  int       m_rendering = 0;

  float     m_fog;                       // fog density
  enum FOG_TYPE
  { FOG_OFF = 0, FOG_LINEAR, FOG_EXP, FOG_EXP2 };
  FOG_TYPE  m_fogMode;           // what kind of fog we have
  GLfloat   m_fogColor[4];       // colour of the fog
  float     m_fogStart;          // start of the linear fog
  float     m_fogEnd;            // start of the linear fog

  float
  m_motionBlur = 0.0f;        // motion-blur factor in double-buffer mode

  float     fps;
  int       fsaa = 0;
  bool      pleaseDestroy = false;

#ifndef GEM_MULTICONTEXT
  //////////
  // Changing these variables is likely to crash GEM
  // This is current rendering window information
  // The window is created and destroyed by the user, so
  //            if there is no window, this will contain NULL pointers.
  WindowInfo   &getWindowInfo(void);

  //////////
  // Changing these variables is likely to crash GEM
  // This is constant rendering window information
  // This window is always available (although not visible)
  WindowInfo   &getConstWindowInfo(void);
#endif /* GEM_MULTICONTEXT */
  int        createConstWindow(const char* disp = 0);

  // gemwin is allowed to modifying "global" window attributes
  friend class gemwin;
  friend class gem::Context;

  GLfloat    m_clear_color[4];   // the frame buffer clear
  GLbitfield m_clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;               // the clear bitmask
  GLfloat    m_mat_ambient[4];   // default ambient material
  GLfloat    m_mat_specular[4];  // default specular material
  GLfloat    m_mat_shininess;    // default shininess material

  GLfloat    m_stereoSep = -15.f;                // stereo separation
  GLfloat    m_stereoFocal = 0.f;              // distance to focal point
  bool
  m_stereoLine = true;               // draw a line between 2 stereo-screens

  double
  m_lastRenderTime = 0.;   // the time of the last rendered frame

  // gemwin should not touch the following member variables and member functions
  int        m_windowState = 0;
  int        m_windowNumber = 0;
  int        m_windowContext = 0;
  int        m_cursor = 1;
  int        m_topmost = 0;
    
  bool glewInitialized = false;
  
  // static data
  static constexpr int NUM_LIGHTS = 8;        // the maximum number of lights
  int m_lightState = 0;        // is lighting on or off
  int m_lights[NUM_LIGHTS];    // the lighting array
  
  t_clock *m_clock = NULL;
  double m_deltime = 50.;
  int m_hit = 0;
  int m_window_ref_count = 0;

  void       windowCleanup(void);
  void       resetValues(void);

  static void resizeCallback(int xsize, int ysize, void*);
  void dispatchWinmessCallback(void *owner);

  //////////
  // check for supported openGL extensions we might need
  void checkOpenGLExtensions(void);
    
  static inline std::map<void*, GemMan> instances = std::map<void*, GemMan>();
    
public:
  static inline GemMan* get()
  {
      bool existed = instances.count(pd_this);
      auto* instance = &instances[reinterpret_cast<void*>(pd_this)];
      if(!existed) instance->initGem();
      return instance;
  }
    
#ifndef GEM_MULTICONTEXT
   WindowInfo gfxInfo;
   WindowInfo constInfo;
#endif /* GEM_MULTICONTEXT */
    
   t_clock *m_render_start_clock = NULL;
};

#endif
