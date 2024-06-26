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
//    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "ortho.h"
#include "Gem/GemGL.h"
#include "Gem/Manager.h"
CPPEXTERN_NEW(ortho);

/////////////////////////////////////////////////////////
//
// ortho
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
ortho :: ortho()
  : m_state(1), m_compat(1)
{ }

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
ortho :: ~ortho()
{ }

/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void ortho :: render(GemState *)
{
  if (m_state) {
    GLfloat aspect = 0;
    int width=1, height=1;
    GemMan::get()->getDimen(&width, &height);

    aspect = (m_compat)?1.f:static_cast<GLfloat>(width) / static_cast<GLfloat>
             (height);
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    //glOrtho(-4.f, 4.f, -4.f, 4.f, .1f, 100.f);
    glOrtho(-4.f*aspect, 4.f*aspect, -4.f, 4.f, .1f, 100.f);
    glMatrixMode(GL_MODELVIEW);
  }
}

/////////////////////////////////////////////////////////
// postrender
//
/////////////////////////////////////////////////////////
void ortho :: postrender(GemState *)
{
  if (m_state) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
  }
}

/////////////////////////////////////////////////////////
// orthoMess
//
/////////////////////////////////////////////////////////
void ortho :: orthoMess(int state)
{
  m_state = state;
  setModified();
}

/////////////////////////////////////////////////////////
// compatMess
//
/////////////////////////////////////////////////////////
void ortho :: compatMess(int state)
{
  m_compat = state;
  setModified();
}
/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void ortho :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG1(classPtr, "float", orthoMess, int);
  CPPEXTERN_MSG1(classPtr, "compat", compatMess, int);
}
