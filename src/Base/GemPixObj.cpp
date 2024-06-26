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

#include "GemPixObj.h"
#include "Gem/Cache.h"
#include "Gem/State.h"
#include "Gem/Rectangle.h"
#include "Utils/Functions.h"

/////////////////////////////////////////////////////////
//
// GemPixObj
//
/////////////////////////////////////////////////////////
GemPixObj :: GemPixObj() :
  cachedPixBlock(pixBlock()),
  orgPixBlock(NULL), m_processOnOff(1),
  m_simd(GemSIMD::getCPU()),
  m_doROI(false)
{
  cachedPixBlock.newimage=0;
  cachedPixBlock.newfilm =0;
}


/////////////////////////////////////////////////////////
// setPixModified
//
/////////////////////////////////////////////////////////
void GemPixObj :: setPixModified()
{
  if(m_cache && m_cache->m_magic!=GEMCACHE_MAGIC) {
    m_cache=NULL;
  }
  if(m_cache) {
    m_cache->resendImage = 1;
  }
}

/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void GemPixObj :: render(GemState *state)
{
  // We save all information of our image, so the pix-processors can do what they want:
  //   change formats, sizes, databuffer, whatever
  // the data is restored in the <postrender> call,
  // so that the objects can rely on their (buffered) images
  pixBlock*image=NULL;
  if (!state || !state->get(GemState::_PIX, image)) {
    return;
  }
  gem::Rectangle*roi=NULL;
  state->get(GemState::getKey("pix.roi.rectangle"),roi);
  if(roi) {
    m_roi=*roi;
    m_doROI=true;
  } else {
    m_doROI=false;
  }
  if(!image ||
      !&image->image) {
    return;
  }
  cachedPixBlock.newimage=image->newimage;
  if (!image->newimage) {
    image = &cachedPixBlock;
  } else {
    orgPixBlock = image;
    cachedPixBlock.newimage = image->newimage;
    cachedPixBlock.newfilm =
      image->newfilm; //added for newfilm copy from cache cgc 6-21-03
    image->image.copy2ImageStruct(&cachedPixBlock.image);
    image = &cachedPixBlock;
    if (m_processOnOff) {
      switch (image->image.type) {
      case GL_FLOAT:
        processFloat32(image->image);
        break;
      case GL_DOUBLE:
        processFloat64(image->image);
        break;
      default:
        switch(image->image.format) {
        case GL_RGBA:
        case GL_BGRA_EXT:
          switch(m_simd) {
          case(GEM_SIMD_MMX):
            processRGBAMMX(image->image);
            break;
          case(GEM_SIMD_SSE2):
            processRGBASSE2(image->image);
            break;
          case(GEM_SIMD_ALTIVEC):
            processRGBAAltivec(image->image);
            break;
          default:
            processRGBAImage(image->image);
          }
          break;
        case GL_RGB:
        case GL_BGR_EXT:
          processRGBImage(image->image);
          break;
        case GL_LUMINANCE:
          switch(m_simd) {
          case(GEM_SIMD_MMX):
            processGrayMMX(image->image);
            break;
          case(GEM_SIMD_SSE2):
            processGraySSE2(image->image);
            break;
          case(GEM_SIMD_ALTIVEC):
            processGrayAltivec(image->image);
            break;
          default:
            processGrayImage(image->image);
          }
          break;
        case GL_YUV422_GEM:
          switch(m_simd) {
          case(GEM_SIMD_MMX):
            processYUVMMX(image->image);
            break;
          case(GEM_SIMD_SSE2):
            processYUVSSE2(image->image);
            break;
          case(GEM_SIMD_ALTIVEC):
            processYUVAltivec(image->image);
            break;
          default:
            processYUVImage(image->image);
          }
          break;
        default:
          processImage(image->image);
        }
      }
    }
  }
  state->set(GemState::_PIX, image);
}

//////////
// get the original state back
void GemPixObj :: postrender(GemState *state)
{
  state->set(GemState::_PIX, orgPixBlock);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void GemPixObj :: processImage(imageStruct &image)
{
  switch (image.format) {
  case GL_RGBA:
  case GL_BGRA_EXT:
    pd_error(0, "cannot handle RGBA image");
    break;
  case GL_RGB:
  case GL_BGR_EXT:
    pd_error(0, "cannot handle RGB image");
    break;
  case GL_LUMINANCE:
    pd_error(0, "cannot handle Grey image");
    break;
  case GL_YUV422_GEM:
    pd_error(0, "cannot handle YUV image");
    break;
  default:
    pd_error(0, "cannot handle this format (%x) !", image.format);
  }
}
void GemPixObj :: processFloat32(imageStruct &image)
{
  switch (image.format) {
  case GL_RGBA:
  case GL_BGRA_EXT:
    pd_error(0, "cannot handle RGBA/float image");
    break;
  case GL_RGB:
  case GL_BGR_EXT:
    pd_error(0, "cannot handle RGB/float image");
    break;
  case GL_LUMINANCE:
    pd_error(0, "cannot handle Grey/float image");
    break;
  case GL_YUV422_GEM:
    pd_error(0, "cannot handle YUV/float image");
    break;
  default:
    pd_error(0, "cannot handle this format (0x%X/float) !", image.format);
  }
}

void GemPixObj :: processFloat64(imageStruct &image)
{
  switch (image.format) {
  case GL_RGBA:
  case GL_BGRA_EXT:
    pd_error(0, "cannot handle RGBA/double image");
    break;
  case GL_RGB:
  case GL_BGR_EXT:
    pd_error(0, "cannot handle RGB/double image");
    break;
  case GL_LUMINANCE:
    pd_error(0, "cannot handle Grey/double image");
    break;
  case GL_YUV422_GEM:
    pd_error(0, "cannot handle YUV/double image");
    break;
  default:
    pd_error(0, "cannot handle this format (0x%X/double) !", image.format);
  }
}


/////////////////////////////////////////////////////////
// processImage (typed)
//
/////////////////////////////////////////////////////////
void GemPixObj :: processRGBAImage(imageStruct &image)
{
  processImage(image);
}
void GemPixObj :: processRGBImage(imageStruct &image)
{
  processImage(image);
}
void GemPixObj :: processGrayImage(imageStruct &image)
{
  processImage(image);
}
void GemPixObj :: processYUVImage(imageStruct &image)
{
  processImage(image);
}

/////////////////////////////////////////////////////////
// processImage - SIMD (typed)
//
/////////////////////////////////////////////////////////
void GemPixObj :: processRGBAMMX    (imageStruct &image)
{
  processRGBAImage(image);
}
void GemPixObj :: processGrayMMX    (imageStruct &image)
{
  processGrayImage(image);
}
void GemPixObj :: processYUVMMX     (imageStruct &image)
{
  processYUVImage(image);
}
void GemPixObj :: processRGBASSE2   (imageStruct &image)
{
  processRGBAMMX(image);
}
void GemPixObj :: processGraySSE2   (imageStruct &image)
{
  processGrayMMX(image);
}
void GemPixObj :: processYUVSSE2    (imageStruct &image)
{
  processYUVMMX(image);
}
void GemPixObj :: processRGBAAltivec(imageStruct &image)
{
  processRGBAImage(image);
}
void GemPixObj :: processGrayAltivec(imageStruct &image)
{
  processGrayImage(image);
}
void GemPixObj :: processYUVAltivec (imageStruct &image)
{
  processYUVImage(image);
}


/////////////////////////////////////////////////////////
// processOnOff
//
/////////////////////////////////////////////////////////
void GemPixObj :: processOnOff(int on)
{
  m_processOnOff = on;
  setPixModified();
}

/////////////////////////////////////////////////////////
// static member functions
//
/////////////////////////////////////////////////////////
void GemPixObj :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG1(classPtr, "float", processOnOff, int);
  CPPEXTERN_MSG1(classPtr, "simd", SIMD, int);
}
void GemPixObj :: SIMD(int n)
{
  m_simd=GemSIMD::requestCPU(n);
}
