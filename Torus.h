// torus.h
// -- triangular mesh for a torus
// cs541 8/17

#ifndef CS541_TORUS_H
#define CS541_TORUS_H

#include "Affine3D.h"
#include "Mesh3D.h"


class Torus : public Mesh3D {
  public:
    Torus(float thickness, int mesh_size);
    ~Torus(void);
    int vertexCount(void);
    Hcoord* vertexArray(void);
    Hcoord* normalArray(void);
    int faceCount(void);
    Face* faceArray(void);
  private:
    int vertex_count,
        face_count;
    Hcoord *vertices,
           *normals;
    Face *faces;
};


#endif

