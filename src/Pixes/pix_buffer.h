/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia



    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_PIXES_PIX_BUFFER_H_
#define _INCLUDE__GEM_PIXES_PIX_BUFFER_H_

#include "Base/CPPExtern.h"
#include "Gem/Image.h"

#include "Gem/Properties.h"

#define DEFAULT_NUM_FRAMES 100


/*-----------------------------------------------------------------
  -------------------------------------------------------------------
  CLASS
  pix_buffer

  this is a buffer object for storing frames of video

  KEYWORDS
  pix

  DESCRIPTION

  -----------------------------------------------------------------*/
namespace gem
{
namespace plugins
{
class imagesaver;
};
namespace RTE
{
class Outlet;
};
};

class GEM_EXTERN pix_buffer : public CPPExtern
{
  CPPEXTERN_HEADER(pix_buffer, CPPExtern);

public:

  //////////
  // Constructor
  pix_buffer(t_symbol *s,t_float f);
  virtual bool  putMess(imageStruct*img,unsigned int pos);
  virtual imageStruct* getMess(unsigned int pos);
  virtual unsigned int numFrames(void);

  //////////
  // Destructor
  virtual ~pix_buffer( void );

  //////////
  // Do the processing
  virtual void  allocateMess(unsigned int,unsigned int,unsigned int);
  void          allocateMess(t_symbol*,int,t_atom*);
  virtual void  bangMess( void );
  virtual void  loadMess(std::string,int);
  virtual void  saveMess(std::string,int);

  virtual void  copyMess(int,int);

  virtual void  resizeMess(int);

  virtual void enumProperties( void );
  virtual void clearProperties( void );
  virtual void setProperties( t_symbol*, int, t_atom*);

protected:
  imageStruct    *m_buffer;
  unsigned int m_numframes;
  t_symbol *m_bindname;

  gem::Properties m_writeprops;

  gem::plugins::imagesaver*m_handle;
  gem::RTE::Outlet*m_outlet;
};

#endif  // for header file
