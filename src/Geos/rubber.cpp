/*
 *  GEM - Graphics Environment for Multimedia
 *
 *  rubber.cpp
 *  gem_darwin
 *
 *  Created by Jamie Tittle on Sun Jan 19 2003.
 *  Copyright (c) 2003-2006 tigital. All rights reserved.
 *    For information on usage and redistribution, and for a DISCLAIMER OF ALL
 *    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
 *
 */

#include "rubber.h"
#include "Gem/State.h"

#define GRID_SIZE_X  32
#define GRID_SIZE_Y  32

CPPEXTERN_NEW_WITH_TWO_ARGS(rubber, t_floatarg, A_DEFFLOAT, t_floatarg,
                            A_DEFFLOAT);

/////////////////////////////////////////////////////////
//
// rubber
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
rubber :: rubber( t_floatarg gridX, t_floatarg gridY )
  : GemShape(1.0),
    ctrX(0.f), ctrY(0.f),
    m_height(0.0),
    m_inletH(0), inletcX(0), inletcY(0),
    m_speed(0), m_grab(-1),
    m_alreadyInit(0),
    m_springKS(0.3), m_drag(0.5),
    xsize(0.), ysize(0.), ysize0(0.),
    m_grid_sizeX(GRID_SIZE_X), m_grid_sizeY(GRID_SIZE_Y),
    m_mass(NULL), m_spring(NULL),
    m_spring_count(0)
{
  int gridXi=static_cast<int>(gridX);
  int gridYi=static_cast<int>(gridY);
  m_grid_sizeX = (gridXi>0)?gridXi:GRID_SIZE_X;
  m_grid_sizeY = (gridYi>0)?gridYi:GRID_SIZE_Y;

  // the height inlet
  m_inletH = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                       gensym("Ht"));
  inletcX = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                      gensym("cX"));
  inletcY = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                      gensym("cY"));

  m_drawType = GL_POLYGON;

  m_drawTypes.clear();
  m_drawTypes["default"]=GL_POLYGON;
  m_drawTypes["point"]=GL_POINTS;
  m_drawTypes["points"]=GL_POINTS;
  m_drawTypes["line"]=GL_LINE_LOOP;
  m_drawTypes["fill"]=GL_POLYGON;
}

////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
rubber :: ~rubber(void)
{
  inlet_free(m_inletH);
  inlet_free(inletcX);
  inlet_free(inletcY);
  m_alreadyInit = 0;
}

void rubber :: ctrXMess(float center)
{
  ctrX = center;
  setModified();
}
void rubber :: ctrYMess(float center)
{
  ctrY = center;
  setModified();
}

void rubber :: rubber_init(void)
{
  int k;

  if (m_mass != NULL) {
    delete[]m_mass;
  }
  m_mass=NULL;

  m_mass = new MASS[m_grid_sizeX*m_grid_sizeY];
  if (m_mass == NULL)    {
    pd_error(0, "can't allocate memory for masses.\n");
    return;
  }

  k = 0;
  for (int i = 0; i < m_grid_sizeX; i++)
    for (int j = 0; j < m_grid_sizeY; j++) {
      m_mass[k].nail = (i == 0 || j == 0 || i == m_grid_sizeX -1
                        || j == m_grid_sizeY - 1 );
      m_mass[k].x[0] = ((i/(m_grid_sizeX-1.0)) - 0.5);
      m_mass[k].x[1] = ((j/(m_grid_sizeY-1.0)) - 0.5);
      m_mass[k].x[2] = 0;

      m_mass[k].v[0] = 0.0;
      m_mass[k].v[1] = 0.0;
      m_mass[k].v[2] = 0.0;

      m_mass[k].t[0] = xsize*( i/(m_grid_sizeX - 1.0) );
      m_mass[k].t[1] = (ysize0-ysize)*( j/(m_grid_sizeY - 1.0) )+ysize;

      k++;
    }

  if (m_spring != NULL) {
    delete[]m_spring;
  }
  m_spring_count = (m_grid_sizeX - 1)*(m_grid_sizeY - 2)
                   + (m_grid_sizeY - 1)*(m_grid_sizeX - 2);

  m_spring=new SPRING[m_spring_count];
  if (m_spring == NULL)    {
    pd_error(0, "can't allocate memory for springs.\n");
    return;
  }

  k = 0;
  for (int i = 1; i < m_grid_sizeX - 1; i++)
    for (int j = 0; j < m_grid_sizeY - 1; j++) {
      int m = m_grid_sizeY*i + j;
      m_spring[k].i = m;
      m_spring[k].j = m + 1;
      m_spring[k].r = 0;
      k++;
    }

  for (int j = 1; j < m_grid_sizeY - 1; j++)
    for (int i = 0; i < m_grid_sizeX - 1; i++) {
      int m = m_grid_sizeY*i + j;
      m_spring[k].i = m;
      m_spring[k].j = m + m_grid_sizeX;
      m_spring[k].r = 0.0;
      k++;
    }
}

/////////////////////////////////////////////////////////
// renderShape
//
/////////////////////////////////////////////////////////
void rubber :: renderShape(GemState *state)
{
  int k, i, j;

  if (GemShape::m_texType && GemShape::m_texNum>=3) {

    if ((xsize  != GemShape::m_texCoords[1].s) ||
        (ysize  != GemShape::m_texCoords[1].t) ||
        (ysize0 != GemShape::m_texCoords[2].t)) {
      m_alreadyInit = 0;
    }

    if (!m_alreadyInit)  {
      xsize  = GemShape::m_texCoords[1].s;
      ysize0 = GemShape::m_texCoords[2].t;
      ysize  = GemShape::m_texCoords[1].t;

      rubber_init();
      m_alreadyInit = 1;
    }

    k = 0;
    for (i = 0; i < m_grid_sizeX - 1; i++)  {
      for (j = 0; j < m_grid_sizeY - 1; j++) {
        glBegin(GL_POLYGON);

        glTexCoord2fv( m_mass[k].t);
        glVertex3f( m_mass[k].x[0]*m_size, m_mass[k].x[1]*m_size, m_mass[k].x[2] );
        glTexCoord2fv( m_mass[k + 1].t );
        glVertex3f( m_mass[k + 1].x[0]*m_size, m_mass[k + 1].x[1]*m_size,
                    m_mass[k + 1].x[2] );
        glTexCoord2fv( m_mass[k + m_grid_sizeY + 1].t );
        glVertex3f( m_mass[k + m_grid_sizeY + 1].x[0]*m_size,
                    m_mass[k + m_grid_sizeY + 1].x[1]*m_size,
                    m_mass[k + m_grid_sizeY + 1].x[2] );
        glTexCoord2fv( m_mass[k + m_grid_sizeY].t );
        glVertex3f( m_mass[k + m_grid_sizeY].x[0]*m_size,
                    m_mass[k + m_grid_sizeY].x[1]*m_size,
                    m_mass[k + m_grid_sizeY].x[2] );
        glEnd();
        k++;
      }
      k++;
    }
    rubber_dynamics();
  } else {
    if (!m_alreadyInit) {
      rubber_init();
      m_alreadyInit = 1;
    }

    k = 0;
    for (i = 0; i < m_grid_sizeX - 1; i++)  {
      for (j = 0; j < m_grid_sizeY - 1; j++) {
        glBegin(m_drawType);
        glTexCoord2fv( m_mass[k].t );
        glVertex3f( m_mass[k].x[0]*m_size, m_mass[k].x[1]*m_size, m_mass[k].x[2] );
        glTexCoord2fv( m_mass[k + 1].t );
        glVertex3f( m_mass[k + 1].x[0]*m_size, m_mass[k + 1].x[1]*m_size,
                    m_mass[k + 1].x[2] );
        glTexCoord2fv( m_mass[k + m_grid_sizeY + 1].t );
        glVertex3f( m_mass[k + m_grid_sizeY + 1].x[0]*m_size,
                    m_mass[k + m_grid_sizeY + 1].x[1]*m_size,
                    m_mass[k + m_grid_sizeY + 1].x[2] );
        glTexCoord2fv( m_mass[k + m_grid_sizeY].t );
        glVertex3f( m_mass[k + m_grid_sizeY].x[0]*m_size,
                    m_mass[k + m_grid_sizeY].x[1]*m_size,
                    m_mass[k + m_grid_sizeY].x[2] );
        glEnd();
        k++;
      }
      k++;
    }
    rubber_dynamics();
    /*      for (k = 0; k < spring_count; k++)
            {
            glBegin(GL_LINES);
            glVertex3fv(mass[spring[k].i].x);
            glVertex3fv(mass[spring[k].j].x);
            glEnd();
            }
            }
    */
  }
}
/*
  Do the dynamics simulation for the next frame.
*/

void rubber :: rubber_dynamics(void)
{
  /* calculate all the spring forces acting on the mass points */

  for (int k = 0; k < m_spring_count; k++) {
    float d[3];
    int i = m_spring[k].i;
    int j = m_spring[k].j;

    d[0] = m_mass[i].x[0] - m_mass[j].x[0];
    d[1] = m_mass[i].x[1] - m_mass[j].x[1];
    d[2] = m_mass[i].x[2] - m_mass[j].x[2];

    float l = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    if (l != 0.0) {
      d[0] /= l;
      d[1] /= l;
      d[2] /= l;

      float a = l - m_spring[k].r;

      m_mass[i].v[0] -= d[0]*a*m_springKS;
      m_mass[i].v[1] -= d[1]*a*m_springKS;
      m_mass[i].v[2] -= d[2]*a*m_springKS;

      m_mass[j].v[0] += d[0]*a*m_springKS;
      m_mass[j].v[1] += d[1]*a*m_springKS;
      m_mass[j].v[2] += d[2]*a*m_springKS;
    }
  }

  /* update the state of the mass points */

  for (int k = 0; k < m_grid_sizeX*m_grid_sizeY; k++)
    if (!m_mass[k].nail) {
      m_mass[k].x[0] += m_mass[k].v[0];
      m_mass[k].x[1] += m_mass[k].v[1];
      m_mass[k].x[2] += m_mass[k].v[2];

      m_mass[k].v[0] *= (1.0 - m_drag);
      m_mass[k].v[1] *= (1.0 - m_drag);
      m_mass[k].v[2] *= (1.0 - m_drag);
    }

  /* if a mass point is grabbed, attach it to the mouse */

  if (m_grab != -1 && !m_mass[m_grab].nail) {
    m_mass[m_grab].x[0] = ctrX;
    m_mass[m_grab].x[1] = ctrY;
    m_mass[m_grab].x[2] = m_height;
  }
}

/*
  Return the index of the mass point that's nearest to the
  given screen coordinate.
*/

int rubber :: rubber_grab(void)
{
  float min_d=0;
  int min_i=0;

  for (int i = 0; i < m_grid_sizeX*m_grid_sizeY; i++) {
    float dx0 = m_mass[i].x[0] - ctrX;
    float dx1 = m_mass[i].x[1] - ctrY;
    float d = sqrt(dx0*dx0 + dx1*dx1);
    if (i == 0 || d < min_d) {
      min_i = i;
      min_d = d;
    }
  }

  return min_i;
}

/////////////////////////////////////////////////////////
// bangMess
//
/////////////////////////////////////////////////////////
void rubber :: rubber_bang(void)
{
  m_grab=(m_grab+1)?-1:rubber_grab();
}

/////////////////////////////////////////////////////////
// heightMess
//
/////////////////////////////////////////////////////////
void rubber :: heightMess(float height)
{
  m_height = (height-1.0)*2.0;
  setModified();
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void rubber :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG0(classPtr, "bang", rubber_bang);
  CPPEXTERN_MSG1(classPtr, "Ht", heightMess, float);
  CPPEXTERN_MSG1(classPtr, "cX", ctrXMess, float);
  CPPEXTERN_MSG1(classPtr, "cY", ctrYMess, float);


  CPPEXTERN_MSG1(classPtr, "drag", dragMess, float);
  CPPEXTERN_MSG1(classPtr, "spring", springMess, float);
}
void rubber :: dragMess(float drag)
{
  m_drag=drag;
}
void rubber :: springMess(float spring)
{
  m_springKS=spring;
}
