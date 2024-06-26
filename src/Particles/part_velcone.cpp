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

#include "part_velcone.h"


CPPEXTERN_NEW_WITH_FOUR_ARGS(part_velcone, t_floatarg, A_DEFFLOAT,
                             t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT);

/////////////////////////////////////////////////////////
//
// part_velcone
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
part_velcone :: part_velcone(t_floatarg xpos, t_floatarg ypos,
                             t_floatarg zpos, t_floatarg rad)
  : m_radius(rad)
{
  pd_error(0, "this is obsolete, use [part_velocity cone <x> <y> <z> <r>] instead");
  m_pos[0] = xpos;
  m_pos[1] = ypos;
  m_pos[2] = zpos;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("list"),
            gensym("vector"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"),
            gensym("vel"));
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
part_velcone :: ~part_velcone()
{ }

/////////////////////////////////////////////////////////
// renderParticles
//
/////////////////////////////////////////////////////////
void part_velcone :: renderParticles(GemState *state)
{
  if (m_tickTime > 0.f) {
    pVelocityD(PDCone, 0.f, 0.f, 0.f, m_pos[0], m_pos[1], m_pos[2], m_radius);
  }
}

/////////////////////////////////////////////////////////
// static member functions
//
/////////////////////////////////////////////////////////
void part_velcone :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG3(classPtr, "vector", vectorMess, float, float, float);
  CPPEXTERN_MSG1(classPtr, "vel", velMess, float);
}
void part_velcone :: vectorMess(float val1, float val2, float val3)
{
  m_pos[0] = val1;
  m_pos[1] = val2;
  m_pos[2] = val3;
}
void    part_velcone :: velMess(float num)
{
  m_radius = num;
}
