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

#include "State.h"
#include "Gem/GemGL.h"

/* for GemMan::StackIDs */
#include "Gem/Manager.h"
#include "Gem/GLStack.h"

#include <map>
#include <memory>

#include <iostream>

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif


#if defined _MSC_VER
/* disable deprecated warnings */
# pragma warning( disable : 4996 )
#endif

#ifdef GEM_ANY_TYPEID_HACK
# if GEM_ANY_TYPEID_HACK
#  warning GEM_ANY_TYPEID_HACK enabled
# else
#  warning GEM_ANY_TYPEID_HACK disabled
# endif
#endif

#define CATCH_ANY(y) catch(gem::bad_any_cast&x) { ::verbose(3, "%s:%d [%s] %d:: %s", __FILE__, __LINE__, __FUNCTION__, (y), x.what()); }

using namespace gem;

class GemStateData
{
  friend class GemState;
public:
  GemStateData(void) : stacks(new GLStack()) {}

  ~GemStateData(void)
  {
    if (NULL==stacks.get()) {
      post("ouch");
      //      const GLStack*dummy=new GLStack();
      //stacks=dummy;
      stacks.reset();
      post("yaroooo!");
    }
  }

  GemStateData& copyFrom(const GemStateData*org)
  {
    data=org->data;
    stacks->reset();
    return (*this);
  }

protected:
  // dictionary for setting values
  std::map <GemState::key_t, any> data;

  std::unique_ptr<GLStack>stacks;

  static std::map <std::string, int> keys;
};
std::map <std::string, int> GemStateData::keys;

/////////////////////////////////////////////////////////
//
// GemState
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
GemState :: GemState()
  : dirty(0), inDisplayList(0), lighting(0), smooth(0), texture(0),
    image(0), texCoords(0), numTexCoords(0), multiTexUnits(0),
    tickTime(50.f), drawType(0),
    VertexDirty(0),
    VertexArray(0), VertexArraySize(0), VertexArrayStride(0),
    ColorArray(0), HaveColorArray(0),
    NormalArray(0), HaveNormalArray(0),
    TexCoordArray(0), HaveTexCoordArray(0),
    data(new GemStateData())
{

  //  std::cout << "GemState" << std::endl;

  stackDepth[GemMan::STACKMODELVIEW]=
    stackDepth[GemMan::STACKCOLOR]=
      stackDepth[GemMan::STACKTEXTURE]=
        stackDepth[GemMan::STACKPROJECTION]=
          1; // 1 is the current matrix

  set(_DIRTY, (dirty=false));
  set(_GL_DISPLAYLIST, (inDisplayList=false));

  set(_GL_LIGHTING, (lighting=false));
  set(_GL_SMOOTH, (smooth=false));
  set(_GL_TEX_TYPE, (texture=0));
  //  set(_PIX, (image=0));
  set(_GL_TEX_NUMCOORDS, (numTexCoords=0));  // LATER get rid of this
  set(_GL_TEX_COORDS, (texCoords=0));  // LATER make this a std::vector
  set(_GL_TEX_UNITS, (multiTexUnits=0));
  set(_TIMING_TICK, (tickTime=50.f));
  set(_GL_DRAWTYPE, (drawType=0));

  set(_GL_STACKS, data->stacks.get());

  /*
    set("vertex.array.vertex", 0);
    set("vertex.array.color", 0);
    set("vertex.array.normal", 0);
    set("vertex.array.texcoord", 0);
  */
}

GemState& GemState::operator=(const GemState&org)
{
  dirty=org.dirty;
  inDisplayList=org.inDisplayList;
  lighting=org.lighting;
  smooth=org.smooth;
  texture=org.texture;
  image=org.image;
  texCoords=org.texCoords;
  numTexCoords=org.numTexCoords;
  multiTexUnits=org.multiTexUnits;
  tickTime=org.tickTime;
  drawType=org.drawType;
  VertexArray=org.VertexArray;
  VertexArraySize=org.VertexArraySize;
  VertexArrayStride=org.VertexArrayStride;
  ColorArray=org.ColorArray;
  HaveColorArray=org.HaveColorArray;
  NormalArray=org.NormalArray;
  HaveNormalArray=org.HaveNormalArray;
  TexCoordArray=org.TexCoordArray;
  HaveTexCoordArray=org.HaveTexCoordArray;

  data->copyFrom(org.data);
  return (*this);
}


GemState::GemState(const GemState&org) :
  dirty(org.dirty),
  inDisplayList(org.inDisplayList),
  lighting(org.lighting),
  smooth(org.smooth),
  texture(org.texture),
  image(org.image),
  texCoords(org.texCoords),
  numTexCoords(org.numTexCoords),
  multiTexUnits(org.multiTexUnits),
  tickTime(org.tickTime),
  drawType(org.drawType),
  VertexDirty(org.VertexDirty),
  VertexArray(org.VertexArray),
  VertexArraySize(org.VertexArraySize),
  VertexArrayStride(org.VertexArrayStride),
  ColorArray(org.ColorArray),
  HaveColorArray(org.HaveColorArray),
  NormalArray(org.NormalArray),
  HaveNormalArray(org.HaveNormalArray),
  TexCoordArray(org.TexCoordArray),
  HaveTexCoordArray(org.HaveTexCoordArray),
  data(new GemStateData())
{
  data->copyFrom(org.data);
}




void GemState :: reset()
{
  VertexArray = 0;
  VertexArraySize = 0;
  ColorArray = 0;
  NormalArray = 0;
  TexCoordArray = 0;
  HaveColorArray = 0;
  HaveNormalArray = 0;
  HaveTexCoordArray = 0;
  drawType = 0;

  if(GemMan::get()->windowExists()) {
    GLStack *stacks;
    get(GemState::_GL_STACKS, stacks);
    stacks->reset();
  }

  set(GemState::_PIX, (image=0));
  set(GemState::_GL_TEX_NUMCOORDS, (numTexCoords=0));

}

GemState :: ~GemState()
{
  if(data) {
    delete data;
  }
  data=NULL;
}


// --------------------------------------------------------------
/* legacy functions */
float GemState::texCoordX(int num) const
{
  if (texture && numTexCoords > num) {
    return texCoords[num].s;
  } else {
    return 0.;
  }
}

float GemState::texCoordY(int num) const
{
  if (texture && numTexCoords > num) {
    return texCoords[num].t;
  } else {
    return 0.;
  }
}


/* real properties */


/* get a named property */
bool GemState::get(const GemState::key_t key, any&value)
{
  std::map<GemState::key_t,any>::iterator it =
    data->data.find(key);

  if(it==data->data.end()) {
    /* key not stored in 'data'; fall back to legacy data */

    switch(key) {
    default:
      break;
    case(_PIX):
      value=image;
      return true;
    case(_GL_TEX_NUMCOORDS):
      value=numTexCoords;
      return true;
#if 0
    case(_DIRTY):
      value=dirty;
      return true;
    case(_GL_DISPLAYLIST):
      value=inDisplayList;
      return true;
    case(_GL_LIGHTING):
      value=lighting;
      return true;
    case(_GL_SMOOTH):
      value=smooth;
      return true;
    case(_GL_TEX_TYPE):
      value=texture;
      return true;
    case(_GL_TEX_COORDS):
      if(!texCoords) {
        return false;
      }
      value=texCoords;
      return true;
    case(_GL_TEX_UNITS):
      value=multiTexUnits;
      return true;
    case(_TIMING_TICK):
      value=tickTime;
      return true;
    case(_GL_DRAWTYPE):
      value=drawType;
      return true;
#endif
    }
    return false;
  }

  value=it->second;
  return true;
}

/* set a named property */
bool GemState::set(const GemState::key_t key, any value)
{
  if(value.empty()) {
    data->data.erase(key);
    return false;
  }

  /* wrapper for DEPRECATED access to member variables */
  if(1) {
    try {
      switch(key) {
      case(_DIRTY):
        dirty=gem::any_cast<bool>(value);
        break;
      case(_PIX):
        image=gem::any_cast<pixBlock*>(value, true);
        break;
      case(_GL_TEX_NUMCOORDS):
        numTexCoords=gem::any_cast<int>(value);
        break;
      case(_GL_TEX_COORDS):
        texCoords=gem::any_cast<TexCoord*>(value);
        break;
      case(_GL_LIGHTING):
        lighting=gem::any_cast<bool>(value);
        break;
      case(_GL_SMOOTH):
        smooth=gem::any_cast<bool>(value);
        break;
      case(_GL_TEX_TYPE):
        texture=gem::any_cast<int>(value);
        break;
      case(_GL_TEX_UNITS):
        multiTexUnits=gem::any_cast<int>(value);
        break;
      case(_TIMING_TICK):
        tickTime=gem::any_cast<float>(value);
        break;
      case(_GL_DRAWTYPE):
        drawType=gem::any_cast<GLenum>(value);
        break;
      case(_GL_DISPLAYLIST):
        inDisplayList=gem::any_cast<bool>(value);
        break;
      default:
        break;
      }
    }
    CATCH_ANY(key);
  }
  data->data[key]=value;
  return true;
}

/* remove a named property */
bool GemState::remove(const GemState::key_t key)
{
  return (0!=data->data.erase(key));
}

const GemState::key_t GemState::getKey(const std::string&s)
{
  if(GemStateData::keys.empty()) {
    GemStateData::keys["dirty"]=_DIRTY;
    GemStateData::keys["timing.tick"]=_TIMING_TICK;
    GemStateData::keys["pix"]=_PIX;
    GemStateData::keys["gl.stacks"]=_GL_STACKS;
    GemStateData::keys["gl.displaylist"]=_GL_DISPLAYLIST;
    GemStateData::keys["gl.lighting"]=_GL_LIGHTING;
    GemStateData::keys["gl.smooth"]=_GL_SMOOTH;
    GemStateData::keys["gl.drawtype"]=_GL_DRAWTYPE;
    GemStateData::keys["gl.tex.type"]=_GL_TEX_TYPE;
    GemStateData::keys["gl.tex.coords"]=_GL_TEX_COORDS;
    GemStateData::keys["gl.tex.numcoords"]=_GL_TEX_NUMCOORDS;
    GemStateData::keys["gl.tex.units"]=_GL_TEX_UNITS;
    GemStateData::keys["gl.tex.orientation"]=_GL_TEX_ORIENTATION;
    GemStateData::keys["gl.tex.basecoord"]=_GL_TEX_BASECOORD;
  }

  key_t result=_ILLEGAL;

  std::map<std::string, int>::iterator it =  GemStateData::keys.find(s);

  if(it != GemStateData::keys.end()) {
    result=(key_t)it->second;
  } else {
    /* add a new key */
    result=(key_t)GemStateData::keys.size();
    GemStateData::keys[s]=result;
  }
  return result;
}
