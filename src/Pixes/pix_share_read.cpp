/*
 *  pix_share_read.cpp
 *  GEM_darwin
 *
 *  Created by lincoln on 9/29/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */
#include "Gem/Image.h"
#include "pix_share_read.h"
#include "Gem/State.h"

#include <errno.h>

CPPEXTERN_NEW_WITH_GIMME(pix_share_read);


pix_share_read :: pix_share_read(int argc, t_atom*argv)
  : pix_share_write(argc,argv)
{}

pix_share_read :: ~pix_share_read()
{
  /* the destructor of the parent class pix_free_write already frees the shm-segment */
  //freeShm();
}

void pix_share_read :: render(GemState *state)
{
#if USE_SHM
  if(shm_id>0) {
#elif defined _WIN32
  if(m_MapFile) {
#else
  if(0) {
#endif /* _WIN32 */
    if (shm_addr) {
      t_pixshare_header *h=(t_pixshare_header *)shm_addr;
      unsigned char* data=shm_addr+sizeof(t_pixshare_header);
      int csize=pix.image.setFormat(h->format);
      int imgsize=csize*h->xsize*h->ysize;
      if(imgsize) {
        pix.image.xsize=h->xsize;
        pix.image.ysize=h->ysize;
        pix.image.reallocate();
        pix.image.upsidedown=h->upsidedown;

        memcpy(pix.image.data,data,imgsize);

        pix.newimage = true;
        state->set(GemState::_PIX, &pix);
      }
    } else {
      pd_error(0, "no shmaddr");
      t_atom atom;
      SETFLOAT(&atom, -1);
      outlet_anything(m_outlet, gensym("error"), 1, &atom);
    }
  }
}

void pix_share_read :: obj_setupCallback(t_class *classPtr)
{}
