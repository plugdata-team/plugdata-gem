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

#include "recordV4L2.h"

#include "Gem/Manager.h"
#include "Gem/Exception.h"

// for logpost(0, 3+):
#include "Gem/RTE.h"

#include "plugins/PluginFactory.h"

using namespace gem::plugins;


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

REGISTER_RECORDFACTORY("v4l2", recordV4L2);
/////////////////////////////////////////////////////////
//
// recordV4L2
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

recordV4L2 :: recordV4L2(void):
  m_fd(-1),
  m_init(false),
  m_palette(0)
{
  m_image.xsize=720;
  m_image.ysize=576;
  m_image.setFormat(GEM_YUV);
  //m_image.setFormat(GEM_RGBA); /* RGBA works with Gem, but not with GStreamer and xawtv */
  m_image.reallocate();

  switch(m_image.format) {
  case GEM_YUV:
    m_palette = V4L2_PIX_FMT_UYVY;
    break;
  case GEM_GRAY:
    m_palette = V4L2_PIX_FMT_GREY;
    break;
  case GEM_RGBA:
    m_palette = V4L2_PIX_FMT_RGB32;
    break;
  default:
    throw(GemException("invalid colorspace"));
  }


}

////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
recordV4L2 :: ~recordV4L2(void)
{
  stop();
}

void recordV4L2 :: stop(void)
{
  if(m_fd>=0) {
    ::close(m_fd);
  }
  m_fd=-1;

}

bool recordV4L2 :: start(const std::string&filename, gem::Properties&props)
{
  stop();
  m_fd=::open(filename.c_str(), O_RDWR);
  if(m_fd<0) {
    return false;
  }
  struct v4l2_capability vid_caps;


  if(ioctl(m_fd, VIDIOC_QUERYCAP, &vid_caps) == -1) {
    perror("[GEM:recordV4L2] VIDIOC_QUERYCAP");
    stop();
    return false;
  }
  if( !(vid_caps.capabilities & V4L2_CAP_VIDEO_OUTPUT) ) {
    pd_error(0, "[GEM:recordV4L2] device '%s' is not a video4linux2 output device",
             filename.c_str());
    stop();
    return false;
  }
  m_init=false;
  return true;
}

bool recordV4L2::init(const imageStruct* dummyImage, const int framedur)
{
  if(m_init) {
    return true;
  }
  if(m_fd<0) {
    return false;
  }

  unsigned int w=dummyImage->xsize;
  unsigned int h=dummyImage->ysize;

  struct v4l2_capability vid_caps;
  if(ioctl(m_fd, VIDIOC_QUERYCAP, &vid_caps) == -1) {
    perror("[GEM:recordV4L2] VIDIOC_QUERYCAP");
    stop();
    return false;
  }
  struct v4l2_format vid_format;

  memset(&vid_format, 0, sizeof(vid_format));

  vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  vid_format.fmt.pix.width = w;
  vid_format.fmt.pix.height = h;
  vid_format.fmt.pix.pixelformat = m_palette;
  vid_format.fmt.pix.sizeimage = w * h * m_image.csize;
  vid_format.fmt.pix.field = V4L2_FIELD_NONE;
  vid_format.fmt.pix.bytesperline = w * m_image.csize;
  //vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
  vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

  int format= vid_format.fmt.pix.pixelformat;
  logpost(0, 3+1, "[GEM:recordV4L2] v4l2-output requested %dx%d @ '%c%c%c%c'",
          vid_format.fmt.pix.width, vid_format.fmt.pix.height,
          (char)(format),
          (char)(format>>8),
          (char)(format>>16),
          (char)(format>>24));
  if(ioctl(m_fd, VIDIOC_S_FMT, &vid_format) == -1) {
    perror("[GEM:recordV4L2] VIDIOC_S_FMT");
    stop();
    return false;
  }

  logpost(0, 3+1, "[GEM:recordV4L2] v4l2-output returned %dx%d @ '%c%c%c%c'",
          vid_format.fmt.pix.width, vid_format.fmt.pix.height,
          (char)(format),
          (char)(format>>8),
          (char)(format>>16),
          (char)(format>>24));
  /* if the driver returns a format other than requested we should adjust! */
  w=vid_format.fmt.pix.width;
  h=vid_format.fmt.pix.height;

  m_image.xsize=w;
  m_image.ysize=h;
  m_image.reallocate();

  ::write(m_fd, m_image.data, m_image.xsize*m_image.ysize*m_image.csize);

  m_init=true;
  return true;
}



/////////////////////////////////////////////////////////
// do the actual encoding and writing to file
//
/////////////////////////////////////////////////////////
bool recordV4L2 :: write(imageStruct*img)
{
  if(!m_init) {
    if(!init(img, 0)) {
      return true;
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


static const std::string s_codec_name=std::string("v4l2");
static const std::string s_codec_desc=std::string("v4l2 loopback device");



/////////////////////////////////////////////////////////
// set codec by name
//
/////////////////////////////////////////////////////////
bool recordV4L2 :: setCodec(const std::string&name)
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
std::vector<std::string>recordV4L2::getCodecs(void)
{
  std::vector<std::string>result;
  result.push_back(s_codec_name);
  return result;
}
const std::string recordV4L2::getCodecDescription(const std::string&codec)
{
  std::string result;
  if(codec==s_codec_name) {
    result=s_codec_desc;
  }
  return result;
}

bool recordV4L2::enumProperties(gem::Properties&props)
{
  props.clear();
  return false;
}
