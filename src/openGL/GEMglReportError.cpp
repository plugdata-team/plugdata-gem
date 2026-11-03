////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Implementation file
//
// SPDX-FileCopyrightText: © 2002, IOhannes m zmölnig and the GEM contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// this file has been generated...
////////////////////////////////////////////////////////

#include "GEMglReportError.h"
#include "Utils/GLUtil.h"

#ifdef __APPLE__
# include <OpenGL/OpenGL.h> // CGLGetCurrentContext
#endif


CPPEXTERN_NEW( GEMglReportError);
using namespace gem::utils::gl;

/////////////////////////////////////////////////////////
//
// GEMglReportError
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglReportError :: GEMglReportError    (void)
{
  m_outlet = outlet_new(this->x_obj, gensym("float"));
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglReportError :: ~GEMglReportError ()
{
  outlet_free(m_outlet);
}

/////////////////////////////////////////////////////////
// Variables
//
void GEMglReportError :: render(GemState *state)
{
  GLenum result=0, err;
#ifdef __APPLE__
  /* plugdata: a bang can arrive outside of rendering, or before there is any
   * window: without a current context, macOS' glGetError() crashes */
  if (!CGLGetCurrentContext()) {
    outlet_float(m_outlet, 0);
    return;
  }
#endif
  while ((err = glReportError(false))) {
    const char*errStr = glErrorString(err);
    error("GL[0x%04X]: %s", err, errStr?errStr:"generic error");
    if (err)
      result = err;
  }
  outlet_float(m_outlet, static_cast<t_float>(result));
}


/////////////////////////////////////////////////////////
// static member functions
//

void GEMglReportError :: obj_setupCallback(t_class *classPtr)
{
  class_addanything(classPtr, GEMglReportError::bangMessCallback);

};
void GEMglReportError :: bangMessCallback (void* data)
{
  GetMyClass(data)->render(NULL);
}
