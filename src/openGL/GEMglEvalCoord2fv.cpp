////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Implementation file
//
// Copyright (c) 2002-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//      zmoelnig@iem.at
//  For information on usage and redistribution, and for a DISCLAIMER
//  *  OF ALL WARRANTIES, see the file, "GEM.LICENSE.TERMS"
//
//  this file has been generated...
////////////////////////////////////////////////////////

#include "GEMglEvalCoord2fv.h"

CPPEXTERN_NEW_WITH_TWO_ARGS ( GEMglEvalCoord2fv, t_floatarg, A_DEFFLOAT,
                              t_floatarg, A_DEFFLOAT);

/////////////////////////////////////////////////////////
//
// GEMglEvalCoord2fv
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglEvalCoord2fv :: GEMglEvalCoord2fv  (t_floatarg arg0, t_floatarg arg1)
{
  vMess(arg0, arg1);
  m_inlet = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                      gensym("v"));
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglEvalCoord2fv :: ~GEMglEvalCoord2fv ()
{
  inlet_free(m_inlet);
}
//////////////////
// extension check
bool GEMglEvalCoord2fv :: isRunnable(void)
{
  if(GLEW_VERSION_1_1) {
    return true;
  }
  pd_error(0, "your system does not support OpenGL-1.1");
  return false;
}
/////////////////////////////////////////////////////////
// Render
//
void GEMglEvalCoord2fv :: render(GemState *state)
{
  glEvalCoord2fv (v);
}

/////////////////////////////////////////////////////////
// variable
//
void GEMglEvalCoord2fv :: vMess (t_float arg0, t_float arg1)    // FUN
{
  v[0]=static_cast<GLfloat>(arg0);
  v[1]=static_cast<GLfloat>(arg1);
  setModified();
}

/////////////////////////////////////////////////////////
// static member functions
//

void GEMglEvalCoord2fv :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglEvalCoord2fv::vMessCallback),
                  gensym("v"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
}

void GEMglEvalCoord2fv :: vMessCallback (void* data, t_float arg0,
    t_float arg1)
{
  GetMyClass(data)->vMess ( arg0, arg1);
}
