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

#include "gemhead.h"

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

#include "Gem/Manager.h"
#include "Gem/State.h"
#include "Gem/Cache.h"
#include "Base/GemBase.h"

#include "Gem/GLStack.h"
#include "Gem/Exception.h"

#include <stdio.h>

GEM_EXTERN extern bool gemWinSetCurrent();
CPPEXTERN_NEW_WITH_GIMME(gemhead);


static std::string float2str(t_float v)
{
  std::string s;
  char buf[1000];
  snprintf(buf, 1000, "%g", v);
  s=buf;
  return s;
}


/////////////////////////////////////////////////////////
//
// gemhead
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
gemhead :: gemhead(int argc, t_atom*argv) :
  gemreceive(gensym("__gem_render")),
  m_cache(new GemCache(this)), m_renderOn(1)
{
  if(m_fltin) {
    /* get rid of left-over inlet from [gemreceive] */
    inlet_free(m_fltin);
  }
  m_fltin=NULL;

  m_contextname="";
  float priority=50.;
  switch(argc) {
  case 2:
    if(argv[0].a_type == A_FLOAT && argv[1].a_type == A_SYMBOL) {
      /* priority, context */
      priority=atom_getfloat(argv+0);
      m_contextname=atom_getsymbol(argv+1)->s_name;
    } else if(argv[1].a_type == A_FLOAT && argv[0].a_type == A_SYMBOL) {
      /* context, priority */
      priority=atom_getfloat(argv+1);
      m_contextname=atom_getsymbol(argv+0)->s_name;
    } else if(argv[1].a_type == A_FLOAT && argv[0].a_type == A_FLOAT) {
      /* priority, context(num) */
      priority=atom_getfloat(argv+0);
      m_contextname=::float2str(atom_getfloat  (argv+1));
    }
    break;
  case 1:
    if(argv[0].a_type == A_FLOAT) {
      /* priority */
      priority=atom_getfloat(argv+0);
    } else if(argv[0].a_type == A_SYMBOL) {
      /* context */
      m_contextname=atom_getsymbol(argv+0)->s_name;
    }
    break;
  case 0:
    priority=50.f;
    break;
  default:
    throw(GemException("invalid arguments: 'gemhead [<priority> [<contextname>]]'"));
  }
  m_priority=priority+1;
  setMess(priority);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
gemhead :: ~gemhead()
{
  if (m_cache) {
    stopRendering();
  }
  if(m_cache) {
    delete m_cache;
  }
  m_cache=NULL;
}

/////////////////////////////////////////////////////////
// renderGL
//
/////////////////////////////////////////////////////////
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
void gemhead :: renderGL(GemState *state)
{
  static const GLfloat a_color[]= {0.2,0.2,0.2,1};
  static const GLfloat d_color[]= {0.8,0.8,0.8,1};
  static const GLfloat e_color[]= {0.0,0.0,0.0,1};
  static const GLfloat s_color[]= {0.0,0.0,0.0,1};
  static const GLfloat shininess[]= {0.0};

  if (!m_cache || !m_renderOn) {
    return;
  }

  // set the default color and transformation matrix
  glColor4f(1.f, 1.f, 1.f, 1.f);

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  a_color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  d_color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e_color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, s_color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

  gem::GLStack*stacks=NULL;
  if(state) {
    state->reset();
    // set the state dirty flag
    state->set(GemState::_DIRTY, m_cache->dirty);
    state->VertexDirty=m_cache->vertexDirty;

    state->get(GemState::_GL_STACKS, stacks);
    if(stacks) {
      stacks->push();
    }
  }

  // are we profiling and need to send new images?
  if (GemMan::get()->getProfileLevel() >= 2) {
    m_cache->resendImage = 1;
  }

  t_atom ap[2];
  ap->a_type=A_POINTER;
  ap->a_w.w_gpointer=reinterpret_cast<t_gpointer*>(m_cache);  // the cache ?
  (ap+1)->a_type=A_POINTER;
  (ap+1)->a_w.w_gpointer=reinterpret_cast<t_gpointer*>(state);
  outlet_anything(m_outlet, gensym("gem_state"), 2, ap);

  m_cache->dirty = false;
  m_cache->vertexDirty=false;
  if(state) {
    state->get(GemState::_GL_STACKS, stacks);
  }
  if(stacks) {
    stacks->pop();
  }
}
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif


/////////////////////////////////////////////////////////
// bangMess
//
/////////////////////////////////////////////////////////
void gemhead :: bangMess()
{
  int renderon = m_renderOn;
  // make sure that the window and the cache exist
  if ( !activateContext() || !m_cache ) {
    return;
  }

  gemWinSetCurrent();

  // make a dummy GemState
  GemState tempState;
  GemMan::get()->fillGemState(tempState);

  m_renderOn = 1;
  renderGL(&tempState);
  m_renderOn = renderon;
  glFlush();
}

/////////////////////////////////////////////////////////
// renderOnOff
//
/////////////////////////////////////////////////////////
void gemhead :: renderOnOff(int state)
{
  m_renderOn = state;
}

/////////////////////////////////////////////////////////
// setPriority
//
/////////////////////////////////////////////////////////
void gemhead :: setMess(t_float priority)
{
  if (0.==priority) {
    priority=50.;
  }

  if(priority==m_priority) {
    return;
  }

  m_priority=priority;

  gemreceive::priorityMess(priority);
  setContext(m_contextname);
}

void gemhead :: setContext(const std::string&contextName)
{
  m_contextname = contextName;
  m_contextsym = gensym(m_contextname.c_str());
  std::string rcv="__gem_render"+contextName;

  if(m_priority<0.f) {
    rcv+="_osd";
  }

  gemreceive::nameMess(rcv);
}

bool gemhead :: activateContext(void) {
  if(!GemMan::get()->windowExists())
    return false;

  t_symbol*window_sym = gensym("__gem_window");
  if(window_sym->s_thing) {
    t_symbol*s = gensym("make_current");
    t_atom ap[1];
    SETSYMBOL(ap+0, s);
    typedmess(window_sym->s_thing, m_contextsym, 1, ap);
  }
  /* NOTE: we could also just trust that 'make_current' succeeds... */
  return m_contextActive;
}

void gemhead :: receive(t_symbol*s, int argc, t_atom*argv)
{
  if(!s)
    return;

  std::string sel = s->s_name;
  if (sel == "gem_state") {
    m_contextActive = true;
    if (m_renderOn) {
      if(1==argc && A_FLOAT==argv->a_type) {
        int i=(int)atom_getfloat(argv);
        switch(i) {
        case 0:
          stopRendering();
          m_contextActive = false;
          break;
        default:
          startRendering();
        }
      } else if (2==argc && A_POINTER==argv[0].a_type
                 && A_POINTER==argv[1].a_type) {
        //GemCache*cache=reinterpret_cast<GemCache*>(argv[0].a_w.w_gpointer);
        GemState*state=reinterpret_cast<GemState*>(argv[1].a_w.w_gpointer);
        renderGL(state);
      }
    }
  } else if (sel == "context_active") {
    int active = 0;
    if(argc) active = (int)atom_getfloat(argv);
    m_contextActive = !!active;
  } else {
    // not for us...
  }
}


/////////////////////////////////////////////////////////
// outputRenderOnOff
//
/////////////////////////////////////////////////////////
void gemhead :: outputRenderOnOff(int state)
{
  // continue sending out the cache message
  t_atom ap[1];
  SETFLOAT(ap, state);
  outlet_anything(m_outlet, gensym("gem_state"), 1, ap);
}

/////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////
void gemhead :: startRendering()
{
  if (m_cache) {
    m_cache->reset(this);
  } else {
    m_cache = new GemCache(this);
  }

  outputRenderOnOff(1);
}

/////////////////////////////////////////////////////////
// stopRendering
//
/////////////////////////////////////////////////////////
void gemhead :: stopRendering()
{
  outputRenderOnOff(0);
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void gemhead :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG0(classPtr, "bang", bangMess);
  CPPEXTERN_MSG1(classPtr, "float", renderOnOff, int);
  CPPEXTERN_MSG1(classPtr, "set", setMess, float);
  CPPEXTERN_MSG1(classPtr, "context", setContext, std::string);

  /* compat with [gemhead] abstraction that uses [savestate] */
  class_addmethod(classPtr, reinterpret_cast<t_method>(::nullfn), gensym("saved"), A_GIMME, A_NULL);
}
