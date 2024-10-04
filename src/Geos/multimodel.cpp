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

#include "multimodel.h"
#include "plugins/modelloader.h"
#include "Gem/State.h"
#include <algorithm> // std::min
#include <string.h>
#include <stdio.h>

namespace
{
static char mytolower(char in)
{
  if(in<='Z' && in>='A') {
    return in-('Z'-'z');
  }
  return in;
}
};


CPPEXTERN_NEW_WITH_FOUR_ARGS(multimodel, t_symbol*, A_DEFSYMBOL, t_floatarg,
                             A_DEFFLOAT, t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT);

/////////////////////////////////////////////////////////
//
// multimodel
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
multimodel :: multimodel(t_symbol* filename, t_floatarg baseModel,
                         t_floatarg topModel, t_floatarg skipRate)
  : m_loader(gem::plugins::modelloader::getInstance())
  , m_model(nullptr)
  , m_infoOut(gem::RTE::Outlet(this))
  , m_update(true)
  , m_drawType(GL_TRIANGLES)
  , m_blend(false)
  , m_linewidth(1.0)
  , m_useMaterial(false)
{
  m_drawTypes.clear();
  m_drawTypes["default"]=m_drawType;
  m_drawTypes["point"]=GL_POINTS;
  m_drawTypes["points"]=GL_POINTS;
  m_drawTypes["line"]=GL_LINES;
  m_drawTypes["lines"]=GL_LINES;
  m_drawTypes["fill"]=GL_TRIANGLES;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("mdl_num"));

  // make sure that there are some characters
  if (filename&&filename->s_name&&*filename->s_name) {
    openMess(filename->s_name, baseModel, topModel, skipRate);
  }
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
multimodel :: ~multimodel(void)
{
  close();
}

/////////////////////////////////////////////////////////
// close
//
/////////////////////////////////////////////////////////
void multimodel :: close(void)
{
  m_loaded.clear();
}


void multimodel :: applyProperties(void)
{
  if(m_loader) {
    m_loader->setProperties(m_properties);
  }
}

/////////////////////////////////////////////////////////
// materialMess
//
/////////////////////////////////////////////////////////
void multimodel :: materialMess(int material)
{
  m_useMaterial = material;
}

/////////////////////////////////////////////////////////
// textureMess
//
/////////////////////////////////////////////////////////
void multimodel :: textureMess(int state)
{
  switch(state) {
  case 0:
    m_texType = gem::modelGL::texturetype::LINEAR;
    break;
  case 1:
    m_texType = gem::modelGL::texturetype::SPHEREMAP;
    break;
  case 2:
    m_texType = gem::modelGL::texturetype::UV;
    break;
  default:
    break;
  }
}

/////////////////////////////////////////////////////////
// smoothMess
//
/////////////////////////////////////////////////////////
void multimodel :: smoothMess(t_float fsmooth)
{
  m_properties.set("smooth", fsmooth);
  applyProperties();
}

/////////////////////////////////////////////////////////
// reverseMess
//
/////////////////////////////////////////////////////////
void multimodel :: reverseMess(bool reverse)
{
  gem::any value=(double)reverse;
  m_properties.set("reverse", value);
  applyProperties();
}
/////////////////////////////////////////////////////////
// matrialMess
//
/////////////////////////////////////////////////////////
void multimodel :: rescaleMess(bool state)
{
  m_rescale = state?gem::modelGL::rescale::NORMALIZE_CENTER:gem::modelGL::rescale::ORIGINAL;
}

/////////////////////////////////////////////////////////
// groupMess
//
/////////////////////////////////////////////////////////
void multimodel :: groupMess(int state)
{
  gem::any value=state;
  m_properties.set("group", value);
  applyProperties();
}

/////////////////////////////////////////////////////////
// drawStyle
//
/////////////////////////////////////////////////////////
void multimodel :: drawMess(int type)
{
  /* raw */
  m_drawType = type;
}

void multimodel :: drawMess(std::string name)
{
  if(0==m_drawTypes.size()) {
    pd_error(0, "unable to change drawstyle");
    return;
  }

  std::transform(name.begin(), name.end(), name.begin(), mytolower);

  std::map<std::string, GLenum>::iterator it=m_drawTypes.find(name);
  if(m_drawTypes.end() == it) {
    pd_error(0, "unknown draw style '%s'... possible values are:", name.c_str());
    it=m_drawTypes.begin();
    while(m_drawTypes.end() != it) {
      pd_error(0, "\t %s", it->first.c_str());
      ++it;
    }
    return;
  }
  m_drawType=it->second;
}


/////////////////////////////////////////////////////////
// backendMess
//
/////////////////////////////////////////////////////////
void multimodel :: backendMess(t_symbol*s, int argc, t_atom*argv)
{
  m_backends.clear();
  if(argc) {
    for(int i=0; i<argc; i++) {
      if(A_SYMBOL == argv->a_type) {
        t_symbol* b=atom_getsymbol(argv+i);
        m_backends.push_back(b->s_name);
      } else {
        pd_error(0, "%s must be symbolic", s->s_name);
      }
    }
  } else {
    /* no backend requested, just enumerate them */
    if(m_loader) {
      const std::string sel = s->s_name;
      std::vector<gem::any>atoms;
      gem::any value;
      gem::Properties props;
      std::vector<std::string> backends;
      props.set("_backends", value);
      m_loader->getProperties(props);
      if(props.type("_backends")!=gem::Properties::UNSET) {
        props.get("_backends", backends);
      }
      atoms.clear();
      atoms.push_back(value=(int)(backends.size()));
      m_infoOut.send(sel+"s", atoms);
      if(!backends.empty()) {
        for(int i=0; i<backends.size(); i++) {
          atoms.clear();
          atoms.push_back(value=backends[i]);
          post("loader[%d] %s", i, backends[i].c_str());
          m_infoOut.send(sel, atoms);
        }
      } else {
        post("no model-loading backends found!");
      }
    }
  }
}

/////////////////////////////////////////////////////////
// openMess
//
/////////////////////////////////////////////////////////
void multimodel :: openMess(const std::string&filename,
                            float baseModel, float topModel, float skipRate)
{
  int skipRatei=static_cast<int>(skipRate);
  int topModeli=static_cast<int>(topModel);
  int baseModeli=static_cast<int>(baseModel);
  if (skipRatei == 0) {
    if (topModeli == 0) {
      open(filename, 0, baseModeli, 1);
    } else {
      open(filename, baseModeli, topModeli, 1);
    }
  } else {
    open(filename, baseModeli, topModeli, skipRatei);
  }
}
void multimodel :: open(const std::string&filename, int baseModel,
                        int topModel, int skipRate)
{
  if(!m_loader) {
    pd_error(0, "no model loader");
    return;
  }
  gem::Properties wantProps = m_properties;
  if(!m_backends.empty()) {
    wantProps.set("_backends", m_backends);
  }
  if (!topModel) {
    pd_error(0, "requires an int for number of models");
    return;
  }
  if (baseModel > topModel) {
    pd_error(0, "top range less than base model");
    return;
  }
  if (skipRate < 1) {
    skipRate = 1;
  }
  char preName[256];
  char postName[256];

  int i = 0;
  const char *strPtr = filename.c_str();
  while (strPtr[i] && strPtr[i] != '*') {
    preName[i] = strPtr[i];
    i++;
  }

  if (!strPtr[i]) {
    pd_error(0, "unable to find * in file name");
    return;
  }

  preName[i] = '\0';
  strncpy(postName, &(strPtr[i+1]), 255);
  postName[255]='\0';

  // need to figure out how many filenames there are to load
  int numModels = (topModel + 1 - baseModel) / skipRate;

  int realNum = baseModel;
  char bufName[MAXPDSTRING];
  canvas_makefilename(const_cast<t_canvas*>(getCanvas()), preName, bufName,
                      MAXPDSTRING);

  char newName[MAXPDSTRING];
  newName[0]=0;

  std::vector<gem::modelGL>loaded;

  for (i = 0; i < numModels; i++, realNum += skipRate) {
    snprintf(newName, MAXPDSTRING, "%s%d%s", bufName, realNum, postName);
    newName[MAXPDSTRING-1]=0;
    verbose(1, "trying to load '%s'", newName);

    if(m_loader->open(newName, wantProps)) {
      loaded.push_back(gem::modelGL(*m_loader));
    } else {
      break;
    }
  }

  if(loaded.size()!=numModels) {
    /* ouch, something went wrong! */
    pd_error(0, "failed to load model#%d of %d (%s)...resetting to original models",
          i, numModels, newName);
    return;
  }

  close();
  m_loaded = loaded;
  m_model = &m_loaded[0];


  post("loaded models: %s %s from %d to %d skipping %d",
       bufName, postName, baseModel, topModel, skipRate);

  setModified();
}

/////////////////////////////////////////////////////////
// changeModel
//
/////////////////////////////////////////////////////////
void multimodel :: changeModel(int modelNum)
{
  if (modelNum < 0 || ((unsigned int)modelNum) >= m_loaded.size()) {
    pd_error(0, "selection %d out of range: 0..%lu", modelNum, m_loaded.size()-1);
    return;
  }

  m_model = &m_loaded[modelNum];
  m_update = true;
  setModified();
}

void multimodel :: startRendering()
{
  for(auto&m: m_loaded) {
    m.update();
  }
}
/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void multimodel :: render(GemState *state)
{
  if(!m_model) return;
  if(m_update) {
    m_model->update();
    //m_update = false;
  }
  bool blend = m_blend;
  GLfloat linewidth = m_linewidth;
  bool setwidth = false;

  int texType = 0;
  state->get(GemState::_GL_TEX_TYPE, texType);
  if(texType) {
    bool rebuild = false;
    TexCoord*texCoords = 0;
    int texNum = 0;
    state->get(GemState::_GL_TEX_COORDS, texCoords);
    state->get(GemState::_GL_TEX_NUMCOORDS, texNum);
    if(texNum>1 && texCoords) {
      m_model->setTexture(texCoords[1].s, texCoords[1].t);
    }
    m_model->setTextureType(m_texType);
  }

  switch(m_drawType) {
  case GL_LINE_LOOP:
  case GL_LINE_STRIP:
  case GL_LINES:
  case GL_LINE:
    setwidth = true;
    break;
  default:
    setwidth = false;
    break;
  }

  m_model->setDrawType(m_drawType);
  m_model->useMaterial(m_useMaterial);
  m_model->setRescale(m_rescale);

  if(setwidth) {
    glLineWidth(linewidth);
  }
  if(blend) {
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_DONT_CARE);
  }

  if (m_group.empty())
    m_model->render();
  else
    m_model->render(m_group);
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void multimodel :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG4(classPtr, "open", openMess, std::string, float, float, float);
  CPPEXTERN_MSG1(classPtr, "mdl_num", changeModel, int);

  CPPEXTERN_MSG1(classPtr, "rescale", rescaleMess, bool);
  CPPEXTERN_MSG1(classPtr, "smooth", smoothMess, float);
  CPPEXTERN_MSG1(classPtr, "revert", reverseMess, bool);
  CPPEXTERN_MSG1(classPtr, "material", materialMess, int);
  CPPEXTERN_MSG1(classPtr, "texture", textureMess, int);
  CPPEXTERN_MSG1(classPtr, "group", groupMess, int);
  CPPEXTERN_MSG (classPtr, "loader", backendMess);
  CPPEXTERN_MSG (classPtr, "backend", backendMess);

  CPPEXTERN_MSG1(classPtr, "draw", drawMess, std::string);
}
