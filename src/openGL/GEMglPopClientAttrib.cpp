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

#include "GEMglPopClientAttrib.h"

CPPEXTERN_NEW ( GEMglPopClientAttrib);

/////////////////////////////////////////////////////////
//
// GEMglViewport
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglPopClientAttrib :: GEMglPopClientAttrib    ()
{
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglPopClientAttrib :: ~GEMglPopClientAttrib () {}

//////////////////
// extension check
bool GEMglPopClientAttrib :: isRunnable(void)
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
void GEMglPopClientAttrib :: render(GemState *state)
{
  glPopClientAttrib ();
}


void GEMglPopClientAttrib :: obj_setupCallback(t_class *classPtr) {}
