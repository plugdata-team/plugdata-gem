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
//
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "GLUtil.h"
#include <string.h>
#include <unordered_map>
#include "Gem/RTE.h"

// I hate Microsoft...I shouldn't have to do this!
#ifdef _WIN32
/* disable warnings about unknown pragmas */
# pragma warning( disable : 4068 )
#endif


#define _GL_UNDEFINED -1

int gem::utils::gl::getGLbitfield(int argc, t_atom *argv)
{
  int accum=0;
  int mode=0;

  if (!(argc%2)) {
    argc--;
  }
  for (int n=0; n<argc; n++) {
    if (n%2) { // && or ||
      char c=*argv->a_w.w_symbol->s_name;
      switch (c) {
      case '|':
        mode = 0;
        break;
      case '&':
        mode=1;
        break;
      default:
        return _GL_UNDEFINED;
      }
      argv++;
    } else {
      int value=getGLdefine(argv++);
      if (value==_GL_UNDEFINED) {
        return _GL_UNDEFINED;
      }
      if (mode==0) {
        accum|=value;
      } else {
        accum&=value;
      }
    }
  }
  return accum;

}

int gem::utils::gl::getGLdefine(const t_atom *ap)
{
  if (!ap) {
    return _GL_UNDEFINED;
  }
  if (ap->a_type == A_SYMBOL) {
    return getGLdefine(ap->a_w.w_symbol);
  }
  if (ap->a_type == A_FLOAT) {
    return atom_getint((t_atom*)ap);
  }
  return _GL_UNDEFINED;
}

int gem::utils::gl::getGLdefine(const t_symbol* s)
{
  if (s && s->s_name) {
    return getGLdefine(s->s_name);
  } else {
    return _GL_UNDEFINED;
  }
}

static const std::unordered_map<std::string, int>& getGLmap() {
  static const std::unordered_map<std::string, int> glmap = {
#include "GLUtil_define_generated.h"
  };
  return glmap;
}

int gem::utils::gl::getGLdefine(const char *fixname)
{
  char namearray[MAXPDSTRING];
  char *c = namearray;
  const char *fc = fixname;
  int count = 0;
  while (*fc && (count < MAXPDSTRING-1)) {
    *c++ = toupper(*fc++);
    count++;
  }
  *c = 0;

  if (count < 4) return _GL_UNDEFINED;
  if (!(namearray[0]=='G' && namearray[1]=='L' && namearray[2]=='_'))
    return _GL_UNDEFINED;

  const auto& m = getGLmap();
  auto it = m.find(namearray);
  if (it != m.end()) return it->second;
  return _GL_UNDEFINED;
}
