/*-----------------------------------------------------------------

GEM - Graphics Environment for Multimedia

Use Sean T. Barret's single-file header-lib to load images

Copyright (c) 2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.


-----------------------------------------------------------------*/

#ifndef _INCLUDE_GEMPLUGIN__IMAGESTB_IMAGESTB_H_
#define _INCLUDE_GEMPLUGIN__IMAGESTB_IMAGESTB_H_
#include "plugins/imageloader.h"
#include "plugins/imagesaver.h"
#include <stdio.h>

/*-----------------------------------------------------------------
  -------------------------------------------------------------------
  CLASS
  imageSTB

  Loads in a picture

  KEYWORDS
  pix

  DESCRIPTION

  -----------------------------------------------------------------*/
namespace gem
{
namespace plugins
{

class GEM_EXPORT imageSTBLoader :   public gem::plugins::imageloader
{
public:
  imageSTBLoader(void) = default;
  virtual ~imageSTBLoader(void) = default;

  //////////
  // read an image
  virtual bool load(std::string filename, imageStruct&result,
                    gem::Properties&props);
  //////////

  virtual bool isThreadable(void)
  {
    return true;
  }
};

class GEM_EXPORT imageSTBSaver :   public gem::plugins::imagesaver
{
public:

  //////////
  // Constructor
  imageSTBSaver(void) = default;
  virtual ~imageSTBSaver(void) = default;

  // write an image
  virtual bool          save(const imageStruct&img,
                             const std::string&filename, const std::string&mimetype,
                             const gem::Properties&props);
  //////////
  // estimate, how well we could save this image
  virtual float estimateSave(const imageStruct&img,
                             const std::string&filename, const std::string&mimetype,
                             const gem::Properties&props);


  // this is always threadable
  virtual bool isThreadable(void)
  {
    return true;
  }
  virtual void getWriteCapabilities(std::vector<std::string>&mimetypes,
                                    gem::Properties&props);


};
};
};

#endif  // for header file
