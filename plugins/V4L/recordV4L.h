/* -----------------------------------------------------------------

GEM - Graphics Environment for Multimedia

Load an digital video (like AVI, Mpeg, Quicktime) into a pix block
(OS-independent parent-class)

Copyright (c) 1997-1999 Mark Danks. mark@danks.org
Copyright (c) Günther Geiger. geiger@epy.co.at
Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef _INCLUDE_GEMPLUGIN__RECORDV4L_RECORDV4L_H_
#define _INCLUDE_GEMPLUGIN__RECORDV4L_RECORDV4L_H_

#include "plugins/record.h"

#if defined HAVE_LIBV4L1 || defined HAVE_LINUX_VIDEODEV_H
# define HAVE_VIDEO4LINUX
#endif

#if defined HAVE_LINUX_VIDEODEV_H
# include <linux/videodev.h>
#endif /*  HAVE_LINUX_VIDEODEV_H */

#if defined HAVE_LIBV4L1
#  include <libv4l1.h>
#endif /* HAVE_LIBV4L1 */


/*---------------------------------------------------------------
 -------------------------------------------------------------------
  CLASS
  recordV4L

  class for outputting video using a v4l loopback device

  KEYWORDS
  pix record movie

  DESCRIPTION

  -----------------------------------------------------------------*/
#if defined HAVE_VIDEO4LINUX

namespace gem
{
namespace plugins
{
class GEM_EXPORT recordV4L : public record
{
public:

  //////////
  // Constructor

  recordV4L(void);

  ////////
  // Destructor
  /* free what is appropriate */
  virtual ~recordV4L(void);


  //////////
  // close the movie file
  // stop recording, close the file and clean up temporary things
  virtual void stop(void);

  //////////
  // open a movie up
  // open the recordV4L "filename" (think better about URIs ?)
  // returns TRUE if opening was successful, FALSE otherwise
  virtual bool start(const std::string&filename, gem::Properties&);


  //////////
  // initialize the encoder
  // dummyImage provides meta-information (e.g. size) that must not change during the encoding cycle
  // (if it does, abort the recording session)
  // framedur is the duration of one frame in [ms]
  //
  // returns TRUE if init was successful, FALSE otherwise
  bool init(const imageStruct* dummyImage, const int framedur);

  //////////
  // compress and write the next frame
  /* this is the core-function of this class !!!!
   * when called it returns something depending on success
   * (what? the framenumber and -1 (0?) on failure?)
   */
  virtual bool write(imageStruct*);

  virtual bool setCodec(const std::string&);

  /**
   * get a list of supported codecs (short-form names, e.g. "mjpa")
   */
  virtual std::vector<std::string>getCodecs(void);
  virtual const std::string getCodecDescription(const std::string&);
  virtual bool enumProperties(gem::Properties&);

  virtual bool dialog(void)
  {
    return false;
  }

private:
  int m_fd;
  imageStruct m_image;
  bool m_init;
  int m_palette;
};
};
};
#endif /* V4L */

#endif  // for header file
