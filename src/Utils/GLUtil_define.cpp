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

int gem::utils::gl::getGLdefine(const t_symbol *s)
{
  if (s && s->s_name) {
    return getGLdefine(s->s_name);
  } else {
    return _GL_UNDEFINED;
  }
}

int gem::utils::gl::getGLdefine(const char *fixname)
{
  char namearray[MAXPDSTRING];
  char*name=namearray;
  char *c=name;
  const char *fc=fixname;
  int count=0;

  while (*fc&&(count<MAXPDSTRING-1)) {
    *c=toupper(*fc);
    c++;
    fc++;
    count++;
  }
  *c=0;

  if (count<4) {
    return _GL_UNDEFINED;
  }
  if (!(name[0]=='G' && name[1]=='L' && name[2]=='_')) {
    return _GL_UNDEFINED;
  }

  /*
   * the following include is generated by something as awful as:
   * we are using slow stringcompares, as Pd's symbol-table is rather small (~1024 entries)
   * and we don't want to pollute it with about 3500 symbols which are likely to never be used...
   */

  /*

   grep define glew.h                              \
        | grep "GL_" | awk '{print $2}'            \
        | egrep "^GL_" | awk '{print length($1),$1}'  \
        | sort -n                                     \
        | sed -e 's|\([0-9]*\) \(.*\)$|\1 \2\n\1|'    \
        | sed -e :a -e '/^\([0-9]*\)$/N; s/\n/ /; ta'                 \
        | awk '{if (3==NF){if ($1!=$2) print $2,$3; else print $3}else if (2==NF)print $1,$2}' \
        | awk '{if (2==NF){print "        return _GL_UNDEFINED;\n    case "$1":"; $1=$2}; print "        if (!strcmp(name, \""$1"\")) return "$1";"}'
    */

  switch (count) {
  default:
#include "GLUtil_define_generated.h"
    return _GL_UNDEFINED;
  }
  return _GL_UNDEFINED;
}
