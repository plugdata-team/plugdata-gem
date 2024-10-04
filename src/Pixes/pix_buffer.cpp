///////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 2002-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

// we want our pd-class "pix_buffer_class" to be defined non-static
// so other pix_buffer_...-objects can bind to it
#define NO_STATIC_CLASS

#include "pix_buffer.h"
#include "Gem/ImageIO.h"

#include <string.h>
#include <stdio.h>
#include "Gem/Files.h"

#include "plugins/imagesaver.h"
#include "plugins/imageloader.h"
#include "RTE/Outlet.h"

/* utilities */
struct pix_buffer :: PIMPL
{
  CPPExtern*parent;
  gem::RTE::Outlet outlet;

  gem::plugins::imagesaver*saver;
  gem::plugins::imageloader*loader;

  PIMPL(CPPExtern*_parent)
    : parent(_parent)
    , outlet(_parent)
    , saver(gem::plugins::imagesaver::getInstance())
    , loader(gem::plugins::imageloader::getInstance())
  {};
  ~PIMPL(void) {
    if(saver) {
      delete saver;
    }
    saver=NULL;
    if(loader) {
      delete loader;
    }
    loader=NULL;
  };

  std::vector<std::string> savebackends;
  std::vector<std::string> loadbackends;

  static gem::any atom2any(t_atom*ap)
  {
    gem::any result;
    if(ap) {
      switch(ap->a_type) {
      case A_FLOAT:
        result=atom_getfloat(ap);
        break;
      case A_SYMBOL:
        result=std::string(atom_getsymbol(ap)->s_name);
        break;
      default:
        result=ap->a_w.w_gpointer;
      }
    }
    return result;
  }
  void addProperties(gem::Properties&props, int argc, t_atom*argv)
  {
    if(!argc) {
      return;
    }

    if(argv->a_type != A_SYMBOL) {
      pd_error(parent, "no key given...");
      return;
    }
    std::string key=std::string(atom_getsymbol(argv)->s_name);
    std::vector<gem::any> values;
    argc--;
    argv++;
    while(argc-->0) {
      values.push_back(atom2any(argv++));
    }
    switch(values.size()) {
    default:
      props.set(key, values);
      break;
    case 1:
      props.set(key, values[0]);
      break;
    case 0: {
      gem::any dummy;
      props.set(key, dummy);
    }
      break;
    }
  }
};

/////////////////////////////////////////////////////////
//
// pix_buffer
//
/////////////////////////////////////////////////////////

CPPEXTERN_NEW_WITH_TWO_ARGS(pix_buffer, t_symbol*,A_DEFSYMBOL,t_float,
                            A_DEFFLOAT);

/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_buffer :: pix_buffer(t_symbol* s,t_float f=100.0)
  : m_buffer(NULL)
  , m_numframes(0)
  , m_bindname(NULL)
  , m_pimpl(new PIMPL(this))
{
  if (s==&s_) {
    static int buffercounter=0;
    char cbuf[16];
    buffercounter++;
    snprintf(cbuf, 16, "pix_buffer_%04d", buffercounter);
    cbuf[15]=0;
    post("defaulting to name '%s'", cbuf);
    s=gensym(cbuf);
  }

  if (f<0) {
    f=DEFAULT_NUM_FRAMES;
  }
  m_bindname = s;
  m_numframes = (unsigned int)f;
  m_buffer = new imageStruct[m_numframes];

  pd_bind(&this->x_obj->ob_pd, m_bindname);
}
/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_buffer :: ~pix_buffer( void )
{
  pd_unbind(&this->x_obj->ob_pd, m_bindname);

  if(m_buffer) {
    delete [] m_buffer;
  }
  m_buffer=NULL;
  if(m_pimpl) {
    delete m_pimpl;
  }
  m_pimpl=NULL;
}
/////////////////////////////////////////////////////////
// allocateMess
//   allocate memory for m_numframes images of size x*y (with pixelsize=c)
//
/////////////////////////////////////////////////////////
void pix_buffer :: allocateMess(unsigned int x, unsigned int y,
                                unsigned int c)
{
  int i = m_numframes;
  int format=0;

  switch (c) {
  case 1:
    format=GEM_GRAY;
    break;
  case 2:
    format=GEM_YUV;
    break;
  case 3:
    format=GEM_RGB;
    break;
  case 4:
    format=GEM_RGBA;
    break;
  default:
    format=0;
  }

  while(i--) {
    m_buffer[i].xsize=x;
    m_buffer[i].ysize=y;
    m_buffer[i].setFormat(format);
    m_buffer[i].reallocate();
    m_buffer[i].setBlack();
  }
}
/////////////////////////////////////////////////////////
// allocateMess
//   allocate memory for m_numframes images of size x*y (with pixelsize=c)
//
/////////////////////////////////////////////////////////
void pix_buffer :: resizeMess(int newsize)
{
  int size=m_numframes;
  int i;

  if(newsize<0) {
    pd_error(0, "refusing to resize to <0 frames!");
    return;
  }

  imageStruct*buffer = new imageStruct[newsize];
  if(size>newsize) {
    size=newsize;
  }

  for(i=0; i<size; i++) {
    if(0!=m_buffer[i].data) {
      // copy the image
      m_buffer[i].copy2Image(buffer+i);
      m_buffer[i].xsize=1;
      m_buffer[i].ysize=1;
      // "free" the old image
      m_buffer[i].allocate();
    }
  }

  delete[]m_buffer;
  m_buffer=buffer;
  m_numframes=newsize;

  bangMess();
}


/////////////////////////////////////////////////////////
// query the number of frames in the buffer
//
/////////////////////////////////////////////////////////
void pix_buffer :: bangMess( void )
{
  m_pimpl->outlet.send(m_numframes);
}
unsigned int pix_buffer :: numFrames( void )
{
  return m_numframes;
}
/////////////////////////////////////////////////////////
// put an image into the buffer @ position <pos>
//
/////////////////////////////////////////////////////////
bool pix_buffer :: putMess(imageStruct*img, unsigned int pos)
{
  if (pos>=m_numframes) {
    return false;
  }
  if(!img) {
    return false;
  }
  img->copy2Image(m_buffer+pos);
  return true;
}
/////////////////////////////////////////////////////////
// get an image from the buffer @ position <pos>
//
/////////////////////////////////////////////////////////
imageStruct*pix_buffer :: getMess(unsigned int pos)
{

  //post("getting frame: %d", pos);

  if (pos>=m_numframes) {
    return 0;
  }

  /* just allocated but no image */
  if(0==m_buffer[pos].format) {
    return 0;
  }

  return (m_buffer+pos);
}


/////////////////////////////////////////////////////////
// openMess
//
/////////////////////////////////////////////////////////
void pix_buffer :: loadMess(std::string filename, int pos)
{
  // GRH: muss i wie in pix_image die ganzen andern Sachen a machen ????

  // load an image into mem
  imageStruct *image = NULL;
  imageStruct img;

  // some checks
  if (pos<0 || pos>=m_numframes) {
    pd_error(0, "index %d out of range (0..%d)!", pos, m_numframes);
    return;
  }
  std::string file=findFile(filename);

  if(m_pimpl->loader) {
    gem::Properties m_loadprops; /* empty for now */
    m_loadprops.erase("_backends");
    if(!m_pimpl->loadbackends.empty()) {
      m_loadprops.set("_backends", m_pimpl->loadbackends);
    }
    if(m_pimpl->loader->load(file, img, m_loadprops)) {
      image = &img;
    }

    if(!image) {
      pd_error(0, "unable to load image'%s'!", file.c_str());
    }
  } else {
    image = image2mem(file.c_str());
    if(!image) {
      pd_error(0, "'%s' is no valid image!", file.c_str());
      return;
    }
  }

  putMess(image,pos);

  // destroy the image-data
  if(image != &img)
    delete image;
}

/////////////////////////////////////////////////////////
// saveMess
//
/////////////////////////////////////////////////////////
void pix_buffer :: saveMess(std::string filename, int pos)
{
  // save an image from mem
  imageStruct*img=NULL;

  if(filename.empty()) {
    pd_error(0, "no filename given!");
    return;
  }
  img=getMess(pos);

  if(img && img->data) {
    std::string fullname=gem::files::getFullpath(filename);
    if(m_pimpl->saver) {
      m_writeprops.erase("_backends");
      if(!m_pimpl->savebackends.empty()) {
        m_writeprops.set("_backends", m_pimpl->savebackends);
      }
      m_pimpl->saver->save(*img, fullname, std::string(), m_writeprops);
    } else {
      mem2image(img, fullname.c_str(), 0);
    }
  } else {
    pd_error(0, "index %d out of range (0..%d) or slot empty!", pos, m_numframes);
    return;
  }
}

/////////////////////////////////////////////////////////
// copyMess
//
/////////////////////////////////////////////////////////
void pix_buffer :: copyMess(int src, int dst)
{
  if(src==dst) {
    //    pd_error(0, "refusing to do void copying within a slot");
    return;
  }
  // copy an image from one slot to another
  if(putMess(getMess(src), dst)) {
    // fine
  } else {
    pd_error(0, "unable to copy image from slot:%d to slot:%d", src, dst);
    return;
  }
}

void pix_buffer :: enumProperties(void)
{
  std::vector<std::string> mimetypes;
  gem::Properties props;

  props.set("quality", 100);
  if(m_pimpl->saver) {
    m_pimpl->saver->getWriteCapabilities(mimetypes, props);
  }

  std::vector<gem::any>data;
  unsigned int i=0;

  std::vector<std::string>keys=props.keys();

  /* mimetypes */
  data.push_back(std::string("numwrite"));
  data.push_back(mimetypes.size());
  m_pimpl->outlet.send("mimelist", data);

  for(i=0; i<mimetypes.size(); i++) {
    data.clear();
    data.push_back(std::string("write"));
    data.push_back(mimetypes[i]);
    m_pimpl->outlet.send("mimelist", data);
  }

  /* write properties */
  data.clear();
  data.push_back(std::string("numwrite"));
  data.push_back(keys.size());
  m_pimpl->outlet.send("proplist", data);

  for(i=0; i<keys.size(); i++) {
    std::string key=keys[i];
    data.clear();
    data.push_back(std::string("write"));
    data.push_back(key);

    switch(props.type(key)) {
    case gem::Properties::NONE:
      data.push_back(std::string("bang"));
      break;
    case gem::Properties::DOUBLE: {
      double d=-1;
      data.push_back(std::string("float"));
      /* LATER: get and show ranges */
      if(props.get(key, d)) {
        data.push_back(d);
      }
    }
    break;
    case gem::Properties::STRING: {
      data.push_back(std::string("symbol"));
      std::string s;
      if(props.get(key, s)) {
        data.push_back(s);
      }
    }
    break;
    default:
      data.push_back(std::string("unknown"));
      break;
    }

    m_pimpl->outlet.send("proplist", data);
  }
}
void pix_buffer :: clearProperties(void)
{
  m_writeprops.clear();
}
void pix_buffer :: setProperties(t_symbol*s, int argc, t_atom*argv)
{
  m_pimpl->addProperties(m_writeprops, argc, argv);
}
/////////////////////////////////////////////////////////
// backendMess
//
/////////////////////////////////////////////////////////
void pix_buffer :: backendMess(t_symbol*s, int argc, t_atom*argv)
{
  const std::string sel = s->s_name;
  bool saver;
  if(sel == "loadbackend")
    saver = false;
  else if (sel == "savebackend")
    saver = true;
  else {
    error("Use 'loadbackend' to set/get image-loading backends, and 'savebackend' to set/get image-saving backends");
    return;
  }

  if(argc) {
    std::vector<std::string>&backends = saver?m_pimpl->savebackends:m_pimpl->loadbackends;
    backends.clear();
    for(int i=0; i<argc; i++) {
      if(A_SYMBOL == argv->a_type) {
        t_symbol* b=atom_getsymbol(argv+i);
        backends.push_back(b->s_name);
      } else {
        error("%s must be symbolic", s->s_name);
      }
    }
  } else {
    /* no backend requested, just enumerate them */

    if((saver && m_pimpl->saver) || (!saver && m_pimpl->loader)) {
      std::vector<gem::any>atoms;
      gem::any value;
      gem::Properties props;
      std::vector<std::string> backends;
      props.set("_backends", value);
      if(saver) {
        std::vector<std::string> mimetypes;
        m_pimpl->saver->getWriteCapabilities(mimetypes, props);
      } else {
        gem::plugins::imageloader::getProperties(m_pimpl->loader, props);
      }
      if(props.type("_backends")!=gem::Properties::UNSET) {
        props.get("_backends", backends);
      }
      atoms.clear();
      atoms.push_back(value=(int)(backends.size()));
      m_pimpl->outlet.send(sel+"s", atoms);
      if(!backends.empty()) {
        for(int i=0; i<backends.size(); i++) {
          atoms.clear();
          atoms.push_back(value=backends[i]);
          m_pimpl->outlet.send(sel, atoms);
        }
      } else {
        if(saver) {
          post("no image saving backends found!");
        } else {
          post("no image loading backends found!");
        }
      }
    }
  }
}


/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_buffer :: obj_setupCallback(t_class *classPtr)
{
  class_addcreator(reinterpret_cast<t_newmethod>(create_pix_buffer),
                   gensym("pix_depot"),
                   A_GIMME, A_NULL);

  CPPEXTERN_MSG1(classPtr, "resize", resizeMess, int);
  CPPEXTERN_MSG0(classPtr, "bang", bangMess);
  CPPEXTERN_MSG2(classPtr, "open", loadMess, std::string, int);
  CPPEXTERN_MSG2(classPtr, "load", loadMess, std::string, int);
  CPPEXTERN_MSG2(classPtr, "save", saveMess, std::string, int);
  CPPEXTERN_MSG2(classPtr, "copy", copyMess, int, int);
  CPPEXTERN_MSG (classPtr, "allocate", allocateMess);

  CPPEXTERN_MSG0(classPtr, "enumProps",  enumProperties);
  CPPEXTERN_MSG0(classPtr, "clearProps", clearProperties);
  CPPEXTERN_MSG (classPtr, "setProp",    setProperties);
  CPPEXTERN_MSG (classPtr, "setProps",   setProperties);

  CPPEXTERN_MSG (classPtr, "backend", backendMess);
  CPPEXTERN_MSG (classPtr, "loadbackend", backendMess);
  CPPEXTERN_MSG (classPtr, "savebackend", backendMess);
}
void pix_buffer :: allocateMess(t_symbol*s, int argc, t_atom*argv)
{
  unsigned int x=0;
  unsigned int y=0;
  unsigned int c=0;

  t_atom*ap=0;

  switch(argc) {
  case 3:
    ap=argv+2;
    if(A_SYMBOL==ap->a_type) {
      char c0 =*atom_getsymbol(ap)->s_name;

      switch (c0) {
      case 'g':
      case 'G':
        c=1;
        break;
      case 'y':
      case 'Y':
        c=2;
        break;
      case 'r':
      case 'R':
        c=4;
        break;
      default:
        pd_error(0, "invalid format %s!", atom_getsymbol(ap)->s_name);
        return;
      }
    } else if(A_FLOAT==ap->a_type) {

      c=(unsigned int)atom_getint(ap);

    } else {
      pd_error(0, "invalid format!");
      return;
    }
  case 2:
    if((A_FLOAT==argv->a_type) && (A_FLOAT==(argv+1)->a_type)) {
      int i=atom_getint(argv);
      if(i<0) {
        pd_error(0, "invalid dimensions: x=%d < 0", i);
        return;
      }
      x=(unsigned int)i;

      i=atom_getint(argv+1);
      if(i<0) {
        pd_error(0, "invalid dimensions: y=%d < 0", i);
        return;
      }
      y=(unsigned int)i;
    } else {
      pd_error(0, "invalid dimensions!");
      return;
    }
    break;
  case 1:
    if(A_FLOAT==argv->a_type) {
      int i=atom_getint(argv);
      if(i<0) {
        pd_error(0, "invalid dimensions: x=%d < 0", i);
        return;
      }
      x=(unsigned int)i;
      y=1;
      c=1;
    } else {
      pd_error(0, "invalid dimension!");
      return;
    }
    break;
  default:
    pd_error(0, "usage: allocate <width> <height> <format>");
    return;
  }

  if (x<1 || y<1) {
    pd_error(0, "init-specs out of range");
    return;
  }
  if (c==0) {
    c=4;
  }

  allocateMess((int)x, (int)y, (int)c);
}
