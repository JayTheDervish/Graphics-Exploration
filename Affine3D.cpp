// Affine3D.cpp
// -- cs 541 8/17

#include <cassert>
#include "Affine3D.h"
using namespace std;


const float DEG_TO_RAD = 4.0f*atan(1.0f)/180.0f;


Hcoord operator+(const Hcoord& u, const Hcoord& v) {
  return Hcoord(u.x+v.x,u.y+v.y,u.z+v.z,u.w+v.w);
}


Hcoord operator-(const Hcoord& u, const Hcoord& v) {
  return Hcoord(u.x-v.x,u.y-v.y,u.z-v.z,u.w-v.w);
}


Hcoord operator-(const Hcoord& v) {
  return Hcoord(-v.x,-v.y,-v.z,-v.w);
}


Hcoord operator*(float r, const Hcoord& v) {
  return Hcoord(r*v.x,r*v.y,r*v.z,r*v.w);
}


Hcoord operator*(const Matrix& A, const Hcoord& v) {
  return Hcoord(dot(A[0],v),dot(A[1],v),dot(A[2],v),dot(A[3],v));
}


Matrix operator*(const Matrix& A, const Matrix& B) {
  Matrix AB;
  for (int r=0; r < 4; ++r)
    for (int c=0; c < 4; ++c)
      for (int k=0; k < 4; ++k) 
        AB[r][c] += A[r][k] * B[k][c];
  return AB;
}


float dot(const Hcoord& u, const Hcoord& v) {
  return u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
}


float abs(const Hcoord& v) {
  return sqrt(dot(v,v));
}


Hcoord normalize(const Hcoord& v) {
  float ilen = 1.0f/abs(v);
  return ilen*v;
}


Hcoord cross(const Hcoord& u, const Hcoord& v) {
  return Hcoord(u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x,0);
}


Matrix rotate(float t, const Hcoord& u) {
  float c = cos(DEG_TO_RAD*t),
        a = (1-c)/dot(u,u),
        b = sin(DEG_TO_RAD*t)/abs(u);
  Matrix R;
  R[0][0] = c + a*u.x*u.x;
    R[0][1] = a*u.x*u.y - b*u.z;
      R[0][2] = a*u.x*u.z + b*u.y;
  R[1][0] = a*u.y*u.x + b*u.z;
    R[1][1] = c + a*u.y*u.y;
      R[1][2] = a*u.y*u.z - b*u.x;
  R[2][0] = a*u.z*u.x - b*u.y;
    R[2][1] = a*u.z*u.y + b*u.x;
      R[2][2] = c + a*u.z*u.z;
  R[3][3] = 1;
  return R;
}


Matrix translate(const Hcoord& v) {
  Matrix T;
  T[0][0] = T[1][1] = T[2][2] = T[3][3] = 1;
  T[0][3] = v.x;  T[1][3] = v.y;  T[2][3] = v.z;
  return T;
}


Matrix scale(float r) {
  Matrix H;
  H[0][0] = H[1][1] = H[2][2] = r;
  H[3][3] = 1;
  return H;
}


Matrix scale(float rx, float ry, float rz) {
  Matrix H;
  H[0][0] = rx;  H[1][1] = ry;  H[2][2] = rz;
  H[3][3] = 1;
  return H;
}


Matrix inverse3x3(const Matrix &A) {
  Matrix IL;
  float idet = 1.0f/(A[0][0]*(A[1][1]*A[2][2]-A[2][1]*A[1][2])
                     - A[0][1]*(A[1][0]*A[2][2]-A[2][0]*A[1][2])
                     + A[0][2]*(A[1][0]*A[2][1]-A[1][1]*A[2][0]));
  IL[0][0] = idet*(A[1][1]*A[2][2] - A[2][1]*A[1][2]);
    IL[0][1] = -idet*(A[0][1]*A[2][2]-A[2][1]*A[0][2]);
      IL[0][2] = idet*(A[0][1]*A[1][2]-A[1][1]*A[0][2]);
  IL[1][0] = -idet*(A[1][0]*A[2][2]-A[2][0]*A[1][2]);
    IL[1][1] = idet*(A[0][0]*A[2][2]-A[2][0]*A[0][2]);
      IL[1][2] = -idet*(A[0][0]*A[1][2]-A[1][0]*A[0][2]);
  IL[2][0] = idet*(A[1][0]*A[2][1]-A[2][0]*A[1][1]);
    IL[2][1] = -idet*(A[0][0]*A[2][1]-A[2][0]*A[0][1]);
      IL[2][2] = idet*(A[0][0]*A[1][1]-A[1][0]*A[0][1]);
  IL[3][3] = 1;
  return IL;
}


Matrix transpose3x3(const Matrix &A) {
  Matrix TL;
  for (int r=0; r < 3; ++r)
    for (int c=0; c < 3; ++c)
      TL[r][c] = A[c][r];
  TL[3][3] = 1;
  return TL;
}


Matrix perspective(float fov, float aspect, float N, float F) {
  Matrix P;
  P[0][0] = 1.0f/tan(0.5f*DEG_TO_RAD*fov);
  P[1][1] = aspect*P[0][0];
  P[2][2] = (N+F)/(N-F);
  P[2][3] = 2*N*F/(N-F);
  P[3][2] = -1;
  return P;
}


Matrix perspective(float fov, float aspect, float N) {
  Matrix P;
  P[0][0] = 1.0f/tan(0.5f*DEG_TO_RAD*fov);
  P[1][1] = aspect*P[0][0];
  P[2][2] = -1;
  P[2][3] = -2*N;
  P[3][2] = -1;
  return P;
}
