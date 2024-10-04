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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "recordV4L.h"

#ifdef  HAVE_VIDEO4LINUX

#include "Gem/Manager.h"
#include "plugins/PluginFactory.h"

using namespace gem::plugins;

#include "Gem/Exception.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

REGISTER_RECORDFACTORY("v4l", recordV4L);
/////////////////////////////////////////////////////////
//
// recordV4L
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

recordV4L :: recordV4L(void):
  m_fd(-1),
  m_init(false),
  m_palette(0)
{
  m_image.xsize=720;
  m_image.ysize=576;
  m_image.setFormat(GEM_YUV);
  m_image.setFormat(GEM_RGBA);
  m_image.reallocate();

  switch(m_image.format) {
  case GEM_YUV:
    m_palette = VIDEO_PALETTE_YUV422;
    break;
  case GEM_GRAY:
    m_palette = VIDEO_PALETTE_GREY;
    break;
  case GEM_RGBA:
    m_palette = VIDEO_PALETTE_RGB32;
    break;
  default:
    throw(GemException("invalid colorspace"));
  }


}

////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
recordV4L :: ~recordV4L(void)
{
  stop();
}

void recordV4L :: stop(void)
{
  if(m_fd>=0) {
    ::close(m_fd);
  }
  m_fd=-1;

}

bool recordV4L :: start(const std::string&filename, gem::Properties&props)
{
  stop();

  m_fd=::open(filename.c_str(), O_RDWR);
  if(m_fd<0) {
    return false;
  }

  struct video_picture vid_pic;
  if (ioctl(m_fd, VIDIOCGPICT, &vid_pic) == -1) {
    perror("[GEM:recordV4L] VIDIOCGPICT");
    stop();
    return false;
  }
  vid_pic.palette = m_palette;
  if (ioctl(m_fd, VIDIOCSPICT, &vid_pic) == -1) {
    perror("[GEM:recordV4L] VIDIOCSPICT");
    stop();
    return false;
  }

  struct video_window vid_win;
  if (ioctl(m_fd, VIDIOCGWIN, &vid_win) == -1) {
    perror("[GEM:recordV4L] (VIDIOCGWIN)");
    stop();
    return false;
  }

  m_init=false;
  return true;
}

bool recordV4L::init(const imageStruct* dummyImage, const int framedur)
{
  if(m_init) {
    return true;
  }
  if(m_fd<0) {
    return false;
  }

  unsigned int w=dummyImage->xsize;
  unsigned int h=dummyImage->ysize;

  struct video_picture vid_pic;
  struct video_window vid_win;

  if (ioctl(m_fd, VIDIOCGPICT, &vid_pic) == -1) {
    perror("[GEM:recordV4L] VIDIOCGPICT");
    stop();
    return false;
  }

  vid_pic.palette = m_palette;

  if (ioctl(m_fd, VIDIOCSPICT, &vid_pic) == -1) {
    perror("[GEM:recordV4L] VIDIOCSPICT");
    stop();
    return false;
  }

  if (ioctl(m_fd, VIDIOCGWIN, &vid_win) == -1) {
    perror("[GEM:recordV4L] ioctl (VIDIOCGWIN)");
    stop();
    return false;
  }

  vid_win.width  = w;
  vid_win.height = h;
  if (ioctl(m_fd, VIDIOCSWIN, &vid_win) == -1) {
    perror("[GEM:recordV4L] ioctl (VIDIOCSWIN)");
    stop();
    return false;
  }

  m_image.xsize=w;
  m_image.ysize=h;
  m_image.reallocate();

  m_init=true;
  return true;
}



/////////////////////////////////////////////////////////
// do the actual encoding and writing to file
//
/////////////////////////////////////////////////////////
bool recordV4L :: write(imageStruct*img)
{
  if(!m_init) {
    if(!init(img, 0)) {
      return false;
    }
  }
  m_image.setFormat();
  m_image.convertFrom(img);

  //  m_image.upsidedown=!m_image.upsidedown;
  m_image.fixUpDown();

  int size=m_image.xsize*m_image.ysize*m_image.csize;

  ::write(m_fd, m_image.data, size);

  return true;
}

/////////////////////////////////////////////////////////
// get number of codecs
//
/////////////////////////////////////////////////////////

static const std::string s_codec_name=std::string("v4l");
static const std::string s_codec_desc=
  std::string("v4l(1) loopback device");

/////////////////////////////////////////////////////////
// set codec by name
//
/////////////////////////////////////////////////////////
bool recordV4L :: setCodec(const std::string&name)
{
  if(name==s_codec_name) {
    return true;
  }

  return false;
}

/////////////////////////////////////////////////////////
// get codecs
//
/////////////////////////////////////////////////////////
std::vector<std::string>recordV4L::getCodecs(void)
{
  std::vector<std::string>result;
  result.push_back(s_codec_name);
  return result;
}
const std::string recordV4L::getCodecDescription(const std::string&codec)
{
  std::string result;
  if(codec==s_codec_name) {
    result=s_codec_desc;
  }
  return result;
}

bool recordV4L::enumProperties(gem::Properties&props)
{
  props.clear();
  return false;
}

#endif /* V4L */

#if 0
/* handler for ioctls from the client */
static int v4l_ioctlhandler(unsigned long int cmd, void *arg)
{
  switch(cmd) {

  /* capabilities */
  case VIDIOCGCAP: {
    struct video_capability *vidcap = arg;

    snprintf(vidcap->name, 32, "Gem vloopback output");
    vidcap->type = VID_TYPE_CAPTURE;
    vidcap->channels = 1;
    vidcap->audios = 0;
    vidcap->maxwidth = MAX_WIDTH;
    vidcap->maxheight = MAX_HEIGHT;
    vidcap->minwidth = MIN_WIDTH;
    vidcap->minheight = MIN_HEIGHT;
    return 0;
  }

  /* channel info (rather limited */
  case VIDIOCGCHAN:     {
    struct video_channel *vidchan = arg;

    if(vidchan->channel != 0) {
      return EINVAL;
    }
    vidchan->flags = 0;
    vidchan->tuners = 0;
    vidchan->type = VIDEO_TYPE_CAMERA;
    strncpy(vidchan->name, "Gem", 32);
    return 0;
  }
  /* setting channel (we only have one) */
  case VIDIOCSCHAN:     {
    int *v = arg;
    if(v[0] != 0) {
      return EINVAL;
    }
    return 0;
  }

  /* getting picture properties */
  case VIDIOCGPICT:     {
    struct video_picture *vidpic = arg;

    vidpic->colour = 0xffff;
    vidpic->hue = 0xffff;
    vidpic->brightness = 0xffff;
    vidpic->contrast = 0xffff;
    vidpic->whiteness = 0xffff;
    vidpic->depth = pixel_depth;
    vidpic->palette = pixel_format;
    return 0;
  }

  /* setting picture properties: we just ignore this */
  /* LATER: don't we have to set the vidpic to the "real" values? */
  case VIDIOCSPICT:     {
    struct video_picture *vidpic = arg;

    return 0;
  }

  case VIDIOCGWIN:      {
    struct video_window *vidwin = arg;

    vidwin->x = 0;
    vidwin->y = 0;
    vidwin->width = vloopback_width;
    vidwin->height = vloopback_height;
    vidwin->chromakey = 0;
    vidwin->flags = 0;
    vidwin->clipcount = 0;
  }

  case VIDIOCSWIN: {
    struct video_window *vidwin = arg;

    if(vidwin->width > MAX_WIDTH || vidwin->height > MAX_HEIGHT) {
      return EINVAL;
    }
    if(vidwin->width < MIN_WIDTH || vidwin->height < MIN_HEIGHT) {
      return EINVAL;
    }
    if(vidwin->flags) {
      return EINVAL;
    }
    vloopback_width = vidwin->width;
    vloopback_height = vidwin->height;
    return 0;
  }

  case VIDIOCSYNC: {
    int frame = *(int *)arg;
    int ret = 0;

    if(frame < 0 || frame > 1) {
      return EINVAL;
    }
    gbuf_lock();
    switch(gbufstat[frame]) {
    case GBUFFER_UNUSED:
      ret = 0;
      break;
    case GBUFFER_GRABBING:
      while(gbufstat[frame] == GBUFFER_GRABBING) {
        pthread_cond_wait(&gbufcond, &gbufmutex);
      }
      if(gbufstat[frame] == GBUFFER_DONE) {
        gbufstat[frame] = GBUFFER_UNUSED;
        ret = 0;
      } else {
        gbufstat[frame] = GBUFFER_UNUSED;
        ret = EIO;
      }
      break;
    case GBUFFER_DONE:
      gbufstat[frame] = GBUFFER_UNUSED;
      ret = 0;
      break;
    case GBUFFER_ERROR:
    default:
      gbufstat[frame] = GBUFFER_UNUSED;
      ret = EIO;
      break;
    }
    gbuf_unlock();
    return ret;
  }

  case VIDIOCMCAPTURE: {
    struct video_mmap *vidmmap = arg;

    if(vidmmap->width > MAX_WIDTH || vidmmap->height > MAX_HEIGHT) {
      fprintf(stderr,
              "[GEM:recordV4L] requested capture size is too big(%dx%d).\n",
              vidmmap->width, vidmmap->height);
      return EINVAL;
    }
    if(vidmmap->width < MIN_WIDTH || vidmmap->height < MIN_HEIGHT) {
      fprintf(stderr,
              "[GEM:recordV4L] requested capture size is to small(%dx%d).\n",
              vidmmap->width, vidmmap->height);
      return EINVAL;
    }
    if(vidmmap->format != pixel_format) {
      converter = palette_get_supported_converter_fromRGB32(vidmmap->format);
      if(converter == NULL) {
        fprintf(stderr,
                "[GEM:recordV4L] unsupported pixel format(%d) is requested.\n",
                vidmmap->format);
        return EINVAL;
      }
      pixel_format = vidmmap->format;
    }
    if(vidmmap->frame > 1 || vidmmap->frame < 0) {
      return EINVAL;
    }
    vloopback_width = vidmmap->width;
    vloopback_height = vidmmap->height;

    if(gbufstat[vidmmap->frame] != GBUFFER_UNUSED) {
      if(gbufqueue[0] == vidmmap->frame || gbufqueue[1] == vidmmap->frame) {
        return EBUSY;
      }
    }
    gbuf_lock();
    gbufstat[vidmmap->frame] = GBUFFER_GRABBING;
    if(gbufqueue[0] < 0) {
      gbufqueue[0] = vidmmap->frame;
    } else  {
      gbufqueue[1] = vidmmap->frame;
    }
    gbuf_unlock();
    return 0;
  }

  case VIDIOCGMBUF:     {
    struct video_mbuf *vidmbuf = arg;

    vidmbuf->size = max_gbufsize;
    vidmbuf->frames = 2;
    vidmbuf->offsets[0] = 0;
    vidmbuf->offsets[1] = max_framesize;
    return 0;
  }

  default: {
    return EPERM;
  }
  }
}

static int signal_loop_init(void)
{
  outputfd = open(output_devname, O_RDWR);
  if(outputfd < 0) {
    fprintf(stderr, "[GEM:recordV4L] couldn't open output device file %s\n",
            output_devname);
    return -1;
  }
  pixel_format = VIDEO_PALETTE_RGB32;
  pixel_depth = 4;
  converter = palette_get_supported_converter_fromRGB32(pixel_format);
  vloopback_width = screen_width;
  vloopback_height = screen_height;
  max_framesize = vloopback_width * vloopback_height * sizeof(RGB32);
  max_gbufsize = max_framesize * 2;
  gbuf = vloopback_mmap(outputfd, max_gbufsize);
  gbuf_clear();
  return 0;
}

static void *signal_loop(void *arg)
{
  int signo;
  int size, ret;
  struct pollfd ufds;
  sigset_t sigset;
  unsigned long int cmd;

  if(signal_loop_init()) {
    signal_loop_initialized = -1;
    return NULL;
  }
  signal_loop_initialized = 1;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGIO);
  while(signal_loop_thread_flag) {
    sigwait(&sigset, &signo);
    if(signo != SIGIO) {
      continue;
    }
    ufds.fd = outputfd;
    ufds.events = POLLIN;
    ufds.revents = 0;
    poll(&ufds, 1, 1000);
    if(!ufds.revents & POLLIN) {
      fprintf(stderr,
              "[GEM:recordV4L] received signal but got negative on poll.\n");
      continue;
    }
    size = read(outputfd, ioctlbuf, MAXIOCTL);
    if(size >= sizeof(unsigned long int)) {
      memcpy(&cmd, ioctlbuf, sizeof(unsigned long int));
      if(cmd == 0) {
        fprintf(stderr, "[GEM:recordV4L] client closed device.\n");
        gbuf_lock();
        gbuf_clear();
        gbuf_unlock();
        continue;
      }
      ret = v4l_ioctlhandler(cmd, ioctlbuf+sizeof(unsigned long int));
      if(ret) {
        /* new vloopback patch supports a way to return EINVAL to
         * a client. */
        memset(ioctlbuf+sizeof(unsigned long int), 0xff,
               MAXIOCTL-sizeof(unsigned long int));
        fprintf(stderr, "[GEM:recordV4L] ioctl %lx unsuccessfully handled.\n",
                cmd);
        ioctl(outputfd, VIDIOCSINVALID);
      }
      if(ioctl(outputfd, cmd, ioctlbuf+sizeof(unsigned long int))) {
        fprintf(stderr, "[GEM:recordV4L] ioctl %lx unsuccessful.\n", cmd);
      }
    }
  }
  return NULL;
}
#endif
