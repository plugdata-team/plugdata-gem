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

#include "GEMglMap2d.h"

CPPEXTERN_NEW_WITH_GIMME ( GEMglMap2d );

/////////////////////////////////////////////////////////
//
// GEMglViewport
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglMap2d :: GEMglMap2d        (int argc, t_atom *argv) :
  target(0),
  u1(0.), u2(0.), ustride(0), uorder(0),
  v1(0.), v2(0.), vstride(0), vorder(0),
  points(0), len(0)
{
  if (argc>0) {
    target =static_cast<GLenum>(atom_getint(argv+0));
  }
  if (argc>1) {
    u1     =static_cast<GLdouble>(atom_getfloat(argv+1));
  }
  if (argc>2) {
    u2     =static_cast<GLdouble>(atom_getfloat(argv+2));
  }
  if (argc>3) {
    ustride=static_cast<GLint>(atom_getint(argv+3));
  }
  if (argc>4) {
    uorder =static_cast<GLint>(atom_getint(argv+4));
  }
  if (argc>5) {
    v1     =static_cast<GLdouble>(atom_getfloat(argv+5));
  }
  if (argc>6) {
    v2     =static_cast<GLdouble>(atom_getfloat(argv+6));
  }
  if (argc>7) {
    vstride=static_cast<GLint>(atom_getint(argv+7));
  }
  if (argc>8) {
    vorder =static_cast<GLint>(atom_getint(argv+8));
  }

  len=128;
  points = new GLdouble[len];

  m_inlet[0] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("target"));
  m_inlet[1] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("u1"));
  m_inlet[2] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("u2"));
  m_inlet[3] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("ustride"));
  m_inlet[4] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("uorder"));
  m_inlet[5] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("v1"));
  m_inlet[6] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("v2"));
  m_inlet[7] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("vstride"));
  m_inlet[8] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("vorder"));
  m_inlet[9] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                         gensym("points"));
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglMap2d :: ~GEMglMap2d ()
{
  inlet_free(m_inlet[0]);
  inlet_free(m_inlet[1]);
  inlet_free(m_inlet[2]);
  inlet_free(m_inlet[3]);
  inlet_free(m_inlet[4]);
  inlet_free(m_inlet[5]);
  inlet_free(m_inlet[6]);
  inlet_free(m_inlet[7]);
  inlet_free(m_inlet[8]);
  inlet_free(m_inlet[9]);

  delete[]points;
}

//////////////////
// extension check
bool GEMglMap2d :: isRunnable(void)
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
void GEMglMap2d :: render(GemState *state)
{
  glMap2d (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

/////////////////////////////////////////////////////////
// Variables
//
void GEMglMap2d :: targetMess (t_float arg1)    // FUN
{
  target = static_cast<GLenum>(arg1);
  setModified();
}

void GEMglMap2d :: u1Mess (t_float arg1)        // FUN
{
  u1 = static_cast<GLdouble>(arg1);
  setModified();
}

void GEMglMap2d :: u2Mess (t_float arg1)        // FUN
{
  u2 = static_cast<GLdouble>(arg1);
  setModified();
}

void GEMglMap2d :: ustrideMess (t_float arg1)   // FUN
{
  ustride = static_cast<GLint>(arg1);
  setModified();
}

void GEMglMap2d :: uorderMess (t_float arg1)    // FUN
{
  uorder = static_cast<GLint>(arg1);
  setModified();
}

void GEMglMap2d :: v1Mess (t_float arg1)        // FUN
{
  v1 = static_cast<GLdouble>(arg1);
  setModified();
}

void GEMglMap2d :: v2Mess (t_float arg1)        // FUN
{
  v2 = static_cast<GLdouble>(arg1);
  setModified();
}

void GEMglMap2d :: vstrideMess (t_float arg1)   // FUN
{
  vstride = static_cast<GLint>(arg1);
  setModified();
}

void GEMglMap2d :: vorderMess (t_float arg1)    // FUN
{
  vorder = static_cast<GLint>(arg1);
  setModified();
}

void GEMglMap2d :: pointsMess (int argc, t_atom*argv)   // FUN
{
  if (argc>len) {
    len=argc;
    delete [] points;
    points = new GLdouble[len];
  }
  while(argc--) {
    points[argc]=atom_getfloat(argv+argc);
  }
  setModified();
}

/////////////////////////////////////////////////////////
// static member functions
//

void GEMglMap2d :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::targetMessCallback),
                  gensym("target"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::u1MessCallback),     gensym("u1"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::u2MessCallback),     gensym("u2"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::ustrideMessCallback),
                  gensym("ustride"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::uorderMessCallback),
                  gensym("uorder"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::v1MessCallback),     gensym("v1"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::v2MessCallback),     gensym("v2"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::vstrideMessCallback),
                  gensym("vstride"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::vorderMessCallback),
                  gensym("vorder"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr,
                  reinterpret_cast<t_method>(&GEMglMap2d::pointsMessCallback),
                  gensym("points"), A_GIMME, A_NULL);
}

void GEMglMap2d :: targetMessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->targetMess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: u1MessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->u1Mess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: u2MessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->u2Mess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: ustrideMessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->ustrideMess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: uorderMessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->uorderMess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: v1MessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->v1Mess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: v2MessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->v2Mess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: vstrideMessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->vstrideMess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: vorderMessCallback (void* data, t_float arg0)
{
  GetMyClass(data)->vorderMess ( static_cast<t_float>(arg0));
}
void GEMglMap2d :: pointsMessCallback (void* data, t_symbol*, int argc,
                                       t_atom*argv)
{
  GetMyClass(data)->pointsMess (argc, argv);
}
