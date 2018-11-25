// Affine3D.h
// -- points, vectors, transformations
// cs541 8/17

#ifndef CS541_AFFINE3D
#define CS541_AFFINE3D

#include <cmath>


struct Hcoord {
  float x, y, z, w;
  explicit Hcoord(float X=0, float Y=0, float Z=0, float W=0)
      : x(X), y(Y), z(Z), w(W) {}
  float operator[](int i) const { return *(&x+i); }
  float& operator[](int i) { return *(&x+i); }
  static bool near(float x, float y) { return std::abs(x-y) < 1e-4f; }
};


struct Matrix {
  Hcoord rows[4];
  Hcoord& operator[](int i) { return rows[i]; }
  const Hcoord& operator[](int i) const { return rows[i]; }
};


Hcoord operator+(const Hcoord& u, const Hcoord& v);
Hcoord operator-(const Hcoord& u, const Hcoord& v);
Hcoord operator-(const Hcoord& v);
Hcoord operator*(float r, const Hcoord& v);
Hcoord operator*(const Matrix& A, const Hcoord& v);
Matrix operator*(const Matrix& A, const Matrix& B);
float dot(const Hcoord& u, const Hcoord& v);
float abs(const Hcoord& v);
Hcoord normalize(const Hcoord& v);
Hcoord cross(const Hcoord& u, const Hcoord& v);
Matrix rotate(float deg, const Hcoord& v);
Matrix translate(const Hcoord& v);
Matrix scale(float r);
Matrix scale(float rx, float ry, float rz);
Matrix inverse3x3(const Matrix &A);
Matrix transpose3x3(const Matrix &A);
Matrix perspective(float fov, float aspect, float near, float far);
Matrix perspective(float fov, float aspect, float near);


#endif
