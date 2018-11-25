// Plane.h
// -- triangular mesh for a plane
// cs541 8/17

#ifndef CS541_PLANE_H
#define CS541_PLANE_H


#include <vector>
#include "Mesh3D.h"


class Plane : public Mesh3D {
  public:
    Plane(int mesh_size);
    int vertexCount(void);
    Hcoord* vertexArray(void);
    Hcoord* normalArray(void);
    int faceCount(void);
    Face* faceArray(void);
  private:
    std::vector<Hcoord> vertices,
                        normals;
    std::vector<Face> faces;
};


#endif

