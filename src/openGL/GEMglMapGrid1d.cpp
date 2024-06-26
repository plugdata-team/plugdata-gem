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

#include "GEMglMapGrid1d.h"

CPPEXTERN_NEW_WITH_THREE_ARGS ( GEMglMapGrid1d, t_floatarg, A_DEFFLOAT,
                                t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT);

/////////////////////////////////////////////////////////
//
// GEMglViewport
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglMapGrid1d :: GEMglMapGrid1d        (t_floatarg arg0, t_floatarg arg1,
    t_floatarg arg2) :
  un(static_cast<GLint>(arg0)),
  u1(static_cast<GLdouble>(arg1)),
  u2(static_cast<GLdouble>(arg2))
{
  m_inlet[0] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("un"));
  m_inlet[1] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("u1"));
  m_inlet[2] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("u2"));
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglMapGrid1d :: ~GEMglMapGrid1d ()
{
  inlet_free(m_inlet[0]);
  inlet_free(m_inlet[1]);
  inlet_free(m_inlet[2]);
}

//////////////////
// extension check
bool GEMglMapGrid1d :: isRunnable(void)
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
void GEMglMapGrid1d :: render(GemState *state)
{
  glMapGrid1d (un, u1, u2);
}

/////////////////////////////////////////////////////////
// Variables
//
void GEMglMapGrid1d :: unMess (t_float arg1)    // FUN
{
  un = static_cast<GLint>(arg1);
  setModified();
}

void GEMglMapGrid1d :: u1Mess (t_float arg1)    // FUN
{
  u1 = static_cast<GLdouble>(arg1);
  setModified();
}

void GEMglMapGrid1d :: u2Mess (t_float arg1)    // FUN
{
  u2 = static_cast<GLdouble>(arg1);
  setModified();
}


/////////////////////////////////////////////////////////
// static member functions
//

void GEMglMapGrid1d :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMapGrid1d::unMessCallback),
                  gensym("un"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMapGrid1d::u1MessCallback),
                  gensym("u1"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMapGrid1d::u2MessCallback),
                  gensym("u2"), A_DEFFLOAT, A_NULL);
}

void GEMglMapGrid1d :: unMessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->unMess ( static_cast<t_float>(arg0));
}
void GEMglMapGrid1d :: u1MessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->u1Mess ( static_cast<t_float>(arg0));
}
void GEMglMapGrid1d :: u2MessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->u2Mess ( static_cast<t_float>(arg0));
}
