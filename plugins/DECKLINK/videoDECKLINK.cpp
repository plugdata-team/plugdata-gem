////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 2015 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "videoDECKLINK.h"

#include "plugins/PluginFactory.h"
#include "Gem/RTE.h"
#include "Gem/Exception.h"

#include <pthread.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <map>

#include <stdio.h>
#define MARK() printf("%s:%d\t%s\n", __FILE__, __LINE__, __FUNCTION__)

#ifdef _WIN32
#include <windows.h>
#include "DeckLinkAPI_i.c"
typedef BOOL deckbool_t;
typedef wchar_t* deckstring_t;
namespace {
  deckstring_t string2deckstring(const std::string&s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), s.size(), NULL, 0);
    deckstring_t buf = new wchar_t[n];
    MultiByteToWideChar(CP_UTF8, 0, s.data(), s.size(), &buf[0], n);
    return buf;
  }
  std::string deckstring2string(deckstring_t s) {
    if (!s){
      return std::string();
    }
    size_t l = wcslen(s);
    int n = WideCharToMultiByte(CP_UTF8, 0, s, l, NULL, 0, NULL, NULL);
    std::string buf;
    buf.resize(n);
    WideCharToMultiByte(CP_UTF8, 0, s, l, &buf[0], n, NULL, NULL);
    return buf;
  }
  void free_deckstring(deckstring_t s) {
    wchar_t*b=s;
    delete[]b;
  }
};
#elif defined __APPLE__
typedef bool deckbool_t;
typedef CFStringRef deckstring_t;
namespace {
  deckstring_t string2deckstring(const std::string&s) {
    return CFStringCreateWithCString(NULL, s.c_str(), kCFStringEncodingUTF8);
  }
  std::string deckstring2string(deckstring_t s) {
    if (const char* fastCString = CFStringGetCStringPtr(s, kCFStringEncodingUTF8))
    {
      return std::string(fastCString);
    }
    CFIndex utf16length = CFStringGetLength(s);
    CFIndex maxUtf8len = CFStringGetMaximumSizeForEncoding(utf16length, kCFStringEncodingUTF8);
    char*cstr = new char[maxUtf8len+1];
    cstr[maxUtf8len] = 0;
    CFStringGetCString(s, cstr, maxUtf8len, kCFStringEncodingUTF8);
    std::string converted(cstr);
    delete[]cstr;
    return converted;
  }
  void free_deckstring(deckstring_t s) {
    CFRelease(s);
  }
};
#else /* linux */
# include <string.h>
typedef bool deckbool_t;
typedef const char* deckstring_t;
namespace {
  deckstring_t string2deckstring(const std::string&s) {
    return strdup(s.c_str());
  }
  std::string deckstring2string(deckstring_t s) {
    return std::string(s);
  }
  void free_deckstring(deckstring_t s) {
    free((void*)s);
  }
};
#endif

static std::map<std::string, BMDVideoConnection> s_connectionstrings;
namespace {
  BMDVideoConnection string2connection(std::string Name) {
    static bool done = false;
    if(!done) {
      s_connectionstrings["auto"] = (BMDVideoConnection)-1;
      s_connectionstrings["sdi"] = bmdVideoConnectionSDI;
      s_connectionstrings["hdmi"] = bmdVideoConnectionHDMI;
      s_connectionstrings["opticalsdi"] = bmdVideoConnectionOpticalSDI;
      s_connectionstrings["component"] = bmdVideoConnectionComponent;
      s_connectionstrings["composite"] = bmdVideoConnectionComposite;
      s_connectionstrings["svideo"] = bmdVideoConnectionSVideo;
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
#else
      verbose(0, "[GEM:videoDECKLINK] lacking C++11 support requires connections to be lower-case");
#endif
    }
    done=true;
    std::string name = std::string(Name);
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
#else
#endif
    std::map<std::string, BMDVideoConnection>::iterator it = s_connectionstrings.find(name);
    if(s_connectionstrings.end() != it)
      return it->second;
    return (BMDVideoConnection)0;
  }
  std::string connection2string(BMDVideoConnection conn) {
    static bool done = false;
    if(!done) string2connection("");
    done = true;
    for (std::map<std::string, BMDVideoConnection>::iterator it = s_connectionstrings.begin(); it != s_connectionstrings.end(); ++it)
      if (it->second == conn)
        return it->first;
    return "";
  }
};
static std::map<std::string, BMDPixelFormat> s_pixformatstrings;
namespace {
  BMDPixelFormat string2pixformat(std::string Name) {
    static bool done = false;
    if(!done) {
      s_pixformatstrings["yuv8"] = bmdFormat8BitYUV;
      s_pixformatstrings["yuv10"] = bmdFormat10BitYUV;
      s_pixformatstrings["argb8"] = bmdFormat8BitARGB;
      s_pixformatstrings["bgra8"] = bmdFormat8BitBGRA;
      s_pixformatstrings["yuv"] = bmdFormat8BitYUV;
      s_pixformatstrings["argb"] = bmdFormat8BitARGB;
      s_pixformatstrings["bgra"] = bmdFormat8BitBGRA;
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
#else
      verbose(0, "[GEM:videoDECKLINK] lacking C++11 support requires pixformats to be lower-case");
#endif
    }
    done=true;
    std::string name = std::string(Name);
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
#else
#endif
    std::map<std::string, BMDPixelFormat>::iterator it = s_pixformatstrings.find(name);
    if(s_pixformatstrings.end() != it)
      return it->second;
    return (BMDPixelFormat)0;
  }
  std::string pixformat2string(BMDPixelFormat conn) {
    static bool done = false;
    if(!done) string2pixformat("");
    done = true;
    for (std::map<std::string, BMDPixelFormat>::iterator it = s_pixformatstrings.begin(); it != s_pixformatstrings.end(); ++it)
      if (it->second == conn)
        return it->first;
    return "";
  }
};

/* -LICENSE-START-
** Copyright (c) 2013 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
**
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/
class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
public:
  ULONG m_refCount;
  pthread_mutex_t m_mutex;

  unsigned long m_frameCount;
  BMDVideoInputFlags m_cfg_inputFlags;
  BMDPixelFormat m_cfg_pixelFormat;
  IDeckLinkInput*m_deckLinkInput;

  gem::plugins::videoDECKLINK*m_priv;

public:
  DeckLinkCaptureDelegate(gem::plugins::videoDECKLINK*parent,
                          IDeckLinkInput*dli)
    : IDeckLinkInputCallback()
    , m_refCount(0)
    , m_frameCount(0)
    , m_cfg_inputFlags(bmdVideoInputFlagDefault)
    , m_cfg_pixelFormat(parent->m_pixelFormat)
    , m_deckLinkInput(dli)
    , m_priv(parent)
  {
    m_deckLinkInput->AddRef();
    pthread_mutex_init(&m_mutex, NULL);
    m_deckLinkInput->SetCallback(this);
    //result = m_deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(), m_cfg_pixelFormat, m_cfg_inputFlags);
    //m_deckLinkInput->DisableAudioInput();

  }

  ~DeckLinkCaptureDelegate()
  {
    m_deckLinkInput->Release();
    pthread_mutex_destroy(&m_mutex);
  }
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
  {
    return E_NOINTERFACE;
  }
  virtual ULONG STDMETHODCALLTYPE AddRef(void)
  {
    pthread_mutex_lock(&m_mutex);
    m_refCount++;
    pthread_mutex_unlock(&m_mutex);

    return (ULONG)m_refCount;
  }
  virtual ULONG STDMETHODCALLTYPE  Release(void)
  {
    pthread_mutex_lock(&m_mutex);
    m_refCount--;
    pthread_mutex_unlock(&m_mutex);

    if (m_refCount == 0) {
      delete this;
      return 0;
    }

    return (ULONG)m_refCount;
  }
  virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(
    IDeckLinkVideoInputFrame*videoFrame, IDeckLinkAudioInputPacket*audioFrame)
  {
    IDeckLinkVideoFrame*rightEyeFrame = NULL;
    IDeckLinkVideoFrame3DExtensions*    threeDExtensions = NULL;
    void*frameBytes;
    void*audioFrameBytes;

    // Handle Video Frame
    if (videoFrame) {
#if 0
      // If 3D mode is enabled we retreive the 3D extensions interface which gives.
      // us access to the right eye frame by calling GetFrameForRightEye() .
      if ( (videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions,
                                       (void **) &threeDExtensions) != S_OK) ||
           (threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK)) {
        rightEyeFrame = NULL;
      }
#endif
      if (threeDExtensions) {
        threeDExtensions->Release();
      }

      if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {
      } else {
        long w=videoFrame->GetWidth();
        long h=videoFrame->GetHeight();
        videoFrame->GetBytes(&frameBytes);
        m_priv->setFrame(videoFrame->GetWidth(),
                         videoFrame->GetHeight(),
                         GEM_YUV,
                         (unsigned char*)frameBytes);
      }

      if (rightEyeFrame) {
        rightEyeFrame->Release();
      }

      m_frameCount++;
    }

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE
  VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events,
                          IDeckLinkDisplayMode*mode,
                          BMDDetectedVideoInputFormatFlags)
  {
    // This only gets called if bmdVideoInputEnableFormatDetection was set
    // when enabling video input
    HRESULT     result;
    deckstring_t displayModeName = NULL;

    if (!(events & bmdVideoInputDisplayModeChanged)) {
      return S_OK;
    }

    mode->GetName(&displayModeName);
    free_deckstring(displayModeName);

    if (m_deckLinkInput) {
      m_deckLinkInput->StopStreams();
      result = m_deckLinkInput->EnableVideoInput(mode->GetDisplayMode(),
               m_cfg_pixelFormat, m_cfg_inputFlags);
      if (result != S_OK) {
        goto bail;
      }

      m_deckLinkInput->StartStreams();
    }

bail:
    return S_OK;
  }
};
/*
** END of Blackmagic licensed code
*/


namespace
{
IDeckLinkDisplayMode*getDisplayMode(IDeckLinkInput*dli,
                                    const std::string&formatname, int formatnum)
{
  IDeckLinkDisplayModeIterator*dmi = NULL;
  IDeckLinkDisplayMode*displayMode = NULL;
  int count=formatnum;
  if(S_OK == dli->GetDisplayModeIterator(&dmi)) {
    while(S_OK == dmi->Next(&displayMode)) {
      if (formatnum<0 && formatname.empty()) {
        // we don't care for the format; accept the first one
        break;
      }

      // if we have set the format name, check that
      if(!formatname.empty()) {
        deckstring_t dmn = NULL;
        if (S_OK == displayMode->GetName(&dmn)) {
          std::string dmns = deckstring2string(dmn);
          bool found=(formatname == dmns);
          verbose(1, "[GEM:videoDECKLINK] checking format '%s'", dmns.c_str());
          free_deckstring(dmn);
          if(found) {
            break;
          }
        }
      }
      // else check the format index
      if(formatnum>=0 && 0 == count) {
        break;
      }
      --count;

      displayMode->Release();
      displayMode=NULL;
    }
    dmi->Release();
  }
  return displayMode;
}
};




using namespace gem::plugins;

REGISTER_VIDEOFACTORY("decklink", videoDECKLINK);

videoDECKLINK::videoDECKLINK(void)
  : video()
  , m_name(std::string("decklink"))
  , m_devname   (std::string("")), m_devnum   (-1)
  , m_formatname(std::string("")), m_formatnum(-1)
  , m_dlIterator(NULL)
  , m_dl(NULL)
  , m_dlInput(NULL)
  , m_displayMode(NULL)
  , m_dlConfig(NULL)
  , m_connectionType((BMDVideoConnection)0)
  , m_pixelFormat(bmdFormat8BitYUV)
  , m_dlCallback(NULL)
{
  IDeckLinkIterator*dli = CreateDeckLinkIteratorInstance();
  if(!dli) {
    throw(GemException("DeckLink: unable to initialize Framework"));
  }
  dli->Release();

  m_pixBlock.image.xsize = 1920;
  m_pixBlock.image.ysize = 1080;
  m_pixBlock.image.setCsizeByFormat(GEM_RGBA);
  m_pixBlock.image.reallocate();
  m_pixBlock.image.xsize = -1;
  m_pixBlock.image.ysize = -1;
}

videoDECKLINK::~videoDECKLINK(void)
{
  close();
}

void videoDECKLINK::close(void)
{
  stop();
  if(m_displayMode) {
    m_displayMode->Release();
    m_displayMode=0;
  }
  if(m_dlConfig) {
    m_dlConfig->Release();
    m_dlConfig=0;
  }

  if(m_dlInput) {
    m_dlInput->DisableAudioInput();
    m_dlInput->DisableVideoInput();
    m_dlInput->Release();
    m_dlInput=0;
  }
  if(m_dlCallback) {
    m_dlCallback->Release();
    m_dlCallback=0;
  }
  if(m_dl) {
    m_dl->Release();
    m_dl=0;
  }
  if(m_dlIterator) {
    m_dlIterator->Release();
    m_dlIterator=0;
  }
}

bool videoDECKLINK::open(gem::Properties&props)
{
  BMDVideoInputFlags flags = bmdVideoInputFlagDefault;
  BMDPixelFormat pixformat = bmdFormat8BitYUV;
  std::string formatname=(("auto"==m_formatname)
                                || ("automatic" == m_formatname))?"":m_formatname;
  //if(m_devname.empty())return false;
  close();

  IDeckLinkIterator*m_dlIterator = CreateDeckLinkIteratorInstance();
  if(m_dlIterator) {
    setProperties(props);
    formatname=(("auto"==m_formatname) || ("automatic" == m_formatname))?"":m_formatname;

    if(m_devnum<0 && m_devname.empty()) {
      // TODO: automatic device detection, based on input and mode
      while (m_dlIterator->Next(&m_dl) == S_OK) {
        m_dlInput=NULL;
        if (S_OK == m_dl->QueryInterface(IID_IDeckLinkInput, (void**)&m_dlInput)) {
          // check whether this device supports the selected format
          m_displayMode=getDisplayMode(m_dlInput, formatname, m_formatnum);
          if(m_displayMode) {
            // supported!
            break;
          }
          m_dlInput->Release();
        }
        m_dlInput=NULL;
      }
    } else { // user requested device (via name or index)
      int deviceCount=0;
      while (m_dlIterator->Next(&m_dl) == S_OK) {
        if(m_devnum == deviceCount) {
          break;
        }
        if(!m_devname.empty()) {
          deckstring_t deckLinkName = NULL;
          if(S_OK == m_dl->GetDisplayName(&deckLinkName)) {
            if (m_devname == deckstring2string(deckLinkName)) {
              free_deckstring(deckLinkName);
              break;
            }
            free_deckstring(deckLinkName);
          }
          if(S_OK == m_dl->GetModelName(&deckLinkName)) {
            if (m_devname == deckstring2string(deckLinkName)) {
              free_deckstring(deckLinkName);
              break;
            }
            free_deckstring(deckLinkName);
          }
        }
        m_dl->Release();
        m_dl=NULL;
        ++deviceCount;
      }
      m_dlInput=NULL;
      if(m_dl) {
        if (S_OK == m_dl->QueryInterface(IID_IDeckLinkInput, (void**)&m_dlInput)) {
          // check whether this device supports the selected format
          m_displayMode=getDisplayMode(m_dlInput, formatname, m_formatnum);
        } else {
          m_dlInput=NULL;
        }
      }
    }
  }
  if(!m_displayMode) {
    goto bail;
  }
  if (m_formatnum<0 && formatname.empty()) {
    // no format specified; try auto-detection
    IDeckLinkAttributes*dlAttribs=0;
    deckbool_t formatDetectionSupported = false;
    if (S_OK == m_dl->QueryInterface(IID_IDeckLinkAttributes,
                                     (void**)&dlAttribs)) {
      if (S_OK == dlAttribs->GetFlag(BMDDeckLinkSupportsInputFormatDetection,
                                     &formatDetectionSupported)) {
        if(formatDetectionSupported) {
          flags|=bmdVideoInputEnableFormatDetection;
        }
      }
    }
    if(dlAttribs) {
      dlAttribs->Release();
    }
  }

  BMDDisplayModeSupport displayModeSupported;
  if (S_OK != m_dlInput->DoesSupportVideoMode(
        m_displayMode->GetDisplayMode(),
        m_pixelFormat,
        flags,
        &displayModeSupported,
        NULL)) {
    goto bail;
  }
  if (displayModeSupported == bmdDisplayModeNotSupported) {
    goto bail;
  }
  if(S_OK != m_dl->QueryInterface (IID_IDeckLinkConfiguration,
                                   (void**)&m_dlConfig)) {
    m_dlConfig=NULL;
  }

  if(m_dlConfig) {
    m_dlConfig->SetInt(bmdDeckLinkConfigVideoInputConnection,
                       m_connectionType);
  }

  m_dlCallback = new DeckLinkCaptureDelegate(this, m_dlInput);
  if(S_OK != m_dlInput->EnableVideoInput(m_displayMode->GetDisplayMode(),
                                         m_pixelFormat, flags)) {
    goto bail;
  }

  return true;

bail:
  close();
  return false;
}

bool videoDECKLINK::start(void)
{
  return (m_dlInput && (S_OK == m_dlInput->StartStreams()));
}
bool videoDECKLINK::stop(void)
{
  if(m_dlInput) {
    m_dlInput->StopStreams();
  }
  return true;
}


pixBlock*videoDECKLINK::getFrame(void)
{
  m_mutex.lock();
  return &m_pixBlock;
}
void videoDECKLINK::setFrame(unsigned int w, unsigned int h, unsigned int format,
                             unsigned char*data)
{
  m_mutex.lock();
  m_pixBlock.image.xsize=w;
  m_pixBlock.image.ysize=h;
  m_pixBlock.image.setCsizeByFormat(format);
  m_pixBlock.image.reallocate();

  m_pixBlock.image.fromUYVY(data);
  m_pixBlock.newimage=true;

  m_mutex.unlock();
}

void videoDECKLINK::releaseFrame(void)
{
  m_pixBlock.newimage=false;
  m_mutex.unlock();
}


std::vector<std::string>videoDECKLINK::enumerate(void)
{
  std::vector<std::string>result;
  IDeckLinkIterator*dli = CreateDeckLinkIteratorInstance();
  if(dli) {
    IDeckLink*deckLink = NULL;
    while (dli->Next(&deckLink) == S_OK) {
      deckstring_t deckLinkName = NULL;
      HRESULT res = deckLink->GetDisplayName(&deckLinkName);
      if (res == S_OK) {
        result.push_back(deckstring2string(deckLinkName));
        free_deckstring(deckLinkName);
      }
      deckLink->Release();
    }
    dli->Release();
  }
  return result;
}

bool videoDECKLINK::setDevice(int ID)
{
  m_devname.clear();
  m_devnum=ID;
  return true;
}
bool videoDECKLINK::setDevice(const std::string&device)
{
  m_devname=device;
  return true;
#if 0
  m_devname.clear();
  const std::string prefix="decklink://";
  if (!device.compare(0, prefix.size(), prefix)) {
    m_devname=device.substr(prefix.size());
    return true;
  }
  return false;
#endif
}
bool videoDECKLINK::enumProperties(gem::Properties&readable,
                                   gem::Properties&writeable)
{
  std::string dummy_s;
  int dummy_i=0;
  readable.clear();
  writeable.clear();

  readable.set("width", m_pixBlock.image.xsize);
  readable.set("height", m_pixBlock.image.ysize);

  dummy_s="auto";
  writeable.set("format", dummy_s);
  writeable.set("connection", dummy_s);

  return true;
}
void videoDECKLINK::setProperties(gem::Properties&props)
{
  if(trySetProperties(props, true)) {
    verbose(1, "[GEM::videoDECKLINK] needs restart");
    if(m_dlInput) {
      stop();
      start();
    }
  }
}

bool videoDECKLINK::trySetProperties(gem::Properties&props, bool canrestart)
{
  bool needrestart = false;
  std::vector<std::string>keys=props.keys();
  int i=0;
  for(i=0; i<keys.size(); i++) {
    const std::string key =keys[i];
    if(canrestart  && needrestart)
      return true;
    if("format" == key) {
      std::string s;
      double d;
      switch(props.type(key)) {
      case gem::Properties::STRING:
        if(props.get(key, s)) {
          m_formatnum =-1;
          m_formatname=s;
          needrestart = true;
        }
        break;
      case gem::Properties::DOUBLE:
        if(props.get(key, d)) {
          m_formatnum =(int)d;
          m_formatname="";
          needrestart = true;
        }
        break;
      default:  break;
      }
    }
    if("pixformat" == key) {
      BMDPixelFormat fmt = (BMDPixelFormat)0;
      std::string s;
      double d;
      switch(props.type(key)) {
      case gem::Properties::STRING:
        if(props.get(key, s)) {
          fmt = string2pixformat(s);
        }
        break;
      case gem::Properties::DOUBLE:
        if(props.get(key, d)) {
          int idx =(int)d;
          switch(idx) {
          default:
          case 0:
            fmt=bmdFormat8BitYUV;
            break;
          case 1:
            fmt=bmdFormat10BitYUV;
            break;
          case 2:
            fmt=bmdFormat8BitARGB;
            break;
          case 3:
            fmt=bmdFormat8BitBGRA;
            break;
          }
        }
        break;
      default:  break;
      }
      if (fmt) {
        m_pixelFormat = (BMDPixelFormat)fmt;
        needrestart = true;
      } else {
        // unknown pixelformat
      }
    }
    if("connection" == key) {
      BMDVideoConnection vconn = m_connectionType;
      std::string s;
      double d;
      switch(props.type(key)) {
      case gem::Properties::STRING:
        if(props.get(key, s)) {
          vconn = string2connection(s);
        }
        post("setting 'connection' to %d '%s'", vconn, s.c_str());
        break;
      case gem::Properties::DOUBLE:
        if(props.get(key, d)) {
          int idx =(int)d;
          switch(idx) {
          default:
          case 0:
            vconn=bmdVideoConnectionSDI;
            break;
          case 1:
            vconn=bmdVideoConnectionHDMI;
            break;
          case 2:
            vconn=bmdVideoConnectionOpticalSDI;
            break;
          case 3:
            vconn=bmdVideoConnectionComponent;
            break;
          case 4:
            vconn=bmdVideoConnectionComposite;
            break;
          case 5:
            vconn=bmdVideoConnectionSVideo;
            break;
          }
        }
        break;
      default:  break;
      }
      if(m_dlConfig && (m_connectionType != vconn)) {
        m_dlConfig->SetInt(bmdDeckLinkConfigVideoInputConnection, vconn);
      } else
        needrestart = true;
      m_connectionType = vconn;
    }
  }
  m_props=props;
  return needrestart;
}
void videoDECKLINK::getProperties(gem::Properties&props)
{
  std::vector<std::string>keys=props.keys();
  unsigned int i;
  for(i=0; i<keys.size(); i++) {
    if("width"==keys[i]) {
      props.set(keys[i], m_pixBlock.image.xsize);
    }
    if("height"==keys[i]) {
      props.set(keys[i], m_pixBlock.image.ysize);
    }
  }
}

std::vector<std::string>videoDECKLINK::dialogs(void)
{
  std::vector<std::string>result;
  return result;
}
bool videoDECKLINK::provides(const std::string&name)
{
  return (name==m_name);
}
std::vector<std::string>videoDECKLINK::provides(void)
{
  std::vector<std::string>result;
  result.push_back(m_name);
  return result;
}
const std::string videoDECKLINK::getName(void)
{
  return m_name;
}


#ifdef GEM_DECKLINK_USE_DLOPEN
# include <DeckLinkAPIDispatch.cpp>
#endif
