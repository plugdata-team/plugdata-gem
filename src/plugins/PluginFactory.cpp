#include "PluginFactory.h"
#include "Gem/Settings.h"
#include "Gem/Files.h"
#include "Gem/Dylib.h"
#include "Gem/RTE.h"

#include "sstream"

using namespace gem;

class gem::BasePluginFactory::Pimpl
{
  friend class BasePluginFactory;
  Pimpl(void)
  {

  }

  ~Pimpl(void)
  {

  }

  std::vector<std::string>p_loaded;

  std::map<std::string, void*>p_ctors;
};


gem::BasePluginFactory::BasePluginFactory(void) : m_pimpl(new Pimpl)
{

}
gem::BasePluginFactory::~BasePluginFactory(void)
{
  delete m_pimpl;
  m_pimpl=NULL;
}

int gem::BasePluginFactory::doLoadPlugins(const std::string&basename,
    const std::string&path_)
{
  int already=m_pimpl->p_loaded.size();
  if(already>0) {
    int once=1;
    gem::Settings::get("gem.plugins.once", once);
    std::string key="gem.plugins.";
    key+=basename;
    key+=".once";
    gem::Settings::get(key, once);

    if(0!=once) {
      std::cerr << "not reloading '" << basename <<
                "' plugins (already "<<already<<" loaded)" << std::endl;
      return 0;
    }
  }
  std::string path = path_;
  if(path.empty()) {
    gem::Settings::get("gem.path", path);
  }
  if(!path.empty()) {
    path=path+std::string("/");
  }
  std::cerr << "load plugins '" << basename << "' in '" << path << "'" <<
            std::endl;

  std::string pattern = path+std::string("gem_") + basename+std::string("*")
                        +GemDylib::getDefaultExtension();
  std::cerr << "pattern : " << pattern << std::endl;

  unsigned int count=0;

  std::vector<std::string>files=gem::files::getFilenameListing(pattern);

  for(unsigned int i=0; i<files.size(); i++) {
    GemDylib*dll=NULL;
    const std::string f=files[i];
    // check whether this file has already been loaded
    // LATER make checks more sophisticated (like checking file-handles)
    bool alreadyloaded=false;
    for(unsigned int j=0; j<m_pimpl->p_loaded.size(); j++) {
      if(f == m_pimpl->p_loaded[j]) {
        alreadyloaded=true;
        std::cerr << "not reloading '"<<f<<"'"<<std::endl;
        break;
      }
    }
    if(alreadyloaded) {
      continue;
    }

    std::cerr << "dylib loading file '" << f << "'!" << std::endl;
    dll=NULL;
    try {
      dll=new GemDylib(f, "");
    } catch (GemException&x) {
      // oops, on w32 this might simply be because getFilenameListing() stripped the path
      // so let's try again, with Path added...
      if(f.find(path) == f.npos) {
        try {
          std::string f1=path;
          f1+=f;
          dll=new GemDylib(f1, "");
        } catch (GemException&x1) {
          // giving up
          std::cerr << "library loading returned: " << x1.what() << std::endl;
          dll=NULL;
        }
      } else {
        std::cerr << "library loading returned: " << x.what() << std::endl;
        dll=NULL;
      }
    }
    if(dll) { // loading succeeded
      try {
        m_pimpl->p_loaded.push_back(f);
        count++;
      } catch (GemException&x) {
        std::cerr << "plugin loading returned: " << x.what() << std::endl;
      }
    }

  }

  return count;
}

std::vector<std::string>gem::BasePluginFactory::get()
{
  std::vector<std::string>result;
  if(m_pimpl) {
    for(std::map<std::string, void*>::iterator iter = m_pimpl->p_ctors.begin(); iter != m_pimpl->p_ctors.end(); ++iter) {
      if(NULL!=iter->second) {
        result.push_back(iter->first);
      }
    }
  }
  return result;
}

void*gem::BasePluginFactory::get(std::string id)
{
  void*ctor=NULL;
  if(m_pimpl) {
    ctor=m_pimpl->p_ctors[id];
  }
  return ctor;
}

void gem::BasePluginFactory::set(std::string id, void*ptr)
{
  if(m_pimpl) {
    m_pimpl->p_ctors[id]=ptr;
  }
}



#include "film.h"
#include "imageloader.h"
#include "imagesaver.h"
#include "modelloader.h"
#include "record.h"
#include "video.h"


namespace
{
static bool default_true (const char*name, int global, int local)
{
  bool res = true;
  if(local<0) {
    res=(0!=global);
  } else {
    res=(0!=local);
  }
  return res;
}

}
#define PLUGIN_INIT(x) s=-1; gem::Settings::get("gem.plugins."#x".startup", s); \
  if(default_true("gem.plugins."#x".startup", s0,s))delete x::getInstance()

namespace gem
{
namespace plugins
{
void init(void)
{
  int s, s0=1;
  gem::Settings::get("gem.plugins.startup", s0);
  using namespace gem::plugins;

  PLUGIN_INIT(film);
  PLUGIN_INIT(imageloader);
  PLUGIN_INIT(imagesaver);
  PLUGIN_INIT(modelloader);
  PLUGIN_INIT(record);
  PLUGIN_INIT(video);
}
};
};
