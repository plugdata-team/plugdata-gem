////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 2011-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "pix_frei0r.h"
#include "Gem/Exception.h"
#include "Gem/Loaders.h"

#include "Gem/Dylib.h"
#include "RTE/RTE.h"

#include "Gem/Properties.h"

#include <iostream>
#include <stdio.h>

#include <map>

#ifdef _MSC_VER
#    include <intrin.h>
#    define BYTESWAP(x) _byteswap_ulong(x)
#else
#    define BYTESWAP(x) __builtin_bswap32(x)
#endif


#ifdef _WIN32
# include <io.h>
# include <windows.h>
# define snprintf _snprintf
# define close _close

#elif defined __APPLE__ && 0
# include <mach-o/dyld.h>
# include <unistd.h>
#else
# define DL_OPEN
# include <dlfcn.h>
# include <unistd.h>
#endif /* __APPLE__ */

#include <string.h>

#include "Gem/GemGL.h"


#ifndef HAVE_STRNLEN
#define strnlen f0r_strnlen
static size_t f0r_strnlen(const char* str, size_t maxlen)
{
  size_t len=0;
  if(NULL==str) {
    return len;
  }
  while(*str++ && len<maxlen) {
    len++;
  }

  return len;
}
#endif

namespace {
  static std::vector<std::string>s_frei0r_paths;
  void frei0r_paths_initialize(void) {
    std::vector<std::string>paths;
    const char*f0path = getenv("FREI0R_PATH");
    if (f0path) {
      std::string frei0r_path = f0path;

#ifdef _WIN32
      const char *separator = ";";
#else
      const char *separator = ":";
#endif
      size_t start;
      size_t end = 0;
      while ((start = frei0r_path.find_first_not_of(separator, end)) != std::string::npos) {
        end = frei0r_path.find(separator, start);
        paths.push_back(frei0r_path.substr(start, end - start));
      }
    } else {
      /* no FREI0R_PATH...fall back to the defaults */
      static const char* const frei0r_pathlist[] = {
        "/usr/local/lib/frei0r-1/",
        "/usr/lib/frei0r-1/",
        /* hmm, i guess these are only valid for 64bit archs, but how do we catch them? */
        "/usr/local/lib64/frei0r-1/",
        "/usr/lib64/frei0r-1/",
#define MULTIARCH_TRIPLET "x86_64-linux-gnu"
#ifdef MULTIARCH_TRIPLET
        /* Debian's multiarch */
        "/usr/local/lib/" MULTIARCH_TRIPLET "/frei0r-1/",
        "/usr/lib/" MULTIARCH_TRIPLET "/frei0r-1/",
#endif
        0
      };

      f0path = getenv("HOME");
      if(f0path) {
        std::string p(f0path);
        paths.push_back(p + "/.frei0r-1/lib/");
        paths.push_back(p + "/.local/lib/frei0r-1/");
#ifdef MULTIARCH_TRIPLET
        /* Debian's multiarch */
        paths.push_back(p + "/.local/lib/" MULTIARCH_TRIPLET "/frei0r-1/");
#endif
      }
      for(int i=0; frei0r_pathlist[i]; i++) {
        paths.push_back(frei0r_pathlist[i]);
      }
    }

    s_frei0r_paths = paths;
  }
};


class pix_frei0r::F0RPlugin
{
public:
  bool init(void)
  {
    if(!f0r_init) {
      return false;
    }

    if(!f0r_get_plugin_info) {
      return false;
    }
    if(!f0r_get_param_info) {
      return false;
    }
    if(!f0r_construct) {
      return false;
    }
    if(!f0r_destruct) {
      return false;
    }
    if(!f0r_set_param_value) {
      return false;
    }
    if(!f0r_get_param_value) {
      return false;
    }
    if(!f0r_deinit) {
      return false;
    }

    int err=0;

    if(f0r_init) {
      err=f0r_init();
    }
    if (err<0) {
      pd_error(0, "[pix_frei0r] failed to initialize plugin");
      return false;
    }

    f0r_plugin_info_t info;
    f0r_get_plugin_info(&info);
    m_name = info.name;
    m_author = info.author;
    m_type = info.plugin_type;
    switch(m_type) {
    case (F0R_PLUGIN_TYPE_SOURCE):
    case (F0R_PLUGIN_TYPE_FILTER):
      break;
    default:
      pd_error(0, "[pix_frei0r] only supports sources/filters, no mixers!");
      return false;
    }

    m_color = info.color_model;
#ifdef __GNUC__
# warning check compatibility
#endif
    m_frei0rVersion = info.frei0r_version;
    m_majorVersion = info.major_version;
    m_minorVersion = info.minor_version;
    m_explanation = info.explanation;

    ::post("%s by %s", info.name, info.author);
    ::post("%d:: %s", m_type, info.explanation);


    if(!f0r_update) {
      return false;
    }
    // if(!f0r_update2)return false;

    int numparameters = info.num_params;
    int i=0;
    m_parameterNames.clear();
    m_parameterTypes.clear();
    m_parameter.clear();

    // dummy parameter (so we start at 1)
    m_parameterNames.push_back("");
    m_parameterTypes.push_back(0);

    for(i=0; i<numparameters; i++) {
      f0r_param_info_t pinfo;
      f0r_get_param_info(&pinfo, i);
      m_parameterNames.push_back(pinfo.name);
      m_parameterTypes.push_back(pinfo.type);

      ::post("parm%02d[%s]: %s", i+1, pinfo.name, pinfo.explanation);
    }

    /* check color-space */
    bool compat_color = true;
    switch(m_color) {
    case F0R_COLOR_MODEL_PACKED32:
      break;
    case F0R_COLOR_MODEL_RGBA8888:
      compat_color = (GEM_RGBA == GEM_RAW_RGBA);
      break;
    case F0R_COLOR_MODEL_BGRA8888:
      compat_color = (GEM_RGBA == GEM_RAW_BGRA);
      break;
    default:
      compat_color = false;
      break;
    }
    if(!compat_color)
      ::post("pix_frei0r: currently only plugins using the RGBA colorspace are properly supported");

    return true;
  }
  void deinit(void)
  {
    destruct();
    if(f0r_deinit) {
      f0r_deinit();
    }
  }

  unsigned int m_width, m_height;

  bool construct(unsigned int width, unsigned int height)
  {
    destruct();
    m_instance=f0r_construct(width, height);
    m_width=width;
    m_height=height;
    return (m_instance!=NULL);
  }
  void destruct(void)
  {
    if(m_instance) {
      f0r_destruct(m_instance);
    }
    m_instance=NULL;
  }

  f0r_instance_t m_instance;

  std::string m_name;
  std::string m_author;
  int   m_type;
  int   m_color;
  int   m_frei0rVersion;
  int   m_majorVersion;
  int   m_minorVersion;
  std::string m_explanation;

  gem::Properties m_parameter;
  std::vector<std::string>m_parameterNames;
  std::vector<int>m_parameterTypes;

  typedef int (*t_f0r_init)(void);
  typedef void (*t_f0r_get_plugin_info)(f0r_plugin_info_t* pluginInfo);
  typedef void (*t_f0r_get_param_info)(f0r_param_info_t* info,
                                       int param_index);
  typedef f0r_instance_t (*t_f0r_construct)(unsigned int width,
      unsigned int height);
  typedef void (*t_f0r_destruct)(f0r_instance_t instance);
  typedef void (*t_f0r_set_param_value)(f0r_instance_t instance,
                                        f0r_param_t param, int param_index);
  typedef void (*t_f0r_get_param_value)(f0r_instance_t instance,
                                        f0r_param_t param, int param_index);
  typedef void (*t_f0r_update) (f0r_instance_t instance, double time,
                                const uint32_t* inframe, uint32_t* outframe);
  typedef void (*t_f0r_update2)(f0r_instance_t instance, double time,
                                const uint32_t* inframe1, const uint32_t* inframe2,
                                const uint32_t* inframe3, uint32_t* outframe);
  typedef int (*t_f0r_deinit)(void);


  t_f0r_init f0r_init;
  t_f0r_get_plugin_info f0r_get_plugin_info;
  t_f0r_get_param_info f0r_get_param_info;
  t_f0r_construct f0r_construct;
  t_f0r_destruct f0r_destruct;
  t_f0r_set_param_value f0r_set_param_value;
  t_f0r_get_param_value f0r_get_param_value;
  t_f0r_update f0r_update;
  t_f0r_update2 f0r_update2;
  t_f0r_deinit f0r_deinit;


  void close(void)
  {
    destruct();
    if(f0r_deinit) {
      int err=f0r_deinit();
      if(err) {
        ::pd_error(0, "[%s] f0r_deinit() failed with %d", m_name.c_str(), err);
      }
    }
  }

  F0RPlugin(const std::string&name) :
    m_width(0), m_height(0),
    m_instance(NULL),
    m_name(""), m_author(""),
    m_type(0), m_color(0),
    m_frei0rVersion(0), m_majorVersion(0), m_minorVersion(0),
    m_explanation(""),
    m_dylib(name)
  {
    f0r_init           =reinterpret_cast<t_f0r_init           >
                        (m_dylib.proc("f0r_init"));
    f0r_get_plugin_info=reinterpret_cast<t_f0r_get_plugin_info>
                        (m_dylib.proc("f0r_get_plugin_info"));
    f0r_get_param_info =reinterpret_cast<t_f0r_get_param_info >
                        (m_dylib.proc("f0r_get_param_info"));
    f0r_construct      =reinterpret_cast<t_f0r_construct      >
                        (m_dylib.proc("f0r_construct"));
    f0r_destruct       =reinterpret_cast<t_f0r_destruct       >
                        (m_dylib.proc("f0r_destruct"));
    f0r_set_param_value=reinterpret_cast<t_f0r_set_param_value>
                        (m_dylib.proc("f0r_set_param_value"));
    f0r_get_param_value=reinterpret_cast<t_f0r_get_param_value>
                        (m_dylib.proc("f0r_get_param_value"));
    f0r_update         =reinterpret_cast<t_f0r_update         >
                        (m_dylib.proc("f0r_update"));
    f0r_update2        =reinterpret_cast<t_f0r_update2        >
                        (m_dylib.proc("f0r_update2"));
    f0r_deinit         =reinterpret_cast<t_f0r_deinit         >
                        (m_dylib.proc("f0r_deinit"));

    if(!init()) {
      deinit();
      throw(GemException("couldn't instantiate frei0r plugin"));
    }
  }

  bool set(unsigned int key, bool value)
  {
    if(!m_instance) {
      return false;
    }
    f0r_param_bool v=value;
    f0r_set_param_value(m_instance, &v, key);
    return true;
  }
  bool set(unsigned int key, double value)
  {
    if(!m_instance) {
      return false;
    }
    f0r_param_double v=value;
    f0r_set_param_value(m_instance, &v, key);
    return true;
  }
  bool set(unsigned int key, double x, double y)
  {
    if(!m_instance) {
      return false;
    }
    f0r_param_position v;
    v.x=x;
    v.y=y;
    f0r_set_param_value(m_instance, &v, key);
    return true;
  }
  bool set(unsigned int key, double r, double g, double b)
  {
    if(!m_instance) {
      return false;
    }
    f0r_param_color v;
    v.r=r;
    v.g=g;
    v.b=b;
    f0r_set_param_value(m_instance, &v, key);
    return true;
  }
  bool set(unsigned int key, std::string s)
  {
    if(!m_instance) {
      return false;
    }
    f0r_param_string v=const_cast<f0r_param_string>(s.c_str());
    f0r_set_param_value(m_instance, &v, key);
    return true;
  }


  bool process(double time, imageStruct&input, imageStruct&output)
  {
    if(!m_instance || m_width!=input.xsize || m_height!=input.ysize) {
      construct(input.xsize, input.ysize);
    }

    if(!m_instance) {
      return false;
    }

    f0r_update(m_instance, time,
               reinterpret_cast<const uint32_t*>(input.data),
               reinterpret_cast<uint32_t*>(output.data));

    return true;
  }

  GemDylib m_dylib;
};

static std::map<const t_symbol*, std::string>s_class2filename;

CPPEXTERN_NEW_WITH_ONE_ARG(pix_frei0r,  t_symbol*, A_DEFSYMBOL);

/////////////////////////////////////////////////////////
//
// pix_frei0r
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_frei0r :: pix_frei0r(t_symbol*s)
  : m_plugin(NULL)
  , m_canopen(false)
{
  //  throw(GemException("Gem has been compiled without Frei0r-support!"));
  int can_rgba=0;
  m_image.setFormat(GEM_RGBA);
  m_converterImage.setFormat(GEM_RGBA);

  if(!s || s==&s_) {
    m_canopen=true;
    return;
  }
  std::string pluginname = s->s_name;
  std::string filename = pluginname;
  if(s_class2filename.find(s) != s_class2filename.end()) {
    filename=s_class2filename[s];
    //::post("using cached filename %s", filename.c_str());
    try {
      m_plugin = new F0RPlugin(filename);
    } catch (GemException&e) {
      // ignore the error, and keep trying
      m_plugin = 0;
    }
  }
  if (0 == m_plugin) {
    gem::RTE::RTE*rte=gem::RTE::RTE::getRuntimeEnvironment();
    if(rte) {
      filename=rte->findFile(pluginname, GemDylib::getDefaultExtension(),
                             getCanvas());
    }

    m_plugin = new F0RPlugin(filename);
  }

  unsigned int numparams = m_plugin->m_parameterNames.size();
  char tempVt[5];

  for(unsigned int i=1; i<numparams; i++) {
    snprintf(tempVt, 5, "#%u", i);
    tempVt[4]=0;
    unsigned int parmType=0;
    t_symbol*s_inletType;
    parmType=m_plugin->m_parameterTypes[i];

    switch(parmType) {
    case(F0R_PARAM_BOOL):
    case(F0R_PARAM_DOUBLE):
      s_inletType=gensym("float");
      break;
    case(F0R_PARAM_COLOR):
    case(F0R_PARAM_POSITION):
      s_inletType=gensym("list");
      break;
    case(F0R_PARAM_STRING):
      s_inletType=gensym("symbol");
      break;
    default:
      s_inletType=&s_;
    }
    m_inlet.push_back(inlet_new(this->x_obj, &this->x_obj->ob_pd, s_inletType,
                                gensym(tempVt)));
  }
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_frei0r :: ~pix_frei0r()
{
  while(!m_inlet.empty()) {
    t_inlet*in=m_inlet.back();
    if(in) {
      inlet_free(in);
    }
    m_inlet.pop_back();
  }
  closeMess();
}

void pix_frei0r :: closeMess()
{
  if(m_plugin) {
    delete m_plugin;
  }
  m_plugin=NULL;
}

void pix_frei0r :: openMess(t_symbol*s)
{
  if(!m_canopen) {
    pd_error(0, "this instance cannot dynamically change the plugin");
    return;
  }

  std::string pluginname = s->s_name;

  if(m_plugin) {
    delete m_plugin;
  }
  m_plugin=NULL;
  try {
    std::string filename = pluginname;
    gem::RTE::RTE*rte=gem::RTE::RTE::getRuntimeEnvironment();
    if(rte) {
      filename=rte->findFile(pluginname, GemDylib::getDefaultExtension(),
                             getCanvas());
    }
    m_plugin = new F0RPlugin(filename);
  } catch (GemException&x) {
    pd_error(0, "%s", x.what());
  }

  if(NULL==m_plugin) {
    pd_error(0, "unable to open '%s'", pluginname.c_str());
    return;
  }
}

namespace
{
void swapBytes(imageStruct &image)
{
  size_t pixelnum=image.xsize*image.ysize;
  unsigned int *pixels=(unsigned int*)image.data;
  while(pixelnum--) {
    unsigned int p = BYTESWAP(*pixels);
    *pixels++ = p;
  }
}
};

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_frei0r :: processRGBAImage(imageStruct &image)
{
  static double time=0;
  if(!m_plugin) {
    return;
  }

  m_image.xsize=image.xsize;
  m_image.ysize=image.ysize;
  m_image.reallocate();
  if(GL_UNSIGNED_INT_8_8_8_8 == m_image.type) {
    swapBytes(image);
  }
  m_plugin->process(time, image, m_image);
  time++;

  image.data   = m_image.data;
  if(GL_UNSIGNED_INT_8_8_8_8 == image.type) {
    swapBytes(image);
  }
  image.not_owned = true;
  image.setFormat(m_image.format);
}

void pix_frei0r :: parmMess(const std::string&key, int argc, t_atom *argv)
{
  if(!m_plugin) {
    pd_error(0, "no plugin present! forgetting parameter....");
    return;
  }
  unsigned int i=0;
  for(i=0; i<m_plugin->m_parameterNames.size(); i++) {
    if(key==m_plugin->m_parameterNames[i]) {
      parmMess(i, argc, argv);
      return;
    }
  }
  pd_error(0, "unknown parameter '%s'", key.c_str());
}


void pix_frei0r :: parmMess(int key, int argc, t_atom *argv)
{
  unsigned int realkey=0;
  if(!m_plugin) {
    pd_error(0, "no plugin present! forgetting parameter....");
    return;
  }
  if(key<=0) {
    pd_error(0, "parameterIDs must be >0");
    return;
  } else {
    realkey=key-1;
  }
  if(static_cast<unsigned int>(key)>=m_plugin->m_parameterNames.size()) {
    pd_error(0, "parameterID out of bounds");
    return;
  }

  int type=m_plugin->m_parameterTypes[key];

  double r, g, b;
  double x, y;

  const char*name=m_plugin->m_parameterNames[key].c_str();
  switch(type) {
  case(F0R_PARAM_BOOL):
    if(argc!=1) {
      pd_error(0, "param#%02d('%s') is of type BOOL: need exactly 1 argument", key,
            name);
      return;
    }
    m_plugin->set(realkey, (atom_getfloat(argv)>0.5));
    break;
  case(F0R_PARAM_DOUBLE):
    if(argc!=1) {
      pd_error(0, "param#%02d('%s') is of type DOUBLE: need exactly 1 argument", key,
            name);
      return;
    }
    m_plugin->set(realkey, static_cast<double>(atom_getfloat(argv)));
    break;
  case(F0R_PARAM_COLOR):
    if(argc!=3) {
      pd_error(0, "param#%02d('%s') is of type COLOR: need exactly 3 arguments", key,
            name);
      return;
    }
    r=atom_getfloat(argv+0);
    g=atom_getfloat(argv+1);
    b=atom_getfloat(argv+2);
    m_plugin->set(realkey, r, g, b);
    break;
  case(F0R_PARAM_POSITION):
    if(argc!=2) {
      pd_error(0, "param#%02d('%s') is of type POSITION: need exactly 2 arguments",
            key, name);
      return;
    }
    x=atom_getfloat(argv+0);
    y=atom_getfloat(argv+1);
    m_plugin->set(realkey, x, y);
    break;
  case(F0R_PARAM_STRING):
    if(argc!=1) {
      pd_error(0, "param#%02d('%s') is of type STRING: need exactly 1 argument", key,
            name);
      return;
    }
    m_plugin->set(realkey, std::string(atom_getsymbol(argv)->s_name));
    break;
  default:
    pd_error(0, "param#%02d('%s') is of UNKNOWN type", key, name);
    break;
  }
  setPixModified();
}

static const int offset_pix_=strlen("pix_");

static void*frei0r_loader_new(t_symbol*s, int argc, t_atom*argv)
{
  if(!s) {
    ::logpost(0, 3+2, "frei0r_loader: no name given");
    return 0;
  }
  ::logpost(0, 3+2, "frei0r_loader: %s",s->s_name);
  try {
    const char*realname=s->s_name+offset_pix_; /* strip of the leading 'pix_' */
    gem::CPPExtern_proxy proxy(pix_frei0r_class, s->s_name, s, argc, argv,
                               0, NULL, 1);
    argc = proxy.getNumArgs();
    proxy.setObject(new pix_frei0r(gensym(realname)));
    return proxy.initialize();
  } catch (GemException&e) {
    ::logpost(0, 3+2, "frei0r_loader: failed! (%s)", e.what());
    return 0;
  }
  return 0;
}
bool pix_frei0r :: loader(const t_canvas*canvas,
                          const std::string&classname, const std::string&path,
                          bool legacy)
{
  if(strncmp("pix_", classname.c_str(), offset_pix_)) {
    return false;
  }
  std::string pluginname = classname.substr(offset_pix_);
  std::string filename = pluginname;
  gem::RTE::RTE*rte=gem::RTE::RTE::getRuntimeEnvironment();
  if(rte) {
    if (legacy) {
      filename=rte->findFile(pluginname, GemDylib::getDefaultExtension(),
                             canvas);
    } else {
      if (path.empty()) {
        for(size_t i=0; i<s_frei0r_paths.size(); i++) {
          if (!s_frei0r_paths[i].empty())
            if (loader(canvas, classname, s_frei0r_paths[i], legacy))
              return true;
        }
        return false;
      } else {
        filename=rte->findFile(path+"/"+pluginname,
                               GemDylib::getDefaultExtension(), canvas);
      }
    }
  }
  pix_frei0r::F0RPlugin*plugin=NULL;
  try {
    plugin=new F0RPlugin(filename);
  } catch (GemException&e) {
    ::logpost(0, 3+2, "frei0r_loader: failed!! (%s)", e.what());
    return false;
  }

  if(plugin!=NULL) {
    delete plugin;
    /* cache the filename that loads this plugin */
    s_class2filename[gensym(pluginname.c_str())]=filename;
    /* register a new class */
    class_addcreator(reinterpret_cast<t_newmethod>(frei0r_loader_new),
                     gensym(classname.c_str()), A_GIMME, 0);
    return true;
  }
  return false;
}

static int frei0r_loader(const t_canvas *canvas, const char *classname,
                         const char *path, bool legacy)
{
  return pix_frei0r::loader(canvas, classname, path?path:"", legacy);
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_frei0r :: obj_setupCallback(t_class *classPtr)
{
  class_addanything(classPtr,
                    reinterpret_cast<t_method>(&pix_frei0r::parmCallback));
  CPPEXTERN_MSG1(classPtr, "load", openMess, t_symbol*);
  gem_register_loader(frei0r_loader);
  gem_register_loader_nopath(frei0r_loader);
  frei0r_paths_initialize();
}

void pix_frei0r :: parmCallback(void *data, t_symbol*s, int argc,
                                t_atom*argv)
{
  if('#'==s->s_name[0]) {
    int i = atoi(s->s_name+1);
    GetMyClass(data)->parmMess(i, argc, argv);
  } else {
    GetMyClass(data)->parmMess(std::string(s->s_name), argc, argv);
  }
}
