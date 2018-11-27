# Graphics-Exploration
My basis for learning OpenGL and rendering in general

To compile from Visual Studio command prompt:
  cl /EHsc cs541transform.cpp Affine3D.cpp Torus.cpp Sphere.cpp Plane.cpp opengl32.lib glew32.lib sdl2.lib sdl2main.lib       /link /subsystem:console