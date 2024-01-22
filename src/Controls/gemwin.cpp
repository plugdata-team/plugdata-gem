////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 1997-2000 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////
#ifdef __APPLE__
# ifdef __x86_64
#  define NO_AUTO_REGISTER_CLASS
# endif
#endif

#include "gemwin.h"

#include "Gem/GemGL.h"
#include "Base/GemWinCreate.h"

#ifdef __APPLE__
# include <Carbon/Carbon.h>
# include <AvailabilityMacros.h>
#endif // __APPLE__

#include "Utils/GemMath.h"
#include "Gem/Manager.h"
#include <functional>

CPPEXTERN_NEW_WITH_ONE_ARG(gemwin, t_floatarg, A_DEFFLOAT);

void GemCallOnMessageThread(std::function<void()> callback);

/////////////////////////////////////////////////////////
//
// gemwin
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
gemwin :: gemwin(t_floatarg framespersecond)
  : m_FrameRate(NULL)
{
  if(!GemMan::get()->stillHaveGemWin(true)) {
    /* this is the only [gemwin] */
    GemMan::get()->resetState();
    if (framespersecond > 0.) {
      GemMan::get()->frameRate(framespersecond);
    }
  } else {
    if(framespersecond>0.) {
      GemMan::get()->frameRate(framespersecond);
    }
  }

  m_FrameRate       = outlet_new(this->x_obj, 0);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
gemwin :: ~gemwin()
{
  if(!GemMan::get()->stillHaveGemWin(false)) {
    GemMan::get()->destroyWindow();
  }
}

/////////////////////////////////////////////////////////
// bangMess
//
/////////////////////////////////////////////////////////
void gemwin :: bangMess()
{
  if ( GemMan::get()->windowExists() ) {
    if(1==GemMan::get()->m_buffer) {
      GemMan::get()->swapBuffers();
    } else { /* double buffered mode */
      GemMan::get()->render(NULL);
    }
  } else {
    pd_error(0, "no window");
  }
}

/////////////////////////////////////////////////////////
// intMess
//
/////////////////////////////////////////////////////////
void gemwin :: intMess(int state)
{
  if (state) {
    GemMan::get()->startRendering();
  } else {
    GemMan::get()->stopRendering();
  }
}
/////////////////////////////////////////////////////////
// renderMess
//
/////////////////////////////////////////////////////////
void gemwin :: renderMess()
{
  if (GemMan::get()->getRenderState()) {
    GemMan::get()->render(NULL);
  } else {
    pd_error(0, "not in render mode");
  }
}
/////////////////////////////////////////////////////////
// titleMess
//
/////////////////////////////////////////////////////////
void gemwin :: titleMess(t_symbol* s)
{
  GemMan::get()->m_title = s->s_name;
}
/////////////////////////////////////////////////////////
// createMess
//
/////////////////////////////////////////////////////////


void gemwin :: createMess(t_symbol* s)
{
  GemCallOnMessageThread([s](){
    const char* disp = NULL;

    /* just in case a "pleaseDestroy" is still pending... */
    GemMan::get()->pleaseDestroy=false;

    if (s != &s_) {
      disp = s->s_name;
    }

    if ( !GemMan::get()->windowExists() )  {
      GemMan::get()->createContext(disp);
      if ( !GemMan::get()->createWindow(disp) )  {
        pd_error(nullptr, "no window made");
        return;
      }
      GemMan::get()->swapBuffers();
      GemMan::get()->swapBuffers();
    } else {
      pd_error(nullptr, "window already made");
    }
  });
}

/////////////////////////////////////////////////////////
// bufferMess
//
/////////////////////////////////////////////////////////
void gemwin :: bufferMess(int buf)
{
  if (buf == 1) {
    GemMan::get()->m_buffer = 1;
  } else {
    GemMan::get()->m_buffer = 2;
  }
}


/////////////////////////////////////////////////////////
// stereoMess
//
/////////////////////////////////////////////////////////
void gemwin :: stereoMess(int mode)
{
  if (mode<0) {
    pd_error(0, "stereo-mode must not be %d", mode);
    return;
  }
  if (mode>1) {
    pd_error(0, "only stereo-modes 1/0 are allowed!!!");
    return;
  }

  GemMan::get()->m_stereo = mode;
}

/////////////////////////////////////////////////////////
// fullscreenMess
//
/////////////////////////////////////////////////////////
void gemwin :: fullscreenMess(int on)
{
  GemMan::get()->m_fullscreen = on;
}

/////////////////////////////////////////////////////////
// menuBarMess          1 = show        -1 = hide, but autoshow
//                                      0 = hide, but neverShow
/////////////////////////////////////////////////////////
void gemwin :: menuBarMess(int on)
{
  GemMan::get()->m_menuBar = on;
#ifdef __APPLE__
  if (on == 0) {
    SetSystemUIMode( kUIModeAllHidden, kUIOptionDisableAppleMenu |
                     kUIOptionDisableProcessSwitch |
                     kUIOptionDisableSessionTerminate |
                     kUIOptionDisableForceQuit );
  } else if (on > 0) {
    SetSystemUIMode( kUIModeNormal, 0 );
  } else if (on < 0) {
    SetSystemUIMode( kUIModeAllHidden, kUIOptionAutoShowMenuBar );
  }
#endif
}

/////////////////////////////////////////////////////////
// secondScreen
//
/////////////////////////////////////////////////////////
void gemwin :: secondscreenMess(int on)
{
  GemMan::get()->m_secondscreen = on;
}

/////////////////////////////////////////////////////////
// dimensionsMess
//
/////////////////////////////////////////////////////////
void gemwin :: dimensionsMess(int width, int height)
{
  if (width <= 0) {
    pd_error(0, "width must be greater than 0");
    return;
  }

  if (height <= 0 ) {
    pd_error(0, "height must be greater than 0");
    return;
  }
  GemMan::get()->m_width = width;
  GemMan::get()->m_height = height;
    
  gemWinResize(GemMan::get()->getWindowInfo(), width, height);
}
/////////////////////////////////////////////////////////
// offsetMess
//
/////////////////////////////////////////////////////////
void gemwin :: offsetMess(int x, int y)
{
  GemMan::get()->m_xoffset = x;
  GemMan::get()->m_yoffset = y;
}
/////////////////////////////////////////////////////////
// colorMess
//
/////////////////////////////////////////////////////////
void gemwin :: colorMess(float red, float green, float blue, float alpha)
{
  GemMan::get()->m_clear_color[0] = red;
  GemMan::get()->m_clear_color[1] = green;
  GemMan::get()->m_clear_color[2] = blue;
  GemMan::get()->m_clear_color[3] = alpha;
  if ( GemMan::get()->windowExists() )  {
    glClearColor(red, green, blue, alpha);
  }
}
/////////////////////////////////////////////////////////
// clearmaskMess
//
/////////////////////////////////////////////////////////
void gemwin :: clearmaskMess(float bitmask)
{
  GemMan::get()->m_clear_mask = static_cast<GLbitfield>(bitmask);
}

/////////////////////////////////////////////////////////
// ambientMess
//
/////////////////////////////////////////////////////////
void gemwin :: ambientMess(float red, float green, float blue, float alpha)
{
  GemMan::get()->m_mat_ambient[0] = red;
  GemMan::get()->m_mat_ambient[1] = green;
  GemMan::get()->m_mat_ambient[2] = blue;
  GemMan::get()->m_mat_ambient[3] = blue;
}

/////////////////////////////////////////////////////////
// specularMess
//
/////////////////////////////////////////////////////////
void gemwin :: specularMess(float red, float green, float blue,
                            float alpha)
{
  GemMan::get()->m_mat_specular[0] = red;
  GemMan::get()->m_mat_specular[1] = green;
  GemMan::get()->m_mat_specular[2] = blue;
  GemMan::get()->m_mat_specular[2] = alpha;

}

/////////////////////////////////////////////////////////
// shininessMess
//
/////////////////////////////////////////////////////////
void gemwin :: shininessMess(float val)
{
  GemMan::get()->m_mat_shininess = val;
}

/////////////////////////////////////////////////////////
// fogDensityMess
//
/////////////////////////////////////////////////////////
void gemwin :: fogDensityMess(float val)
{
  if (val < 0.f) {
    val = 0.f;
  }

  GemMan::get()->m_fog = val;
}

/////////////////////////////////////////////////////////
// fogRangeMess
//
/////////////////////////////////////////////////////////
void gemwin :: fogRangeMess(float start, float end)
{
  if (start < 0.f) {
    start= 0.f;
  }

  if (end < 0.f) {
    end  = 0.f;
  }

  GemMan::get()->m_fogStart = start;
  GemMan::get()->m_fogEnd = end;
}

/////////////////////////////////////////////////////////
// fogColorMess
//
/////////////////////////////////////////////////////////
void gemwin :: fogColorMess(float red, float green, float blue,
                            float alpha)
{
  GemMan::get()->m_fogColor[0] = red;
  GemMan::get()->m_fogColor[1] = green;
  GemMan::get()->m_fogColor[2] = blue;
  GemMan::get()->m_fogColor[3] = alpha;
}

/////////////////////////////////////////////////////////
// fogModeMess
//
/////////////////////////////////////////////////////////
void gemwin :: fogModeMess(int mode)
{
  switch (mode) {
  case 0 :
    GemMan::get()->m_fogMode = GemMan::get()->FOG_OFF;
    break;

  case 1 :
    GemMan::get()->m_fogMode = GemMan::get()->FOG_LINEAR;
    break;

  case 2 :
    GemMan::get()->m_fogMode = GemMan::get()->FOG_EXP;
    break;

  case 3 :
    GemMan::get()->m_fogMode = GemMan::get()->FOG_EXP2;
    break;

  default :
    pd_error(0, "fogmode must be 0, 1, 2 or 3");
    break;
  }
}

/////////////////////////////////////////////////////////
// cursorMess
//
/////////////////////////////////////////////////////////
void gemwin :: cursorMess(float setting)
{
  GemMan::get()->cursorOnOff(static_cast<int>(setting));
}

/////////////////////////////////////////////////////////
// topmostMess
//
/////////////////////////////////////////////////////////
void gemwin :: topmostMess(float topmost)
{
  GemMan::get()->topmostOnOff(static_cast<int>(topmost));
}


/////////////////////////////////////////////////////////
// cursorMess
//
/////////////////////////////////////////////////////////
void gemwin :: blurMess(float setting)
{
  if (setting>=0.f && setting <= 1.f) {
    GemMan::get()->m_motionBlur = setting;
  }
}

/////////////////////////////////////////////////////////
// fpsMess
//
/////////////////////////////////////////////////////////
void gemwin :: fpsMess()
{
  outlet_float(m_FrameRate,GemMan::get()->fps);
}

/////////////////////////////////////////////////////////
// fsaaMess
//
/////////////////////////////////////////////////////////
void gemwin :: fsaaMess(int value)
{
  if (value == 2 || value == 4 || value == 8) {
    GemMan::get()->fsaa = value;
  } else {
    GemMan::get()->fsaa = value;
  }
}


/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void gemwin :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG0(classPtr, "bang", bangMess);
  CPPEXTERN_MSG1(classPtr, "float", intMess, int);
  CPPEXTERN_MSG0(classPtr, "render", renderMess);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::titleMessCallback),
                  gensym("title"), A_DEFSYMBOL,A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::createMessCallback),
                  gensym("create"), A_DEFSYMBOL,A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::createStereoMessCallback),
                  gensym("createStereo"), A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::createStereoMessCallback),
                  gensym("createstereo"), A_NULL);

  CPPEXTERN_MSG1(classPtr, "buffer", bufferMess, int);
  CPPEXTERN_MSG1(classPtr, "fullscreen", fullscreenMess, int);
  CPPEXTERN_MSG1(classPtr, "menubar", menuBarMess, int);
  CPPEXTERN_MSG1(classPtr, "secondscreen", secondscreenMess, int);
  CPPEXTERN_MSG1(classPtr, "topmost", topmostMess, int);
  CPPEXTERN_MSG2(classPtr, "dimen", dimensionsMess, int, int);
  CPPEXTERN_MSG2(classPtr, "offset", offsetMess, int, int);

  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::colorMessCallback),
                  gensym("color"), A_GIMME, A_NULL);
  CPPEXTERN_MSG1(classPtr, "clearmask", clearmaskMess, float);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::perspectiveMessCallback),
                  gensym("perspec"), A_GIMME, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::viewMessCallback),
                  gensym("view"), A_GIMME, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::fogMessCallback),
                  gensym("fog"), A_GIMME, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::fogColorMessCallback),
                  gensym("fogcolor"), A_GIMME, A_NULL);

  CPPEXTERN_MSG1(classPtr, "fogmode", fogModeMess, int);

  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::ambientMessCallback),
                  gensym("ambient"), A_GIMME, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::specularMessCallback),
                  gensym("specular"), A_GIMME, A_NULL);
  CPPEXTERN_MSG1(classPtr, "shininess", shininessMess, float);

  CPPEXTERN_MSG1(classPtr, "cursor", cursorMess, float);
  CPPEXTERN_MSG1(classPtr, "blur", blurMess, float);

  // just call GemMan directly
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::resetMessCallback),
                  gensym("reset"), A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::destroyMessCallback),
                  gensym("destroy"), A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::printMessCallback),
                  gensym("print"), A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::profileMessCallback),
                  gensym("profile"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::lightingMessCallback),
                  gensym("lighting"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoMessCallback),
                  gensym("stereo"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoSepMessCallback),
                  gensym("stereoSep"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoFocMessCallback),
                  gensym("stereoFoc"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoSepMessCallback),
                  gensym("stereosep"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoFocMessCallback),
                  gensym("stereofoc"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoLineMessCallback),
                  gensym("stereoLine"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::stereoLineMessCallback),
                  gensym("stereoline"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::borderMessCallback),
                  gensym("border"), A_FLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&gemwin::frameMessCallback),
                  gensym("frame"), A_FLOAT, A_NULL);

  CPPEXTERN_MSG0(classPtr, "fps", fpsMess);
  CPPEXTERN_MSG1(classPtr, "FSAA", fsaaMess, int);
}
void gemwin :: printMessCallback(void *)
{
  GemMan::get()->printInfo();
}
void gemwin :: profileMessCallback(void *, t_float state)
{
  GemMan::get()->m_profile = static_cast<int>(state);
}
void gemwin :: lightingMessCallback(void *, t_float state)
{
  GemMan::get()->lightingOnOff(static_cast<int>(state));
}
void gemwin :: fogMessCallback(void *data, t_symbol*, int argc,
                               t_atom *argv)
{
  switch (argc) {
  case (1):
    GetMyClass(data)->fogDensityMess(atom_getfloat(&argv[0]));
    break;
  case (2):
    GetMyClass(data)->fogRangeMess(atom_getfloat(&argv[0]),
                                   atom_getfloat(&argv[1]));
    break;
  default:
    GetMyClass(data)->error("fog message needs 1 or 2 arguments");
  }
}
void gemwin :: fogColorMessCallback(void *data, t_symbol*s,int argc,
                                    t_atom*argv)
{
  float red, green, blue, alpha=1.f;
  switch(argc) {
  case 4:
    alpha=atom_getfloat(argv+3);
  case 3:
    red=  atom_getfloat(argv);
    green=atom_getfloat(argv+1);
    blue= atom_getfloat(argv+2);
    GetMyClass(data)->fogColorMess(static_cast<float>(red),
                                   static_cast<float>(green), static_cast<float>(blue),
                                   static_cast<float>(alpha));
    break;
  default:
    GetMyClass(data)->error("\"fogcolor\" expects 3 or 4 values");
  }
}
void gemwin :: stereoMessCallback(void *data, t_float state)
{
  int mode = static_cast<int>(state);
  if (mode<0 || mode>3) {
    GetMyClass(data)->error("possible stereo-modes are: 0, 1, 2, 3");
    return;
  }

  GemMan::get()->m_stereo = mode;
}
void gemwin :: stereoSepMessCallback(void *, t_float state)
{
  GemMan::get()->m_stereoSep = state;
}
void gemwin :: stereoFocMessCallback(void *, t_float state)
{
  GemMan::get()->m_stereoFocal = state;
}
void gemwin :: stereoLineMessCallback(void *, t_float state)
{
  GemMan::get()->m_stereoLine = (state!=0.0);
}
void gemwin :: borderMessCallback(void *, t_float state)
{
  GemMan::get()->m_border = static_cast<int>(state);
}
void gemwin :: destroyMessCallback(void *)
{
  GemMan::get()->destroyWindow();
}
void gemwin :: resetMessCallback(void *)
{
  GemMan::get()->resetState();
}
void gemwin :: frameMessCallback(void *, t_float rate)
{
  GemMan::get()->frameRate(static_cast<float>(rate));
}
void gemwin :: perspectiveMessCallback(void *data, t_symbol*, int argc,
                                       t_atom *argv)
{
  if (argc != 6) {
    GetMyClass(data)->error("perspec message needs 6 arguments");
    return;
  }
  GemMan::get()->m_perspect[0] = atom_getfloat(&argv[0]);      // left
  GemMan::get()->m_perspect[1] = atom_getfloat(&argv[1]);      // right
  GemMan::get()->m_perspect[2] = atom_getfloat(&argv[2]);      // bottom
  GemMan::get()->m_perspect[3] = atom_getfloat(&argv[3]);      // top
  GemMan::get()->m_perspect[4] = atom_getfloat(&argv[4]);      // front
  GemMan::get()->m_perspect[5] = atom_getfloat(&argv[5]);      // back
}
void gemwin :: viewMessCallback(void *data, t_symbol*, int argc,
                                t_atom *argv)
{
  const float DEG2RAD = 0.01745329251994f;
  float azimuth = 0.f;
  float theta = 0.f;
  float distance = 1.f;

  if (GemMan::get()->m_stereoFocal > 0) {
    distance = GemMan::get()->m_stereoFocal;
  }

  switch (argc) {
  // setting all lookat values directly
  case 9 :
    GemMan::get()->m_lookat[0] = atom_getfloat(&argv[0]);    // eyex
    GemMan::get()->m_lookat[1] = atom_getfloat(&argv[1]);    // eyey
    GemMan::get()->m_lookat[2] = atom_getfloat(&argv[2]);    // eyez
    GemMan::get()->m_lookat[3] = atom_getfloat(&argv[3]);    // centerx
    GemMan::get()->m_lookat[4] = atom_getfloat(&argv[4]);    // centery
    GemMan::get()->m_lookat[5] = atom_getfloat(&argv[5]);    // centerz
    GemMan::get()->m_lookat[6] = atom_getfloat(&argv[6]);    // upx
    GemMan::get()->m_lookat[7] = atom_getfloat(&argv[7]);    // upy
    GemMan::get()->m_lookat[8] = atom_getfloat(&argv[8]);    // upz
    break;

  case 5 :
    theta     = static_cast<float>(DEG2RAD) * atom_getfloat(&argv[4]);

  case 4 :
    azimuth = static_cast<float>(DEG2RAD) * atom_getfloat(&argv[3]);

  // just have position
  case 3 : {
    const float dx =  static_cast<float>(cos(theta) * sinf(azimuth));
    const float dy =  static_cast<float>(sin(theta));
    const float dz = -static_cast<float>(cos(theta) * cosf(azimuth));

    GemMan::get()->m_lookat[0] = atom_getfloat(&argv[0]);          // eyex
    GemMan::get()->m_lookat[1] = atom_getfloat(&argv[1]);          // eyey
    GemMan::get()->m_lookat[2] = atom_getfloat(&argv[2]);          // eyez
    GemMan::get()->m_lookat[3] = GemMan::get()->m_lookat[0] + dx * distance;      // centerx
    GemMan::get()->m_lookat[4] = GemMan::get()->m_lookat[1] + dy * distance;      // centery
    GemMan::get()->m_lookat[5] = GemMan::get()->m_lookat[2] + dz * distance;      // centery
    GemMan::get()->m_lookat[6] = -dx*dy;                                   // upx
    GemMan::get()->m_lookat[7] = dx*dx+dz*dz;                              // upy
    GemMan::get()->m_lookat[8] = -dy*dz;                                   // upz
  }
  break;

  default:
    GetMyClass(data)->error("view message needs 3, 4, 5 or 9 arguments");

    // note :: LATER set the StereoView ...
  }
}
void gemwin :: titleMessCallback(void *data, t_symbol* disp)
{
  GetMyClass(data)->titleMess(disp);
}
void gemwin :: createMessCallback(void *data, t_symbol* disp)
{
  GetMyClass(data)->createMess(disp);
}
void gemwin :: createStereoMessCallback(void *data)
{
  GetMyClass(
    data)->error("'createStereo' is deprecated and does not work any more");
  GetMyClass(data)->error("use 'stereo 1' + 'create' instead");
}
void gemwin :: colorMessCallback(void *data, t_symbol*s, int argc,
                                 t_atom*argv)
{
  float red, green, blue, alpha=0.f;
  switch(argc) {
  case 4:
    alpha=atom_getfloat(argv+3);
  case 3:
    red=  atom_getfloat(argv);
    green=atom_getfloat(argv+1);
    blue= atom_getfloat(argv+2);
    GetMyClass(data)->colorMess(static_cast<float>(red),
                                static_cast<float>(green), static_cast<float>(blue),
                                static_cast<float>(alpha));
    break;
  default:
    GetMyClass(data)->error("\"color\" expects 3 or 4 values");
  }
}
void gemwin :: ambientMessCallback(void *data, t_symbol*s,int argc,
                                   t_atom*argv)
{
  float red, green, blue, alpha=1.f;
  switch(argc) {
  case 4:
    alpha=atom_getfloat(argv+3);
  case 3:
    red=  atom_getfloat(argv);
    green=atom_getfloat(argv+1);
    blue= atom_getfloat(argv+2);
    GetMyClass(data)->ambientMess(static_cast<float>(red),
                                  static_cast<float>(green), static_cast<float>(blue),
                                  static_cast<float>(alpha));
    break;
  default:
    GetMyClass(data)->error("\"ambient\" expects 3 or 4 values");
  }
}
void gemwin :: specularMessCallback(void *data, t_symbol*s,int argc,
                                    t_atom*argv)
{
  float red, green, blue, alpha=1.f;
  switch(argc) {
  case 4:
    alpha=atom_getfloat(argv+3);
  case 3:
    red=  atom_getfloat(argv);
    green=atom_getfloat(argv+1);
    blue= atom_getfloat(argv+2);
    GetMyClass(data)->specularMess(static_cast<float>(red),
                                   static_cast<float>(green), static_cast<float>(blue),
                                   static_cast<float>(alpha));
    break;
  default:
    GetMyClass(data)->error("\"specular\" expects 3 or 4 values");
  }
}
