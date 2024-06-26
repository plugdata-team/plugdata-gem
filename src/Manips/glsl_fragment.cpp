////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Created by tigital on 11/13/2005.
// Copyright 2005 James Tittle
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

#include "glsl_fragment.h"

CPPEXTERN_NEW_WITH_ONE_ARG(glsl_fragment, t_symbol*, A_DEFSYMBOL);

/////////////////////////////////////////////////////////
//
// glsl_fragment
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

glsl_fragment :: glsl_fragment(t_symbol* filename) :
  glsl_vertex()
{
  openMess(filename);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
glsl_fragment :: ~glsl_fragment()
{
  closeMess();
}

////////////////////////////////////////////////////////
// extension check
//
/////////////////////////////////////////////////////////
bool glsl_fragment :: isRunnable()
{
  if(GLEW_VERSION_2_0) {
    m_shaderTarget = GL_FRAGMENT_SHADER;
    m_shaderType = GL2;
    return true;
  } else if (GLEW_ARB_fragment_shader) {
    m_shaderTarget = GL_FRAGMENT_SHADER_ARB;
    m_shaderType = ARB;
    return true;
  }

  pd_error(0, "need OpenGL-2.0 (or at least the fragment-shader ARB-extension) to run GLSL");
  return false;
}


/////////////////////////////////////////////////////////
// printInfo
//
/////////////////////////////////////////////////////////
void glsl_fragment :: printInfo()
{
  if(getState()==INIT) {
    verbose(0, "not initialized yet with a valid context");
    return;
  }
  if(GLEW_VERSION_2_0 || GLEW_ARB_shader_objects) {
    post("fragment shader - Hardware Info");
    post("===============================");
    if(GLEW_VERSION_2_0) {
      GLSL_GETPOSTINT( MAX_FRAGMENT_UNIFORM_COMPONENTS );
      GLSL_GETPOSTINT( MAX_TEXTURE_COORDS );
      GLSL_GETPOSTINT( MAX_TEXTURE_IMAGE_UNITS );

      if(m_shader) {
        GLint shader = m_shader;
        post("compiled last shader to ID: %d", shader);
      }
    } else {
      GLSL_GETPOSTINT( MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB );
      GLSL_GETPOSTINT( MAX_TEXTURE_COORDS_ARB );
      GLSL_GETPOSTINT( MAX_TEXTURE_IMAGE_UNITS_ARB );

      if(m_shaderARB) {
        GLhandleARB shader = m_shaderARB;

        post("compiled last shaderARB to ID: %d", shader);
      }
    }
  } else {
    post("no GLSL support");
  }
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void glsl_fragment :: obj_setupCallback(t_class *classPtr)
{
}
