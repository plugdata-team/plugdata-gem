////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 1997-1999 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2014 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    Copyright (c) 2002 tigital
//
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////
#include "Gem/GemConfig.h"

#include "Gem/Manager.h"
#include "Gem/RTE.h"

#include "Gem/Settings.h"
#include "Gem/GLStack.h"
#include "Gem/State.h"
#include "Gem/Event.h"

#include <stdlib.h>
#include <string.h>

#ifdef __unix__
# include <sys/time.h>
# include <X11/Xlib.h>
#elif defined __APPLE__
# include <time.h>
#endif

#include "Utils/SIMD.h"

#include "Controls/gemhead.h"


#ifdef DEBUG
# define debug_post post
#else
# include "Utils/nop.h"
# define debug_post nop_post
#endif


using namespace gem::utils::gl;

GEM_EXTERN void gemAbortRendering()
{
  GemMan::get()->stopRendering();
}

static int s_windowDelTime = 10;


static int s_windowRun = 0;
static int s_singleContext = 0;

void GemMan :: resizeCallback(int xSize, int ySize, void *)
{
#ifndef GEM_MULTICONTEXT
  auto* gemMan = GemMan::get();
  
  //gemWinMakeCurrent(gemMan->getWindowInfo());
    
  float xDivy = (float)xSize / (float)ySize;
  if (ySize == 0) {
    xDivy = 1;
  }
  gemMan->m_h = ySize;
  gemMan->m_w = xSize;
  gemMan->m_height = ySize;
  gemMan->m_width = xSize;
    
  // don't set up the viewport on macOS, I think the JUCE window already takes care of that?
  // It works better like this either way
#ifndef __APPLE__
  glViewport(0, 0, xSize, ySize);
#endif
  // setup the matrices
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(gemMan->m_perspect[0] * xDivy,
            gemMan->m_perspect[1] * xDivy,       // left, right
            gemMan->m_perspect[2],
            gemMan->m_perspect[3],                       // bottom, top
            gemMan->m_perspect[4],
            gemMan->m_perspect[5]);                      // front, back

  glMatrixMode(GL_MODELVIEW);
  //  TODO:
  //    shouldn't this be called here?
  //  glLoadIdentity();
    
  //windowInit();
    
#endif /* GEM_MULTICONTEXT */
}

void GemMan :: checkOpenGLExtensions(void)
{
  /* rectangle textures */
  texture_rectangle_supported = 0;

  if(GLEW_ARB_texture_rectangle) {
    texture_rectangle_supported=2;
  } else if (GLEW_EXT_texture_rectangle) {
    texture_rectangle_supported=1;
  }

  t_atom*a=gem::Settings::get("texture.rectangle");
  if(a) {
    int i=atom_getint(a);
    if(0==i) {
      texture_rectangle_supported = 0;
    }
  }
}

void GemMan :: createContext(const char* disp)
{
  // can we only have one context?
  t_atom*a=gem::Settings::get("window.singlecontext"); // find a better name!
  if(a) {
    int i=atom_getint(a);
    if(1==i) {
      s_singleContext = 1;
    }
    /*
    m_width = 640;
    m_height = 480;
    */
  }

    if (!m_windowContext && !createConstWindow(disp)) {
    pd_error(nullptr, "GEM: A serious error occurred creating const Context");
    pd_error(nullptr, "GEM: Continue at your own risk!");
    m_windowContext = 0;
  } else {
    m_windowContext = 1;
  }
  setResizeCallback(GemMan::resizeCallback, NULL);
}

int GemMan :: contextExists(void)
{
  return(m_windowContext);
}

/////////////////////////////////////////////////////////
//
// GemMan
//
/////////////////////////////////////////////////////////
// initGem
//
/////////////////////////////////////////////////////////
void GemMan :: initGem()
{
    /*
  static int alreadyInit = 0;
  if (alreadyInit) {
    return;
  }
  alreadyInit = 1; */

  // clear the light array
  for (int i = 0; i < NUM_LIGHTS; i++) {
    m_lights[i] = 0;
  }

  m_clear_color[0] = 0.0;
  m_clear_color[1] = 0.0;
  m_clear_color[2] = 0.0;
  m_clear_color[3] = 0.0;
  m_mat_ambient[0] = 0.0f;
  m_mat_ambient[1] = 0.0f;
  m_mat_ambient[2] = 0.0f;
  m_mat_ambient[3] = 1.0f;
  m_mat_specular[0] = 1.0;
  m_mat_specular[1] = 1.0;
  m_mat_specular[2] = 1.0;
  m_mat_specular[3] = 1.0;
  m_mat_shininess = 100.0;

  m_clock = clock_new(NULL, reinterpret_cast<t_method>(GemMan::render));
  m_render_start_clock = clock_new(NULL, reinterpret_cast<t_method>(GemMan::resumeRendering));

  GemSIMD simd_init;

  // setup the perspective values
  m_perspect[0] = -1.f; // left
  m_perspect[1] =  1.f; // right
  m_perspect[2] = -1.f; // bottom
  m_perspect[3] =  1.f; // top
  m_perspect[4] =  1.f; // front
  m_perspect[5] = 20.f; // back

  // setup the lookat values
  m_lookat[0] = 0.f;    // eye:         x
  m_lookat[1] = 0.f;    //              y
  m_lookat[2] = 4.f;    //              z
  m_lookat[3] = 0.f;    // center :     x
  m_lookat[4] = 0.f;    //              y
  m_lookat[5] = 0.f;    //              z
  m_lookat[6] = 0.f;    // up::         x
  m_lookat[7] = 1.f;    //              y
  m_lookat[8] = 0.f;    //              z

  // setup the fog
  m_fogMode     = FOG_OFF;
  m_fogStart    = 1.f;
  m_fogEnd      = 20.f;
  m_fog         = 0.5f;
  m_fogColor[0] = m_fogColor[1] = m_fogColor[2] = m_fogColor[3] = 1.f;

  maxStackDepth[GemMan::STACKMODELVIEW] = 16; // model
  maxStackDepth[GemMan::STACKCOLOR] =
    0; // color (defaults to 0, in case GL_ARB_imaging is not supported
  maxStackDepth[GemMan::STACKTEXTURE] = 2; // texture
  maxStackDepth[GemMan::STACKPROJECTION] = 2; // projection

  m_motionBlur = 0.f;
#ifndef GEM_MULTICONTEXT
  initGemWin();
#endif /* GEM_MULTICONTEXT */
}

/////////////////////////////////////////////////////////
// resetValues
//
/////////////////////////////////////////////////////////
void GemMan :: resetValues()
{
    
    
  if (m_lightState) {
    glEnable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &m_mat_shininess);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
  } else {
    // TODO:
    //   this should be cached, & only disabled if it was enabled
    glDisable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_NORMALIZE);
    glShadeModel(GL_FLAT);
  }

  // setup the transformation matrices
  float xDivy = (float)m_w / (float)m_h;
  if (m_w == 0) {
    xDivy = 1;
  }
  if (m_h == 0) {
    xDivy = 1;
  }

  if(GLEW_ARB_imaging) {
    glMatrixMode(GL_COLOR);
    glLoadIdentity();
  }

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glFrustum(m_perspect[0] * xDivy,
            m_perspect[1] * xDivy,       // left, right
            m_perspect[2], m_perspect[3],                               // bottom, top
            m_perspect[4], m_perspect[5]);                              // front, back

  glMatrixMode(GL_MODELVIEW);
  // TODO:
  //   shouldn't this be called here?
  //  glLoadIdentity();
  gem::utils::gl::gluLookAt(m_lookat[0], m_lookat[1], m_lookat[2], m_lookat[3], m_lookat[4],
                            m_lookat[5], m_lookat[6], m_lookat[7], m_lookat[8]);

  if (m_fogMode == FOG_OFF) {
    //  TODO:
    //    this should be cached, & only disabled if it was enabled
    glDisable(GL_FOG);
  } else {
    glEnable(GL_FOG);
    switch (m_fogMode)  {
    case (FOG_LINEAR):
      glFogf(GL_FOG_MODE, GL_LINEAR);
      break;
    case (FOG_EXP):
      glFogf(GL_FOG_MODE, GL_EXP);
      break;
    case (FOG_EXP2):
      glFogf(GL_FOG_MODE, GL_EXP2);
      break;
    case (FOG_OFF):
      glDisable(GL_FOG);
      break;
    }
    glFogf(GL_FOG_DENSITY, m_fog);
    glFogf(GL_FOG_START, m_fogStart);
    glFogf(GL_FOG_END, m_fogEnd);
    glFogfv(GL_FOG_COLOR, m_fogColor);
  }
}

/////////////////////////////////////////////////////////
// fillGemState
//
/////////////////////////////////////////////////////////
void GemMan :: fillGemState(GemState &state)
{
  if (m_lightState) {
    // GemState['lighting'] = 1;
    state.set(GemState::_GL_LIGHTING, true);
    state.set(GemState::_GL_SMOOTH, true);
  }

  gem::GLStack*stacks=NULL;
  state.get(GemState::_GL_STACKS, stacks);
  if(stacks) {
    stacks->reset();
  }
}

/////////////////////////////////////////////////////////
// resetState
//
/////////////////////////////////////////////////////////
void GemMan :: resetState()
{
  debug_post("GemMan::resetState entered");

  m_clear_color[0] = 0.0;
  m_clear_color[1] = 0.0;
  m_clear_color[2] = 0.0;
  m_clear_color[3] = 0.0;

  m_clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
  m_mat_ambient[0] = 0.1f;
  m_mat_ambient[1] = 0.1f;
  m_mat_ambient[2] = 0.1f;
  m_mat_ambient[3] = 1.0f;
  m_mat_specular[0] = 1.0;
  m_mat_specular[1] = 1.0;
  m_mat_specular[2] = 1.0;
  m_mat_specular[3] = 1.0;
  m_mat_shininess = 100.0;

  m_lightState = 0;

  // window hints
  m_height = 500;
  m_width = 500;
  gem::Settings::get("window.width", m_width);
  gem::Settings::get("window.height", m_height);

  m_w=m_width;
  m_h=m_height;
  m_xoffset = 0;
  m_yoffset = 0;

  gem::Settings::get("window.x", m_xoffset);
  gem::Settings::get("window.y", m_yoffset);

  m_fullscreen = 0;
  gem::Settings::get("window.fullscreen", m_fullscreen);

  m_border = 1;
  gem::Settings::get("window.border", m_border);

  m_menuBar = 1;
  gem::Settings::get("window.menubar", m_menuBar);

  m_title = "GEM";
  gem::Settings::get("window.title", m_title);


  m_buffer = 2;

  m_stereo=0;
  m_stereoSep = -15.f;
  m_stereoFocal = 0.f;
  m_stereoLine = true;

  // setup the perspective values
  m_perspect[0] = -1.f; // left
  m_perspect[1] =  1.f; // right
  m_perspect[2] = -1.f; // bottom
  m_perspect[3] =  1.f; // top
  m_perspect[4] =  1.f; // front
  m_perspect[5] = 20.f; // back

  // setup the lookat values
  m_lookat[0] = 0.f;    // eye: x
  m_lookat[1] = 0.f;    //              y
  m_lookat[2] = 4.f;    //              z
  m_lookat[3] = 0.f;    // center :     x
  m_lookat[4] = 0.f;    //              y
  m_lookat[5] = 0.f;    //              z
  m_lookat[6] = 0.f;    // up::         x
  m_lookat[7] = 1.f;    //              y
  m_lookat[8] = 0.f;    //              z

  // setup the fog
  m_fogMode     = FOG_OFF;
  m_fogStart    = 1.f;
  m_fogEnd      = 20.f;
  m_fog         = 0.5f;
  m_fogColor[0] = m_fogColor[1] = m_fogColor[2] = m_fogColor[3] = 1.f;

  // turn on the cursor
  m_cursor = 1;
  fps = 0;
  m_topmost = 0;

  t_float rate=20.;
  gem::Settings::get("window.fps", rate);
  frameRate(rate);

}

/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void GemMan :: renderChain(t_symbol*s, bool start)
{
  if(s->s_thing) {
    t_atom ap[1];
    SETFLOAT(ap, start);
    typedmess(s->s_thing, gensym("gem_state"), 1, ap);
  }
}
void GemMan :: renderChain(t_symbol*s, GemState *state)
{
  if(s->s_thing) {
    t_atom ap[2];
    ap->a_type=A_POINTER;
    ap->a_w.w_gpointer=NULL;  // the cache ?
    (ap+1)->a_type=A_POINTER;
    (ap+1)->a_w.w_gpointer=(t_gpointer *)state;
    typedmess(s->s_thing, gensym("gem_state"), 2, ap);
  }
}

namespace
{
typedef enum {
  WHITE=0,
  RED,
  GREEN,
  BLUE,
} color_t;
static inline void setColorMask(color_t color)
{
  switch (color) {
  default: /* white */
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    break;
  case RED: /* red */
    glColorMask(GL_TRUE,GL_FALSE,GL_FALSE,GL_TRUE);
    break;
  case GREEN: /* green */
    glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);
    break;
  case BLUE: /* blue */
    glColorMask(GL_FALSE,GL_FALSE,GL_TRUE,GL_TRUE);
    break;
  }
}
};

void GemMan :: pauseRendering()
{
    if (!m_rendering) {
      return;
    }

    m_rendering = 0;
    clock_unset(m_clock);
    clock_unset(m_render_start_clock);
}

void GemMan :: resumeRendering(void *)
{
    auto* gemMan = GemMan::get();
    gemWinMakeCurrent(gemMan->getWindowInfo());
    gemMan->windowInit();
    gemMan->m_rendering = 1;
    clock_set(gemMan->m_clock, 15);
}

void GemMan :: render(void *)
{
  auto* gemMan =  GemMan::get();
  gemWinMakeCurrent( gemMan->getWindowInfo());
    
  int profiling= gemMan->m_profile;
  t_symbol*chain1=gensym("__gem_render");
  t_symbol*chain2=gensym("__gem_render_osd");
  if( gemMan->pleaseDestroy) {
      gemMan->destroyWindow();
  }
  if (!gemMan->m_windowState) {
    return;
  }

  // are we profiling?
  double starttime=sys_getrealtime();
  double stoptime=0;

    gemMan->m_hit = 0;
    gemMan->resetValues();

  GemState currentState;
  float tickTime;

  // fill in the elapsed time
  if (gemMan->m_buffer == 1) {
    tickTime = 50.f;
  } else {
    tickTime = static_cast<float>(clock_gettimesince( gemMan->m_lastRenderTime));
  }

  currentState.set(GemState::_TIMING_TICK, tickTime);

    gemMan->m_lastRenderTime = clock_getlogicaltime();

  //test to see if stereo is supported
  //XXX maybe there is a better place to do this?
  GLboolean stereoWindowTest;
  glGetBooleanv (GL_STEREO, &stereoWindowTest);
  //if we're trying to do crystal glasses stereo but don't have a stereo window
  //disable stereo and post a warning
  if(gemMan->m_stereo == 3 && !stereoWindowTest) {
    pd_error(0, "GEM: you've selected Crystal Glasses Stereo but your graphics card isn't set up for stereo, setting stereo=0");
      gemMan->m_stereo = 0;
  } else if(stereoWindowTest) {
    //if we're not doing crystal eyes stereo but our window is enabled to do stereo
    //select the back buffer for drawing
    glDrawBuffer(GL_BACK);
  }

  // if stereoscopic rendering
  switch (gemMan->m_stereo) {
  case 1: { // 2-screen stereo
    int xSize = gemMan->m_w / 2;
    int ySize = gemMan->m_h;
    float xDivy = static_cast<float>(xSize) / static_cast<float>(ySize);
    if (ySize == 0) {
      xDivy = 1;
    }

    // setup the left viewpoint
    glViewport(0, 0, xSize, ySize);
    // setup the matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(gemMan->m_perspect[0] * xDivy,
              gemMan->m_perspect[1] * xDivy,   // left, right
              gemMan->m_perspect[2],
              gemMan->m_perspect[3],                   // bottom, top
              gemMan->m_perspect[4],
              gemMan->m_perspect[5]);                  // front, back

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(gemMan->m_lookat[0] - gemMan->m_stereoSep / 100.f, gemMan->m_lookat[1], gemMan->m_lookat[2],
                              gemMan->m_lookat[3], gemMan->m_lookat[4],
                              gemMan->m_lookat[5] + gemMan->m_stereoFocal, gemMan->m_lookat[6], gemMan->m_lookat[7], gemMan->m_lookat[8]);

    // render left view
    gemMan->fillGemState(currentState);

    gemMan->renderChain(chain1, &currentState);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0 - gemMan->m_stereoSep / 100.f, 0, 4, 0, 0, 0 + gemMan->m_stereoFocal, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);

    // setup the right viewpoint
    glViewport(xSize, 0, xSize, ySize);
    // setup the matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(gemMan->m_perspect[0] * xDivy,
              gemMan->m_perspect[1] * xDivy,   // left, right
              gemMan->m_perspect[2],
              gemMan->m_perspect[3],                   // bottom, top
              gemMan->m_perspect[4],
              gemMan->m_perspect[5]);                  // front, back

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(gemMan->m_lookat[0] + gemMan->m_stereoSep / 100.f, gemMan->m_lookat[1], gemMan->m_lookat[2],
                              gemMan->m_lookat[3], gemMan->m_lookat[4],
                              gemMan->m_lookat[5] + gemMan->m_stereoFocal, gemMan->m_lookat[6], gemMan->m_lookat[7], gemMan->m_lookat[8]);

    // render right view
    gemMan->fillGemState(currentState);
    tickTime=0;
    currentState.set(GemState::_TIMING_TICK, tickTime);
    gemMan->renderChain(chain1, &currentState);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0 + gemMan->m_stereoSep / 100.f, 0, 4, 0, 0, 0 + gemMan->m_stereoFocal, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);

    if (gemMan->m_stereoLine) {
      // draw a line between the views
      glDisable(GL_LIGHTING);

      glViewport(0, 0, gemMan->m_w, gemMan->m_h);
      xDivy = static_cast<float>(gemMan->m_w) / static_cast<float>(ySize);
      // setup the matrices
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-1, 1, -1, 1, 1, 20);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gem::utils::gl::gluLookAt(0, 0, 4, 0, 0, 0, 0, 1, 0);

      glLineWidth(2.f);
      glColor3f(1.f, 1.f, 1.f);
      glBegin(GL_LINES);
      glVertex2f(0.f, -6.f);
      glVertex2f(0.f, 6.f);
      glEnd();
      glLineWidth(1.0f);
    }
  }
  break;
  case 2: { // color-stereo
    int xSize = gemMan->m_w;
    int ySize = gemMan->m_h;
    float xDivy = static_cast<float>(xSize) / static_cast<float>(ySize);
    if (ySize == 0) {
      xDivy = 1;
    }

    color_t left_color=RED;
    color_t right_color=GREEN;

    glClear(GL_COLOR_BUFFER_BIT & gemMan->m_clear_mask);
    glClear(GL_DEPTH_BUFFER_BIT & gemMan->m_clear_mask);
    glClear(GL_STENCIL_BUFFER_BIT & gemMan->m_clear_mask);
    glClear(GL_ACCUM_BUFFER_BIT & gemMan->m_clear_mask);

    // setup the left viewpoint
    setColorMask(left_color);

    // setup the matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(gemMan->m_perspect[0] * xDivy,
              gemMan->m_perspect[1] * xDivy,   // left, right
              gemMan->m_perspect[2],
              gemMan->m_perspect[3],                   // bottom, top
              gemMan->m_perspect[4],
              gemMan->m_perspect[5]);                  // front, back

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(gemMan->m_lookat[0] - gemMan->m_stereoSep / 100.f, gemMan->m_lookat[1], gemMan->m_lookat[2],
                              gemMan->m_lookat[3], gemMan->m_lookat[4],
                              gemMan->m_lookat[5] + gemMan->m_stereoFocal, gemMan->m_lookat[6], gemMan->m_lookat[7], gemMan->m_lookat[8]);

    // render left view
    gemMan->fillGemState(currentState);
    gemMan->renderChain(chain1, &currentState);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0 - gemMan->m_stereoSep / 100.f, 0, 4, 0, 0, 0 + gemMan->m_stereoFocal, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);

    // setup the right viewpoint
    glClear(GL_DEPTH_BUFFER_BIT & gemMan->m_clear_mask);
    setColorMask(right_color);

    // setup the matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(gemMan->m_perspect[0] * xDivy,
              gemMan->m_perspect[1] * xDivy,   // left, right
              gemMan->m_perspect[2],
              gemMan->m_perspect[3],                   // bottom, top
              gemMan->m_perspect[4],
              gemMan->m_perspect[5]);                  // front, back

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(gemMan->m_lookat[0] + gemMan->m_stereoSep / 100.f, gemMan->m_lookat[1], gemMan->m_lookat[2],
                              gemMan->m_lookat[3], gemMan->m_lookat[4],
                              gemMan->m_lookat[5] + gemMan->m_stereoFocal, gemMan->m_lookat[6], gemMan->m_lookat[7], gemMan->m_lookat[8]);

    // render right view
    gemMan->fillGemState(currentState);
    tickTime=0;
    currentState.set(GemState::_TIMING_TICK, tickTime);
    gemMan->renderChain(chain1, &currentState);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0 + gemMan->m_stereoSep / 100.f, 0, 4, 0, 0, 0 + gemMan->m_stereoFocal, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);

    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  }
  break;
  case 3: { // Crystal Eyes Stereo
    int xSize = gemMan->m_w;
    int ySize = gemMan->m_h;
    float xDivy = static_cast<float>(xSize) / static_cast<float>(ySize);
    if (ySize == 0) {
      xDivy = 1;
    }

    // setup the left viewpoint

    // setup the matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(gemMan->m_perspect[0] * xDivy,
              gemMan->m_perspect[1] * xDivy, // left, right
              gemMan->m_perspect[2], gemMan->m_perspect[3],     // bottom, top
              gemMan->m_perspect[4], gemMan->m_perspect[5]);      // front, back

    glMatrixMode(GL_MODELVIEW);
    glDrawBuffer(GL_BACK_LEFT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gem::utils::gl::gluLookAt(gemMan->m_lookat[0] - gemMan->m_stereoSep / 100.f, gemMan->m_lookat[1], gemMan->m_lookat[2],
                              gemMan->m_lookat[3], gemMan->m_lookat[4],
                              gemMan->m_lookat[5] + gemMan->m_stereoFocal, gemMan->m_lookat[6], gemMan->m_lookat[7], gemMan->m_lookat[8]);

    // render left view
    gemMan->fillGemState(currentState);
    gemMan->renderChain(chain1, &currentState);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0 - gemMan->m_stereoSep / 100.f, 0, 4, 0, 0, 0 + gemMan->m_stereoFocal, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);

    // setup the right viewpoint
    glClear(GL_DEPTH_BUFFER_BIT & gemMan->m_clear_mask);

    // setup the matrices
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(gemMan->m_perspect[0] * xDivy,
              gemMan->m_perspect[1] * xDivy, // left, right
              gemMan->m_perspect[2], gemMan->m_perspect[3],     // bottom, top
              gemMan->m_perspect[4], gemMan->m_perspect[5]);      // front, back

    glMatrixMode(GL_MODELVIEW);
    glDrawBuffer(GL_BACK_RIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gem::utils::gl::gluLookAt(gemMan->m_lookat[0] + gemMan->m_stereoSep / 100.f, gemMan->m_lookat[1], gemMan->m_lookat[2],
                              gemMan->m_lookat[3], gemMan->m_lookat[4],
                              gemMan->m_lookat[5] + gemMan->m_stereoFocal, gemMan->m_lookat[6], gemMan->m_lookat[7], gemMan->m_lookat[8]);

    // render right view
    gemMan->fillGemState(currentState);
    tickTime=0;
    currentState.set(GemState::_TIMING_TICK, tickTime);
    gemMan->renderChain(chain1, &currentState);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0 + gemMan->m_stereoSep / 100.f, 0, 4, 0, 0, 0 + gemMan->m_stereoFocal, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);

    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  }
  break;
  default: { // normal rendering
    gemMan->fillGemState(currentState);
    gemMan->renderChain(chain1, &currentState);

    // setup the matrices
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gem::utils::gl::gluLookAt(0, 0, 4, 0, 0, 0, 0, 1, 0);
    gemMan->renderChain(chain2, &currentState);
  }
  }
  gemMan->swapBuffers();

  // are we profiling?
  stoptime=sys_getrealtime();
  if (profiling>0) {
    double seconds =  stoptime-starttime;
    if(seconds>0.f) {
        gemMan->fps = (1 / (seconds * 1000.f)) * 1000.f;
    } else {
      pd_error(0, "GEM: unable to profile");
    }
  }

  // only keep going if no one set the m_hit (could be hit if scheduler gets
  //        ahold of a stopRendering command)
  double deltime= gemMan->m_deltime;
  if(profiling<0) {
    float spent=(stoptime-starttime)*1000;
    if(profiling<-1) {
      deltime-=spent;
    } else if(spent<deltime && spent>0.f) {
      deltime-=spent;
    } else {
      post("unable to annihiliate %f ms", spent);
    }
    if(deltime<0.) {
      logpost(0, 3+1, "negative delay time: %f", deltime);
      deltime=1.f;
    }
  }

  if (!gemMan->m_hit && (0.0 != deltime)) {
    clock_delay(gemMan->m_clock, deltime);
  }

  glReportError();
}

/////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////
void GemMan :: startRendering()
{
  if (!m_windowState) {
    pd_error(0, "GEM: Create window first!");
    return;
  }

  if (m_rendering) {
    return;
  }

  post("GEM: Start rendering");

  // set up all of the gemheads
  renderChain(gensym("__gem_render"), true);
  renderChain(gensym("__gem_render_osd"), true);

  m_rendering = 1;

  // if only single buffering then just return
  if (m_buffer == 1) {
    return;
  }

  m_lastRenderTime = clock_getlogicaltime();
  render(NULL);
}

/////////////////////////////////////////////////////////
// stopRendering
//
/////////////////////////////////////////////////////////
void GemMan :: stopRendering()
{
  if (!m_rendering) {
    return;
  }

  m_rendering = 0;
  clock_unset(m_clock);
  clock_unset(m_render_start_clock);
  m_hit = 1;

  // clean out all of the gemheads
  renderChain(gensym("__gem_render"), false);
  renderChain(gensym("__gem_render_osd"), false);

  post("GEM: Stop rendering");
}


int GemMan :: getRenderState(void)
{
  return(m_rendering);
}


/////////////////////////////////////////////////////////
// windowInit
//
/////////////////////////////////////////////////////////
void GemMan :: windowInit()
{
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glClearDepth(1.0);
  glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2],
               m_clear_color[3]);
    
  /* i am not really sure whether it is a good idea to enable FSAA by default
   * this might slow down everything a lot;
   * JMZ: additionally, we should not enable it, without checking first,
   * whether GL_NV_multisample_filter_hint is supported by the backend
   */
  if(GLEW_ARB_multisample) {
    glEnable (GL_MULTISAMPLE_ARB);
  }
  if(GLEW_NV_multisample_filter_hint) {
    glHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
  }

  resetValues();
}

/////////////////////////////////////////////////////////
// windowCleanup
//
/////////////////////////////////////////////////////////
void GemMan :: windowCleanup()
{ }

int GemMan :: windowExists(void)
{
  return(m_windowState);
}

/////////////////////////////////////////////////////////
// createWindow
//
/////////////////////////////////////////////////////////
int GemMan :: createWindow(const char* disp)
{
#ifndef GEM_MULTICONTEXT
  if ( m_windowState ) {
    return(s_singleContext);
  }
  debug_post("GemMan: create window");

  WindowHints myHints;
  myHints.border = m_border;
  myHints.buffer = m_buffer;
  myHints.width = m_width;
  myHints.height = m_height;
  myHints.fullscreen = m_fullscreen;
  myHints.secondscreen = m_secondscreen;
  myHints.x_offset = m_xoffset;
  myHints.y_offset = m_yoffset;
  myHints.actuallyDisplay = 1;
  myHints.display = disp;
  myHints.title = const_cast<char*>(m_title.c_str());
  myHints.fsaa = fsaa;

  if (disp) {
    post("GEM: creating gem-window on display %s",disp);
  }
  if (!createGemWindow(gfxInfo, myHints) ) {
    pd_error(0, "GEM: Unable to create window");
    return(0);
  }
  /*
    Check for the presence of a couple of useful OpenGL extensions
    we can use to speed up the movie rendering.

    GL_EXT_texture_rectangle allows for non-power-of-two sized
    textures.  Texture coordinates for these textures run from
    0..width, 0..height instead of 0..1, 0..1 as for normal
    power-of-two textures.  GL_EXT_texture_rectangle is available
    on the NVidia GeForce2MX and above, or the ATI Radeon and above.
  */

  
  glewInitialized=false;
  GLenum err = glewInit();

  if (GLEW_OK != err) {
    if(GLEW_ERROR_GLX_VERSION_11_ONLY == err) {
      pd_error(0, "GEM: failed to init GLEW (glx): continuing anyhow - please report any problems to the gem-dev mailinglist!");
    } else if (GLEW_ERROR_GL_VERSION_10_ONLY) {
      pd_error(0, "GEM: failed to init GLEW: your system only supports openGL-1.0");
      return(0);
    } else {
      pd_error(0, "GEM: failed to init GLEW");
      return(0);
    }
  }
  glewInitialized=true;
  post("GEM: GLEW version %s",glewGetString(GLEW_VERSION));

  checkOpenGLExtensions();

  /* check the stack-sizes */
  glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, maxStackDepth+STACKMODELVIEW);
  if(GLEW_ARB_imaging) {
    glGetIntegerv(GL_MAX_COLOR_MATRIX_STACK_DEPTH, maxStackDepth+STACKCOLOR);
  }
  glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, maxStackDepth+STACKTEXTURE);
  glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH,
                maxStackDepth+STACKPROJECTION);

  m_w=myHints.real_w;
  m_h=myHints.real_h;

  m_windowState = 1;
  cursorOnOff(m_cursor);
  topmostOnOff(m_topmost);
  m_windowNumber++;
  
  // For plugdata, we do this later so that it happens on the correct thread
  //windowInit();
    
  s_windowRun = 1;
#endif /* GEM_MULTICONTEXT */
  return(1);
}

/////////////////////////////////////////////////////////
// destroyWindow
//
/////////////////////////////////////////////////////////
void GemMan :: destroyWindowSoon()
{
  GemMan::pleaseDestroy=true;
  /* jump to the render() to destroy the window asap */
  clock_delay(m_clock, 0.0);
}
void GemMan :: destroyWindow()
{
#ifndef GEM_MULTICONTEXT
  GemMan::pleaseDestroy=false;

  // don't want to get rid of this
  //  if (s_singleContext) return;

  // nothing to destroy...
  if (!m_windowState) {
    return;
  }

  stopRendering();

  glFlush();
  glFinish();

  destroyGemWindow(gfxInfo);

  m_windowState = 0;

  windowCleanup();

  // this should really go into the GemWinCreate<OS> files::

  // reestablish the const glxContext
  /* this crashes on linux with intel cards */
  //gemWinMakeCurrent(constInfo);
    
  s_windowRun = 0;
#endif /* GEM_MULTICONTEXT */
}

bool GemMan :: stillHaveGemWin(bool up)
{
  if (up) {
    m_window_ref_count++;
    if (m_window_ref_count==1) {
      return false;
    }
  } else {
      m_window_ref_count--;
    if (m_window_ref_count<0) {
        m_window_ref_count=0;
    }
    return (m_window_ref_count!=0);
  }
  return true;
}

int GemMan :: windowNumber(void)
{
  return(m_windowNumber);
}


/////////////////////////////////////////////////////////
// createConstWindow
//
/////////////////////////////////////////////////////////
int GemMan::createConstWindow(const char* disp)
{
#ifndef GEM_MULTICONTEXT
  // can we only have one context?
  if (s_singleContext) {
    return(GemMan::createWindow(disp));
  }

  WindowHints myHints;
  myHints.title = const_cast<char*>(m_title.c_str());
  myHints.border = 1;
  myHints.buffer = 1;
  myHints.x_offset = 0;
  myHints.y_offset = 0;
  myHints.width = m_width;
  myHints.height = m_height;

  initWin_sharedContext(constInfo, myHints);

  myHints.actuallyDisplay = 0;
  myHints.fullscreen = 0;
  myHints.display = disp;
  myHints.fsaa = GemMan::fsaa;

  if (!createGemWindow(constInfo, myHints) ) {
    pd_error(0, "GEM: Error creating const context");
    constInfo.have_constContext=0;
    gfxInfo.have_constContext=0;
    return(0);
  } else {
    constInfo.have_constContext=1;
    gfxInfo.have_constContext=1;
  }
#endif /* GEM_MULTICONTEXT */
  return(1);
}

/////////////////////////////////////////////////////////
// destroyConstWindow
//
/////////////////////////////////////////////////////////
void destroyConstWindow()
{
#ifndef GEM_MULTICONTEXT
  if (!s_singleContext) {
    destroyGemWindow(GemMan::get()->constInfo);
  }
#endif /* GEM_MULTICONTEXT */
}

/////////////////////////////////////////////////////////
// swapBuffers
//
/////////////////////////////////////////////////////////
void GemMan :: swapBuffers()
{
#ifndef GEM_MULTICONTEXT
  if (!m_windowState) {
    return;
  }
  if (m_buffer == 2) {
    gemWinSwapBuffers(gfxInfo);
  } else {
    glFlush();
  }

  //  TODO:
  //  why is this called here?
  //  seems like it'd ruin single buffer rendering...
  glClear(m_clear_mask);
  // why is this called here?
  // glColor3f(1.0, 1.0, 1.0);
  // why is this called here?
  //  not clear what glMatrixMode() we're loading...probably GL_MODELVIEW?
  glLoadIdentity();

  if (m_buffer == 1) {
    glFlush();
    // setup the transformation matrices
    float xDivy = static_cast<float>(m_w) / static_cast<float>(m_h);
    if (m_h == 0) {
      xDivy = 1;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(m_perspect[0] * xDivy, m_perspect[1] * xDivy,   // left, right
              m_perspect[2], m_perspect[3],                   // bottom, top
              m_perspect[4], m_perspect[5]);                  // front, back

    glMatrixMode(GL_MODELVIEW);
    // TODO:
    // shouldn't this be called here?
    //          glLoadIdentity();
    gem::utils::gl::gluLookAt(m_lookat[0], m_lookat[1], m_lookat[2], m_lookat[3], m_lookat[4],
                              m_lookat[5], m_lookat[6], m_lookat[7], m_lookat[8]);
  }
#endif /* GEM_MULTICONTEXT */
}

/////////////////////////////////////////////////////////
// lightingOnOff
//
/////////////////////////////////////////////////////////
void GemMan :: lightingOnOff(int state)
{
  if (state) {
    m_lightState = 1;
  } else {
    m_lightState = 0;
  }
}

/////////////////////////////////////////////////////////
// cursorOnOff
//
/////////////////////////////////////////////////////////
void GemMan :: cursorOnOff(int state)
{
#ifndef GEM_MULTICONTEXT
  if (m_windowState) {
    cursorGemWindow(gfxInfo,state);
  }
  m_cursor = state;
#endif /* GEM_MULTICONTEXT */
}

/////////////////////////////////////////////////////////
// topmostOnOff
//
/////////////////////////////////////////////////////////
void GemMan :: topmostOnOff(int state)
{
#ifndef GEM_MULTICONTEXT
  if (m_windowState) {
    topmostGemWindow(gfxInfo,state);
  }
  m_topmost = state;
#endif /* GEM_MULTICONTEXT */
}

/////////////////////////////////////////////////////////
// frameRate
//
/////////////////////////////////////////////////////////
void GemMan :: frameRate(float framespersecond)
{
  /* the new framerate will only take effect at the next render-cycle
   * so if we are currently at at frame-rate=0,
   * we have to reschedule rendering (else we would wait too long)
   */
  bool reschedule=(m_deltime<=0.f);

  if (framespersecond == 0.) {
    m_deltime = 0.;
    return;
  }
  if (framespersecond < 0.) {
    pd_error(0, "GEM: Invalid frame rate: %f", framespersecond);
    framespersecond = 20;
  }
  m_deltime = 1000. / framespersecond;

  if(reschedule) {
    render(NULL);
  }
}

/////////////////////////////////////////////////////////
// get Framerate
//
/////////////////////////////////////////////////////////
float GemMan :: getFramerate()
{
  return (m_deltime != 0.0) ? (1000. / m_deltime) : 0.0;
}


/////////////////////////////////////////////////////////
// get window dimensions
//
/////////////////////////////////////////////////////////
void GemMan :: getDimen(int*width, int*height)
{
  if(NULL!=width ) {
    *width =m_width;
  }
  if(NULL!=height) {
    *height=m_height;
  }
}
void GemMan :: setDimen(int width, int height)
{
  m_width = width;
  m_height = height;
}

/////////////////////////////////////////////////////////
// get real window dimensions (as reported by the window creator)
//
/////////////////////////////////////////////////////////
void GemMan :: getRealDimen(int*width, int*height)
{
  if(NULL!=width ) {
    *width =m_w;
  }
  if(NULL!=height) {
    *height=m_h;
  }
}


/////////////////////////////////////////////////////////
// get window position
//
/////////////////////////////////////////////////////////
void GemMan :: getOffset(int*x, int*y)
{
  if(NULL!=x) {
    *x=m_xoffset;
  }
  if(NULL!=y) {
    *y=m_yoffset;
  }
}



//
int GemMan :: getProfileLevel()
{
  return m_profile;
}



/////////////////////////////////////////////////////////
// requestLight
//
/////////////////////////////////////////////////////////
GLenum GemMan :: requestLight(int specific)
{
  int i = 0;
  if (specific > 0) {
    i = specific - 1;
  } else {
    while(m_lights[i]) {
      i++;
      if (i >= NUM_LIGHTS) {
        pd_error(0, "GEM: Unable to allocate light");
        return(static_cast<GLenum>(0));
      }
    }
  }
  m_lights[i]++;
  GLenum retLight;
  switch(i) {
  case (0) :
    retLight = GL_LIGHT0;
    break;
  case (1) :
    retLight = GL_LIGHT1;
    break;
  case (2) :
    retLight = GL_LIGHT2;
    break;
  case (3) :
    retLight = GL_LIGHT3;
    break;
  case (4) :
    retLight = GL_LIGHT4;
    break;
  case (5) :
    retLight = GL_LIGHT5;
    break;
  case (6) :
    retLight = GL_LIGHT6;
    break;
  case (7) :
    retLight = GL_LIGHT7;
    break;
  default :
    pd_error(0, "GEM: Unable to allocate world_light");
    return(static_cast<GLenum>(0));
  }
  return(retLight);
}

/////////////////////////////////////////////////////////
// freeLight
//
/////////////////////////////////////////////////////////
void GemMan :: freeLight(GLenum lightNum)
{
  /* LATER use maxlights and calculate i dynamically */
  int i = 0;
  switch(lightNum) {
  case(GL_LIGHT0):
    i = 0;
    break;
  case(GL_LIGHT1):
    i = 1;
    break;
  case(GL_LIGHT2):
    i = 2;
    break;
  case(GL_LIGHT3):
    i = 3;
    break;
  case(GL_LIGHT4):
    i = 4;
    break;
  case(GL_LIGHT5):
    i = 5;
    break;
  case(GL_LIGHT6):
    i = 6;
    break;
  case(GL_LIGHT7):
    i = 7;
    break;
  default:
    pd_error(0, "GEM: Error freeing a light - bad number");
    return;
  }
  m_lights[i]--;
  if (m_lights[i] < 0) {
    pd_error(0, "GEM: light ref count below zero: %d", i);
    m_lights[i] = 0;
  }
}

/////////////////////////////////////////////////////////
// printInfo
//
/////////////////////////////////////////////////////////
void GemMan :: printInfo()
{
  post("GEM information");
  post("---------------");
  if(!glewInitialized) {
    post("OpenGL has not been initialized yet!");
    post("create a window first");
    return;
  }



  post("OpenGL info");
  post("Vendor: %s", glGetString(GL_VENDOR));
  post("Renderer: %s", glGetString(GL_RENDERER));
  post("Version: %s", glGetString(GL_VERSION));

  if (glGetString(GL_EXTENSIONS)) {
    char *text = new char [strlen((char *)glGetString(GL_EXTENSIONS)) + 1];
    strcpy(text,(char *)glGetString(GL_EXTENSIONS));
    char *token = strtok(text,
                         " ");    // Parse 'text' For Words, Separated By " " (spaces)
    while(token != NULL) {              // While The Token Isn't NULL
      post("Extensions: %s", token);    // Print extension string
      token = strtok(NULL, " ");
    }
    delete [] text;
  }

  post("---------------");
  post("window state: %d", m_windowState);
  post("topmost: %d", m_topmost);
  post("profile: %d", m_profile);
  post("buffer: %d", m_buffer);
  post("stereo: %d", m_stereo);
  post("full screen: %d", m_fullscreen);
  post("width: %d, height %d", m_width, m_height);
  post("offset: %d+%d", m_xoffset, m_yoffset);
  post("frame rate: %f", (0.0 != m_deltime) ? 1000. / m_deltime : 0.0);

  GLint bitnum = 0;
  glGetIntegerv(GL_RED_BITS, &bitnum);
  post("red: %d", bitnum);
  glGetIntegerv(GL_GREEN_BITS, &bitnum);
  post("green: %d", bitnum);
  glGetIntegerv(GL_BLUE_BITS, &bitnum);
  post("blue: %d", bitnum);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &bitnum);
  post("max texture: %d", bitnum);

  post("lighting %d", m_lightState);
  for (int i = 0; i < NUM_LIGHTS; i++) {
    if (m_lights[i]) {
      post("light%d: on", i);
    }
  }

  post("rectangle texturing: %d", texture_rectangle_supported);
  post("direct yuv texturing: %d", GLEW_APPLE_ycbcr_422);

  post("");

  post("gem::Settings");
  post("-----------");
  gem::Settings::print();
}


#ifndef GEM_MULTICONTEXT
/////////////////////////////////////////////////////////
// getWindowInfo
//
/////////////////////////////////////////////////////////
WindowInfo &GemMan :: getWindowInfo()
{
  return(gfxInfo);
}

/////////////////////////////////////////////////////////
// getConstWindowInfo
//
/////////////////////////////////////////////////////////
WindowInfo &GemMan :: getConstWindowInfo()
{
  return(constInfo);
}

void initGemWindow()
{
    GemMan::get()->windowInit();
}

void gemBeginExternalResize()
{
    GemMan::get()->pauseRendering();
}

void gemEndExternalResize()
{
    // Resume rendering on audio thread
    clock_delay(GemMan::get()->m_render_start_clock, 10);
}

#endif /* GEM_MULTICONTEXT */
