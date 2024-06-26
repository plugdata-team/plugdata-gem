/*
 *  GEM - Graphics Environment for Multimedia
 *
 *  newWave.h
 *  gem_darwin
 *
 *  Created by Jamie Tittle on Thu Oct 10 2002.
 *  modified by cyrille Henry
 *  Copyright (c) 2002 tigital. All rights reserved.
 *    For information on usage and redistribution, and for a DISCLAIMER OF ALL
 *    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
 *
 */

#include "newWave.h"
#include "Gem/State.h"

/* Grid */
enum {WIREFRAME, HIDDENLINE, FLATSHADED, SMOOTHSHADED, TEXTURED};
enum {FACENORMALS, ENVMAP};
enum {VWEAK, WEAK, NORMAL, STRONG};
enum {SMALL, MEDIUM, LARGE, XLARGE};
enum {CURRENT, FLAT, SPIKE, DIAGONALWALL, SIDEWALL, HOLE,
      MIDDLEBLOCK, DIAGONALBLOCK, CORNERBLOCK, HILL, HILLFOUR
     };

#define SQRTOFTWOINV 1.0 / 1.414213562

static int random2(void)
{
  static int foo = 1489853723;
  foo = foo * (int)435898247 + (int)382842987;
  return (foo & 0x7fffffff);
}

CPPEXTERN_NEW_WITH_GIMME(newWave);

/////////////////////////////////////////////////////////
//
// newWave
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
newWave :: newWave( int argc,
                    t_atom*argv)//t_floatarg widthX, t_floatarg widthY )
  : GemShape(MEDIUM)
  , gridX(0), gridY(0)
  , m_height(1.0)
  , m_inletH(0), m_inletM(0)
  , xsize(0), xsize0(0), ysize(0), ysize0(0)
  , K1(0.05), D1(0.1), K2(0), D2(0), K3(0), D3(0)
  , alreadyInit(0)
  , m_textureMode(0)
  , m_resetMode(HILLFOUR)
{
  int widthX=10;
  int widthY=10;

  switch(argc) {
  /* coverity[unterminated_default] */
  default:
    pd_error(0, "ignoring extra arguments");
  case 3:
    m_height=atom_getfloat(argv+2);
  case 2:
    widthY=atom_getint(argv+1);
  case 1:
    widthX=atom_getint(argv);
    if(argc==1) {
      widthY=widthX;
    }
    break;
  case 0:
    break;
  }

  gridX = MIN(widthX, MAXGRID );
  gridX = MAX( 3,     gridX);

  gridY = MIN(widthY, MAXGRID );
  gridY = MAX( 3,     gridY);

  // the height inlet
  m_inletH = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                       gensym("height"));
  m_inletM = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float,
                       gensym("mode"));

  m_drawTypes.clear();
  m_drawTypes["default"]=GL_DEFAULT_GEM;
  m_drawTypes["point"]=GL_POINTS;
  m_drawTypes["points"]=GL_POINTS;
  m_drawTypes["line"]=GL_LINE_STRIP;
  m_drawTypes["fill"]=GL_TRIANGLE_STRIP;

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
newWave :: ~newWave(void)
{
  alreadyInit = 0;
  if(m_inletH) {
    inlet_free(m_inletH);
  }
  if(m_inletM) {
    inlet_free(m_inletM);
  }
}

void newWave :: modeMess(int mode)
{
  reset(mode);
  setModified();
}

void newWave :: positionMess(float posX, float posY, float posZ)
{
  position(posX, posY, posZ);
  setModified();

}
void newWave :: forceMess(float posX, float posY, float valforce)
{
  setforce(posX, posY, valforce);
  setModified();

}

void newWave :: textureMess(int mode)
{
  if(mode<0) {
    pd_error(0, "textureMode must be >= 0");
    return;
  }
  m_textureMode = mode;
  setModified();
  alreadyInit=0;

}

/////////////////////////////////////////////////////////
// renderShape
//
/////////////////////////////////////////////////////////
void newWave :: renderShape(GemState *state)
{
  if(m_drawType==GL_DEFAULT_GEM) {
    m_drawType=GL_TRIANGLE_STRIP;
  }

  GLfloat sizeX = 2.*m_size / (gridX-1);
  GLfloat sizeY = 2.*m_size / (gridY-1);

  glNormal3f( 0.0f, 0.0f, 1.0f);

  if (GemShape::m_texType && GemShape::m_texNum>=3) {
    if ((xsize0!= GemShape::m_texCoords[0].s) ||
        (xsize != GemShape::m_texCoords[1].s-xsize0) ||
        (ysize0!= GemShape::m_texCoords[1].t) ||
        (ysize != GemShape::m_texCoords[2].t-ysize0)) {
      alreadyInit = 0;
    }

    if (!alreadyInit) {
      xsize0 = GemShape::m_texCoords[0].s;
      xsize  = GemShape::m_texCoords[1].s-xsize0;
      ysize0 = GemShape::m_texCoords[1].t;
      ysize  = GemShape::m_texCoords[2].t-ysize0;

      setSize( gridX, gridY );
      setOther(m_textureMode);
      reset( m_resetMode );
      alreadyInit = 1;
    }

    for (int i=0; i<gridX -1; ++i) {
      glBegin(m_drawType);
      for (int j = 0; j < gridY ; ++j) {
        glNormal3fv( vertNorms[i][j] );
        glTexCoord2fv( texCoords[i][j] );
        glVertex3f( i*sizeX - 1, j*sizeY -1, posit[i][j]*m_height);

        glNormal3fv( vertNorms[i+1][j] );
        glTexCoord2fv( texCoords[i+1][j] );
        glVertex3f(  (i+1)*sizeX - 1, j*sizeY -1, posit[i+1][j]*m_height);
      }
      glEnd();
    }
  } else {
    if (!alreadyInit) {
      xsize = 1;
      ysize = 1;
      ysize0= 0;
      xsize0= 0;
      setSize( gridX, gridY);
      setOther(m_textureMode );
      reset( m_resetMode );
      alreadyInit = 1;
    }

    for (int i = 0; i<gridX -1; ++i) {
      glBegin(m_drawType);
      for (int j = 0; j < gridY  ; ++j) {
        glNormal3fv( vertNorms[i][j] );
        glTexCoord2fv( texCoords[i][j] );
        glVertex3f( i*sizeX -1, j*sizeY -1, posit[i][j]*m_height );

        glNormal3fv( vertNorms[i+1][j] );
        glTexCoord2fv( texCoords[i+1][j] );
        glVertex3f( (i+1)*sizeX -1, j*sizeY -1, posit[i+1][j]*m_height );
      }
      glEnd();
    }
  }
}

/////////////////////////////////////////////////////////
// heightMess
//
/////////////////////////////////////////////////////////
void newWave :: heightMess(float size)
{
  m_height = size;
  setModified();
}

/////////////////////////////////////////////////////////
// getforce
//
/////////////////////////////////////////////////////////
void newWave :: getforce(void)
{
  float d;
  int i;
  int j;
  for (i=0; i<gridX; i++)
    for ( j=0; j<gridY; j++) {
      force[i][j] =0.0;
    }
  // add (low amplitude) noise to avoid denormalisation.
  // this noise does propagate thrus the all structure.
  force[2][2]= 2e-20 * (double)random2() * (1. / 2147483648.) - 1e-20;

  if (K1 != 0) {
    for ( i=1; i<gridX; i++)
      for (int j=1; j<gridY; j++) {
        d = K1 * (posit[i][j] - posit[i][j-1]);
        force[i][j] -= d;
        force[i][j-1] += d;


        d = K1 * (posit[i][j] - posit[i-1][j]);
        force[i][j] -= d;
        force[i-1][j] += d;
      }
  }

  if (K2 != 0) {
    for ( i=1; i<gridX; i++)
      for (int j=1; j<gridY; j++) {
        d = K2 * (posit[i][j] - posit[i-1][j-1]);
        force[i][j] -= d;
        force[i-1][j-1] += d;

      }

    for ( i=0; i<gridX-1; i++)
      for (int j=1; j<gridY; j++) {
        d = K2 * (posit[i][j] - posit[i+1][j-1]);
        force[i][j] -= d;
        force[i+1][j-1] += d;
      }
  }

  if (K3 != 0) {
    for ( i=1; i<gridX-1; i++)
      for (int j=1; j<gridY-1; j++) {
        d = K3 * posit[i][j];
        force[i][j] -= d;
      }
  }

}
/////////////////////////////////////////////////////////
// random
//
/////////////////////////////////////////////////////////
void newWave :: noise(float rnd)
{
  int i, j;
  for (i=0; i<gridX; i++)
    for ( j=0; j<gridY; j++) {
      force[i][j] += rnd * (double)random2() * (1. / 2147483648.) - rnd/2;
    }
}

/////////////////////////////////////////////////////////
// getdamp
//
/////////////////////////////////////////////////////////
void newWave :: getdamp(void)
{
  float d;
  int i;
  int j;

  if (D1 != 0) {
    for ( i=1; i<gridX; i++) {
      for (j=1; j<gridY; j++) {
        d = D1 * ((posit[i][j] - posit[i][j-1])-(positold[i][j] - positold[i][j
                  -1]));
        force[i][j] -= d;
        force[i][j-1] += d;

        d = D1 * ((posit[i][j] - posit[i-1][j])-(positold[i][j] - positold[i
                  -1][j]));
        force[i][j] -= d;
        force[i-1][j] += d;
      }
    }
  }

  if (D2 != 0) {
    for ( i=1; i<gridX; i++)
      for (j=1; j<gridY; j++) {
        d = D2 * ((posit[i][j] - posit[i-1][j-1])-(positold[i][j] - positold[i-1][j
                  -1]));
        force[i][j] -= d;
        force[i-1][j-1] += d;
      }

    for ( i=0; i<gridX-1; i++)
      for (j=1; j<gridY; j++) {
        d = D2 * ((posit[i][j] - posit[i+1][j-1])-(positold[i][j] - positold[i+1][j
                  -1]));
        force[i][j] -= d;
        force[i+1][j-1] += d;
      }
  }

  if (D3 != 0) {
    for ( i=1; i<gridX-1; i++)
      for (j=1; j<gridY-1; j++) {
        d = D3 * (posit[i][j]-positold[i][j]);
        force[i][j] -= d;

        d = D3 * (posit[i][j]-positold[i][j]);
        force[i][j] -= d;
      }
  }
}

/////////////////////////////////////////////////////////
// force
//
/////////////////////////////////////////////////////////
void newWave :: setforce(float posX, float posY, float valforce)
{
  int posXi=static_cast<int>(posX);
  int posYi=static_cast<int>(posY);
  if ( (posXi > 0) & (posXi < gridX - 1) & (posYi > 0) &
       (posYi < gridY - 1) ) {
    force[posXi][posYi] += valforce;
  }
}

/////////////////////////////////////////////////////////
// position
//
/////////////////////////////////////////////////////////
void newWave :: position(float posX, float posY, float posZ)
{
  int posXi=static_cast<int>(posX);
  int posYi=static_cast<int>(posY);
  if ( (posXi > 0) & (posXi < gridX - 1) & (posYi > 0) &
       (posYi < gridY - 1) ) {
    posit[posXi][posYi] = posZ;
  }
}


/////////////////////////////////////////////////////////
// savepos
//
/////////////////////////////////////////////////////////
void newWave :: savepos(void)
{
  int i;
  int j;
  for (i=0; i<gridX; i++)
    for ( j=0; j<gridY; j++) {
      positold[i][j] = posit[i][j];
    }
}


/////////////////////////////////////////////////////////
// getvelocity
//
/////////////////////////////////////////////////////////
void newWave :: getvelocity(void)
{
  for (int i=1; i<gridX-1; i++)
    for (int j=1; j<gridY-1; j++) {
      veloc[i][j] += force[i][j] ;
    }
}

/////////////////////////////////////////////////////////
// getposition
//
/////////////////////////////////////////////////////////
void newWave :: getposition(void)
{
  for ( int i=1; i<gridX-1; i++)
    for ( int j=1; j<gridY-1; j++)
      //            posit[i][j] += veloc[i][j];
    {
      posit[i][j] = MAX(-1e20f, MIN(1e20f, posit[i][j]+veloc[i][j]));
    }
}

/////////////////////////////////////////////////////////
// getTexCoords
//
/////////////////////////////////////////////////////////
void newWave :: getTexCoords(void)
{
  for ( int i = 0; i < gridX; ++i) {
    for ( int j = 0; j < gridY; ++j) {
      texCoords[i][j][0] = (((xsize*1.*i)/(gridX-1)) + xsize0 );
      texCoords[i][j][1] = (((ysize*1.*j)/(gridY-1)) + ysize0 );
    }
  }
}

/////////////////////////////////////////////////////////
// setSize
//
/////////////////////////////////////////////////////////
void newWave :: setSize( int valueX, int valueY )
{
  gridX = valueX>MAXGRID?MAXGRID:valueX;
  gridY = valueY>MAXGRID?MAXGRID:valueY;

  reset(m_resetMode);

  getTexCoords();
}

/////////////////////////////////////////////////////////
// bang
//
/////////////////////////////////////////////////////////
void newWave :: bangMess(void)
{

  savepos();

  getvelocity();
  getposition();

  getFaceNorms();
  getVertNorms();

  getforce();
  getdamp();

}

/////////////////////////////////////////////////////////
// vector operations
//
/////////////////////////////////////////////////////////
void newWave :: copy(float vec0[3], float vec1[3])
{
  vec0[0] = vec1[0];
  vec0[1] = vec1[1];
  vec0[2] = vec1[2];
}

void newWave :: sub(float vec0[3], float vec1[3], float vec2[3])
{
  vec0[0] = vec1[0] - vec2[0];
  vec0[1] = vec1[1] - vec2[1];
  vec0[2] = vec1[2] - vec2[2];
}

void newWave :: add(float vec0[3], float vec1[3], float vec2[3])
{
  vec0[0] = vec1[0] + vec2[0];
  vec0[1] = vec1[1] + vec2[1];
  vec0[2] = vec1[2] + vec2[2];
}

void newWave :: scalDiv(float vec[3], float c)
{
  vec[0] /= c;
  vec[1] /= c;
  vec[2] /= c;
}

void newWave :: cross(float vec0[3], float vec1[3], float vec2[3])
{
  vec0[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
  vec0[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
  vec0[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

void newWave :: norm(float vec[3])
{
  float c = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
  scalDiv(vec, c);
}

void newWave :: set(float vec[3], float x, float y, float z)
{
  vec[0] = x;
  vec[1] = y;
  vec[2] = z;
}

/////////////////////////////////////////////////////////
// getFaceNorms
// face normals - for flat shading
/////////////////////////////////////////////////////////

void newWave :: getFaceNorms(void)
{
  float vec0[3], vec1[3], vec2[3], norm0[3], norm1[3];
  float geom0[3], geom1[3], geom2[3], geom3[3];
  for (int i = 0; i < gridX-1; ++i) {
    for ( int j = 0; j < gridY-1; ++j) {
      /* get vectors from geometry points */
      geom0[0] = i;
      geom0[1] = j;
      geom0[2] = posit[i][j];
      geom1[0] = i;
      geom1[1] = j+1;
      geom1[2] = posit[i][j+1];
      geom2[0] = i+1;
      geom2[1] = j;
      geom2[2] = posit[i+1][j];
      geom3[0] = i+1;
      geom3[1] = j+1;
      geom3[2] = posit[i+1][j+1];

      sub( vec0, geom1, geom0 );
      sub( vec1, geom1, geom2 );
      sub( vec2, geom1, geom3 );

      /* get triangle face normals from vectors & normalize them */
      cross( norm0, vec0, vec1 );
      norm( norm0 );

      cross( norm1, vec1, vec2 );
      norm( norm1 );

      copy( faceNorms[0][i][j], norm0 );
      copy( faceNorms[1][i][j], norm1 );
    }
  }
}
/////////////////////////////////////////////////////////
// getVertNorms
// vertex normals - average of face normals for smooth shading
/////////////////////////////////////////////////////////
void newWave :: getVertNorms(void)
{
  float avg[3];
  for ( int i = 0; i < gridX; ++i) {
    for ( int j = 0; j < gridY; ++j) {
      /* For each vertex, average normals from all faces sharing */
      /* vertex.  Check each quadrant in turn */
      set(avg, 0.0, 0.0, 0.0);

      /* Right & above */
      if (j < gridY-1 && i < gridX-1) {
        add( avg, avg, faceNorms[0][i][j] );
      }
      /* Right & below */
      if (j < gridY-1 && i > 0) {
        add( avg, avg, faceNorms[0][i-1][j] );
        add( avg, avg, faceNorms[1][i-1][j] );
      }
      /* Left & above */
      if (j > 0 && i < gridX-1) {
        add( avg, avg, faceNorms[0][i][j-1] );
        add( avg, avg, faceNorms[1][i][j-1] );
      }
      /* Left & below */
      if (j > 0 && i > 0) {
        add( avg, avg, faceNorms[1][i-1][j-1] );
      }

      /* Normalize */
      norm( avg );
      copy( vertNorms[i][j], avg );
    }
  }
}

/////////////////////////////////////////////////////////
// getFaceNormSegs
//
/////////////////////////////////////////////////////////
void newWave :: getFaceNormSegs(void)
{
  float center0[3], center1[3], normSeg0[3], normSeg1[3];
  float geom0[3], geom1[3], geom2[3], geom3[3];
  for ( int i = 0; i < gridX - 1; ++i) {
    for ( int j = 0; j < gridY - 1; ++j) {
      geom0[0] = i;
      geom0[1] = j;
      geom0[2] = posit[i][j];
      geom1[0] = i;
      geom1[1] = j+1;
      geom1[2] = posit[i][j+1];
      geom2[0] = i+1;
      geom2[1] = j;
      geom2[2] = posit[i+1][j];
      geom3[0] = i+1;
      geom3[1] = j+1;
      geom3[2] = posit[i+1][j+1];

      /* find center of triangle face by averaging three vertices */
      add( center0, geom2, geom0 );
      add( center0, center0, geom1 );
      scalDiv( center0, 3.0 );

      add( center1, geom2, geom1 );
      add( center1, center1, geom3 );
      scalDiv( center1, 3.0 );

      /* translate normal to center of triangle face to get normal segment */
      add( normSeg0, center0, faceNorms[0][i][j] );
      add( normSeg1, center1, faceNorms[1][i][j] );

      copy( faceNormSegs[0][0][i][j], center0 );
      copy( faceNormSegs[1][0][i][j], center1 );

      copy( faceNormSegs[0][1][i][j], normSeg0 );
      copy( faceNormSegs[1][1][i][j], normSeg1 );
    }
  }
}

void newWave :: reset(int value)
{
  if (value != CURRENT) {
    m_resetMode = value;
  }
  for( int i=0; i<gridX; i++)
    for( int j=0; j<gridY; j++) {
      force[i][j]=0.0;
      veloc[i][j]=0.0;

      switch(m_resetMode) {
      case FLAT:
        posit[i][j] = 0.0;
        break;
      case SPIKE:
        posit[i][j]= (i == gridX/2 && j == gridY/2) ? gridX*1.5 : 0.0;
        break;
      case HOLE:
        posit[i][j]= (!((i > gridX/3 && j > gridY/3)&&(i < gridX*2/3
                        && j < gridY*2/3))) ? gridX/4 : 0.0;
        break;
      case DIAGONALWALL:
        posit[i][j]= (((gridX-i)-j<3) && ((gridX-i)-j>0)) ? gridX/6 : 0.0;
        break;
      case SIDEWALL:
        posit[i][j]= (i==1) ? gridX/4 : 0.0;
        break;
      case DIAGONALBLOCK:
        posit[i][j]= ((gridX-i)-j<3) ? gridX/6 : 0.0;
        break;
      case MIDDLEBLOCK:
        posit[i][j]= ((i > gridX/3 && j > gridY/3)&&(i < gridX*2/3
                      && j < gridY*2/3)) ? gridX/4 : 0.0;
        break;
      case CORNERBLOCK:
        posit[i][j]= ((i > gridX*3/4 && j > gridY*3/4)) ? gridX/4 : 0.0;
        break;
      case HILL:
        posit[i][j]=
          (sin(M_PI * ((1.*i)/gridX)) +
           sin(M_PI * ((1.*j)/gridY)))* gridX/6.0;
        break;
      case HILLFOUR:
        posit[i][j]=
          (sin(M_PI*2.* ((1.*i)/gridX)) +
           sin(M_PI*2.* ((1.*j)/gridY)))* gridX/6.0;
        break;
      }
      if (i==0||j==0||i==gridX-1||j==gridY-1) {
        posit[i][j]=0.0;
      }
    }
}

void newWave :: setOther(int value)
{
  switch(value) {
  case 1:
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    break;
  default:
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
  }
}
/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void newWave :: obj_setupCallback(t_class *classPtr)
{
  CPPEXTERN_MSG0(classPtr, "bang", bangMess);
  CPPEXTERN_MSG1(classPtr, "height", heightMess, float);
  CPPEXTERN_MSG3(classPtr, "force", forceMess, float, float, float);
  CPPEXTERN_MSG3(classPtr, "position", positionMess, float, float, float);
  CPPEXTERN_MSG1(classPtr, "mode", modeMess, int);
  CPPEXTERN_MSG1(classPtr, "texture", textureMess, int);
  CPPEXTERN_MSG1(classPtr, "noise", noise, float);


  CPPEXTERN_MSG1(classPtr, "K1", setK1Mess, float);
  CPPEXTERN_MSG1(classPtr, "K2", setK2Mess, float);
  CPPEXTERN_MSG1(classPtr, "K3", setK3Mess, float);
  CPPEXTERN_MSG1(classPtr, "D1", setD1Mess, float);
  CPPEXTERN_MSG1(classPtr, "D2", setD2Mess, float);
  CPPEXTERN_MSG1(classPtr, "D3", setD3Mess, float);
}

void newWave :: setK1Mess(float K)
{
  K1=K;
}
void newWave :: setK2Mess(float K)
{
  K2=K;
}
void newWave :: setK3Mess(float K)
{
  K3=K;
}

void newWave :: setD1Mess(float D)
{
  D1=D;
}
void newWave :: setD2Mess(float D)
{
  D2=D;
}
void newWave :: setD3Mess(float D)
{
  D3=D;
}
