// Mesh3D.h
// -- triangular 3D mesh iterface
// cs541 8/17

#ifndef CS541_MESH3D_H
#define CS541_MESH3D_H

#include "Affine3D.h"


class Mesh3D {
  public:
    virtual ~Mesh3D(void) { }
    virtual int vertexCount(void) = 0;
    virtual Hcoord* vertexArray(void) = 0;
    virtual Hcoord* normalArray(void) = 0;
    virtual int faceCount(void) = 0;
    struct Face {
      unsigned index[3];
      unsigned operator[](int i) const { return index[i]; }
      unsigned& operator[](int i) { return index[i]; }
    };
    virtual Face* faceArray(void) = 0;
};


#endif

