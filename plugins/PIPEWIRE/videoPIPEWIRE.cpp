////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 2022 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "LICENSE.txt"
//
/////////////////////////////////////////////////////////
#include "videoPIPEWIRE.h"
#include "plugins/PluginFactory.h"

#include <spa/param/video/format-utils.h>
#include <spa/debug/types.h>

#include "m_pd.h"

#ifdef __BIG_ENDIAN__
# define GEM_SPA_GRAY16 SPA_VIDEO_FORMAT_GRAY16_BE
#else
# define GEM_SPA_GRAY16 SPA_VIDEO_FORMAT_GRAY16_LE
#endif

using namespace gem::plugins;

REGISTER_VIDEOFACTORY("pipewire", videoPIPEWIRE);

namespace
{
static struct pw_thread_loop *s_loop = NULL;
static unsigned int s_loopcount = 0;

bool videoPIPEWIRE_init(void)
{
  if(s_loop) {
    s_loopcount++;
    return false;
  }
  pw_init(0, 0);
  s_loop = pw_thread_loop_new("pd-gem", NULL);
  if(!s_loop) {
    pw_deinit();
    return false;
  }
  pw_thread_loop_start(s_loop);
  //::post("lop %p started", s_loop);
  s_loopcount = 1;
  return true;
}
void videoPIPEWIRE_deinit(void)
{
  if(!s_loopcount) {
    return;
  }
  if(--s_loopcount) {
    return;
  }
  pw_thread_loop_stop(s_loop);
  pw_thread_loop_destroy(s_loop);
  s_loop=0;
  pw_deinit();
}
}


videoPIPEWIRE::videoPIPEWIRE(void)
  : video()
  , m_name(std::string("test"))
  , m_stream(0)
  , m_format(SPA_VIDEO_FORMAT_UNKNOWN)

{
  m_pixBlock.image.xsize = 1;
  m_pixBlock.image.ysize = 1;
  m_pixBlock.image.setCsizeByFormat(GEM_RGBA);
  m_pixBlock.image.reallocate();
  m_pixBlock.image.setBlack();
  videoPIPEWIRE_init();
  m_stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = param_changed_cb,
    .process = process_cb,
  };
}

videoPIPEWIRE::~videoPIPEWIRE(void)
{
  videoPIPEWIRE_deinit();
}

bool videoPIPEWIRE::open(gem::Properties&props)
{
  if(!s_loop)return false;
  struct pw_properties *pwprops = pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                                                    PW_KEY_MEDIA_CATEGORY, "Capture",
                                                    PW_KEY_MEDIA_ROLE, "Camera",
                                                    PW_KEY_APP_NAME, "Pd",
                                                    PW_KEY_APP_ID, "info.puredata.gem",
                                                    PW_KEY_NODE_NAME, "Gem",
                                                    NULL);

  uint8_t buffer[1024];
  const struct spa_pod *params[1];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
  struct spa_rectangle minsize = SPA_RECTANGLE(320, 240);
  struct spa_rectangle maxsize = SPA_RECTANGLE(4096, 4096);
  struct spa_rectangle stpsize = SPA_RECTANGLE(1,1);
  struct spa_fraction minrate  = SPA_FRACTION(25, 1);
  struct spa_fraction maxrate  = SPA_FRACTION(1000, 1);
  struct spa_fraction stprate  = SPA_FRACTION(0, 1);
  //int flags = PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS;
  int flags = PW_STREAM_FLAG_MAP_BUFFERS;

  pw_thread_loop_lock(s_loop);
  m_stream = pw_stream_new_simple(
               pw_thread_loop_get_loop(s_loop),
               "video-capture",
               pwprops,
               &m_stream_events,
               this);
  if(!m_stream) {
    goto err;
  }

  params[0] = (const spa_pod*)spa_pod_builder_add_object(&b,
                                         SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
                                         SPA_FORMAT_mediaType,       SPA_POD_Id(SPA_MEDIA_TYPE_video),
                                         SPA_FORMAT_mediaSubtype,    SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                                         SPA_FORMAT_VIDEO_format,    SPA_POD_CHOICE_ENUM_Id(
                                           9,
                                           SPA_VIDEO_FORMAT_RGB,
                                           SPA_VIDEO_FORMAT_RGBA,
                                           SPA_VIDEO_FORMAT_BGR,
                                           SPA_VIDEO_FORMAT_BGRA,
                                           SPA_VIDEO_FORMAT_RGB16,
                                           SPA_VIDEO_FORMAT_YUY2,
                                           SPA_VIDEO_FORMAT_UYVY,
                                           SPA_VIDEO_FORMAT_GRAY8,
                                           GEM_SPA_GRAY16),
                                         SPA_FORMAT_VIDEO_size,      SPA_POD_CHOICE_RANGE_Rectangle(
                                           &minsize, &stpsize, &maxsize),
                                         SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(
                                           &minrate, &stprate, &maxrate));

  pw_stream_connect(m_stream,
                    PW_DIRECTION_INPUT,
                    PW_ID_ANY,
                    (pw_stream_flags)flags,
                    params, 1);
  setProperties(props);
  pw_thread_loop_unlock(s_loop);
  return (true);
err:
  pw_thread_loop_unlock(s_loop);
  return false;
}

bool videoPIPEWIRE::start()
{
  if(!m_stream) {
    return (false);
  }
  struct timespec abstime;
  const char*error=0;
  pw_thread_loop_lock (s_loop);
  pw_stream_set_active(m_stream, true);
  pw_thread_loop_get_time (s_loop, &abstime,
                           3 * SPA_NSEC_PER_SEC);
  while (false) {
    enum pw_stream_state state;
    state = pw_stream_get_state (m_stream, &error);
    if (state >= PW_STREAM_STATE_PAUSED) {
      break;
    }

    if (state == PW_STREAM_STATE_ERROR) {
      goto start_error;
    }

    if (pw_thread_loop_timed_wait_full (s_loop, &abstime) < 0) {
      error = "timeout";
      goto start_error;
    }
  }
  pw_thread_loop_signal (s_loop, false);
  pw_thread_loop_unlock (s_loop);
  return (true);
start_error: {
    ::pd_error(0, "ERROR: %s", error);
    pw_thread_loop_unlock (s_loop);
    return false;
  }
}
bool videoPIPEWIRE::stop()
{
  if(!m_stream) {
    return (false);
  }
  pw_stream_set_active(m_stream, false);
  return (true);
}



pixBlock*videoPIPEWIRE::getFrame(void)
{
  m_mutex.lock();
  if(!m_pixBlock.image.csize) {
    m_mutex.unlock();
    return 0;
  }
  return &m_pixBlock;

  m_pixBlock.image.setCsizeByFormat(GEM_RGBA);
  m_pixBlock.image.reallocate();
  const unsigned int count = m_pixBlock.image.xsize * m_pixBlock.image.ysize;
  unsigned int i=0;
  unsigned char*data=m_pixBlock.image.data;
  //::post("%s", __FUNCTION__);
  m_pixBlock.newimage = true;
  pw_thread_loop_lock (s_loop);
  pw_thread_loop_signal (s_loop, false);
  pw_thread_loop_unlock (s_loop);

  return &m_pixBlock;
}
void videoPIPEWIRE::releaseFrame(void)
{
  m_pixBlock.newimage = false;
  m_mutex.unlock();
}

std::vector<std::string>videoPIPEWIRE::enumerate(void)
{
  std::vector<std::string>result;
  result.push_back("pipewire");
  return result;
}

bool videoPIPEWIRE::setDevice(int ID)
{
  return true;
}
bool videoPIPEWIRE::setDevice(const std::string&device)
{
  return ("pipewire"==device);
}
bool videoPIPEWIRE::enumProperties(gem::Properties&readable,
                                   gem::Properties&writeable)
{
  readable.clear();
  writeable.clear();


  writeable.set("width", 64);
  readable.set("width", 64);
  writeable.set("height", 64);
  readable.set("height", 64);

  //writeable.set("type", std::string("noise"));
  return true;
}
void videoPIPEWIRE::setProperties(gem::Properties&props)
{
  m_props=props;

  double d;
  if(props.get("width", d)) {
    if(d>0) {
      m_pixBlock.image.xsize = d;
    }
  }
  if(props.get("height", d)) {
    if(d>0) {
      m_pixBlock.image.ysize = d;
    }
  }
}
void videoPIPEWIRE::getProperties(gem::Properties&props)
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

std::vector<std::string>videoPIPEWIRE::dialogs(void)
{
  std::vector<std::string>result;
  return result;
}
bool videoPIPEWIRE::provides(const std::string&name)
{
  return (name==m_name);
}
std::vector<std::string>videoPIPEWIRE::provides(void)
{
  std::vector<std::string>result;
  result.push_back(m_name);
  return result;
}
const std::string videoPIPEWIRE::getName(void)
{
  return m_name;
}

void videoPIPEWIRE::on_process(void)
{
  //pw_thread_loop_signal (s_loop, false);
  struct pw_buffer *b;
  struct spa_buffer *buf;

  if ((b = pw_stream_dequeue_buffer(m_stream)) == NULL) {
    pw_log_warn("out of buffers: %m");
    return;
  }

  buf = b->buffer;
  if (buf->datas[0].data == NULL) {
    return;
  }

  m_mutex.lock();
  switch(m_format) {
  case SPA_VIDEO_FORMAT_RGB:
    m_pixBlock.image.fromRGB((unsigned char*)buf->datas[0].data);
    m_pixBlock.image.notowned = false;
    break;
  case SPA_VIDEO_FORMAT_BGR:
    m_pixBlock.image.fromBGR((unsigned char*)buf->datas[0].data);
    m_pixBlock.image.notowned = false;
    break;
  case SPA_VIDEO_FORMAT_BGRA:
    m_pixBlock.image.fromBGRA((unsigned char*)buf->datas[0].data);
    m_pixBlock.image.notowned = false;
    break;
  case SPA_VIDEO_FORMAT_RGB16:
    m_pixBlock.image.fromRGB16((unsigned char*)buf->datas[0].data);
    m_pixBlock.image.notowned = false;
    break;
  case SPA_VIDEO_FORMAT_YUY2:
    m_pixBlock.image.fromYUY2((unsigned char*)buf->datas[0].data);
    m_pixBlock.image.notowned = false;
    break;
  case GEM_SPA_GRAY16:
    m_pixBlock.image.fromGray((short*)buf->datas[0].data);
    m_pixBlock.image.notowned = false;
    break;
  default:
    m_pixBlock.image.data = (unsigned char*)buf->datas[0].data;
    m_pixBlock.image.notowned = true;
  }

  m_pixBlock.newimage = true;
  m_mutex.unlock();

  pw_stream_queue_buffer(m_stream, b);
}

void videoPIPEWIRE::on_param_changed(uint32_t id, const struct spa_pod *param)
{
  if (param == NULL || id != SPA_PARAM_Format) {
    return;
  }
  struct spa_video_info format;

  if (spa_format_parse(param,
                       &format.media_type,
                       &format.media_subtype) < 0) {
    return;
  }
  if (format.media_type != SPA_MEDIA_TYPE_video ||
      format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
    return;
  }

  if (spa_format_video_raw_parse(param, &format.info.raw) < 0) {
    return;
  }


  m_mutex.lock();
  m_pixBlock.newfilm = true;
  m_pixBlock.image.xsize = format.info.raw.size.width;
  m_pixBlock.image.ysize = format.info.raw.size.height;
  m_format = format.info.raw.format;
  switch(format.info.raw.format) {
  case(SPA_VIDEO_FORMAT_RGB):
    m_pixBlock.image.setCsizeByFormat(GEM_RGBA);
    break;
  case(SPA_VIDEO_FORMAT_RGBA):
    m_pixBlock.image.setCsizeByFormat(GEM_RGBA);
    break;
  case(SPA_VIDEO_FORMAT_YUY2):
    m_pixBlock.image.setCsizeByFormat(GEM_YUV);
    break;
  case(SPA_VIDEO_FORMAT_UYVY):
    m_pixBlock.image.setCsizeByFormat(GEM_YUV);
    break;
  case(SPA_VIDEO_FORMAT_GRAY8):
    m_pixBlock.image.setCsizeByFormat(GEM_GRAY);
    break;
    m_pixBlock.image.csize = 0;
  default:
    break;
  }
  m_mutex.unlock();
  //return;

  ::post("got video format:");
  ::post("  format: %d (%s)", format.info.raw.format,
         spa_debug_type_find_name(spa_type_video_format,
                                  format.info.raw.format));
  ::post("  size: %dx%d", format.info.raw.size.width,
         format.info.raw.size.height);
  ::post("  framerate: %d/%d", format.info.raw.framerate.num,
         format.info.raw.framerate.denom);

}

void videoPIPEWIRE::process_cb(void*data)
{
  //::pd_error(0, "process");
  ((videoPIPEWIRE*)data)->on_process();
}
void videoPIPEWIRE::param_changed_cb(void*data, uint32_t id, const struct spa_pod *param)
{
  //::pd_error(0, "param_changed");
  ((videoPIPEWIRE*)data)->on_param_changed(id, param);
}