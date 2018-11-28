// cs541transform.cpp
// -- assignment #5 (Shadows)
// cs541 9/17
//
// By Jay Coleman, j.coleman 
//
// To compile from Visual Studio 2015 command prompt:
//    cl /EHsc cs541transform.cpp Affine3D.cpp Torus.cpp Sphere.cpp Plane.cpp opengl32.lib glew32.lib sdl2.lib sdl2main.lib       /link /subsystem:console
// To compile from Linux command line:
//    g++ cs541transform.cpp Affine3D.cpp Torus.cpp Sphere.cpp Plane.cpp
//        -lGL -lGLEW -lSDL2

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "GL/gl.h"
#include "Affine3D.h"
#include "Torus.h"
#include "Sphere.h"
#include "Plane.h"
#include "Mesh3D.h"
using namespace std;


const int MESH_SIZE = 20;
const float THICKNESS = 0.5f;
const float PI = 4.0f*atan(1.0f);

const int FRAME_BUFFER_W = 1024,
		FRAME_BUFFER_H = 1024;

#define SKYBOX_DISTANCE 500
#define epsilon 0.37



// load a text file into a string
string loadTextFile(const char *fname) {
	string out,
		line;
	ifstream in(fname);
	getline(in, line);
	while (in) {
		out += line + "\n";
		getline(in, line);
	}
	return out;
}

// read a 24-bit color bitmap image file
//   Note: caller is responsible for deallocating the
//         RGB data
unsigned char* loadBitmapFile(const char *fname, int *width, int *height) {
	fstream in(fname, ios_base::in | ios_base::binary);
	char header[38];
	in.read(header, 38);
	unsigned offset = *reinterpret_cast<unsigned*>(header + 10);
	int W = *reinterpret_cast<int*>(header + 18),
		H = *reinterpret_cast<int*>(header + 22),
		S = 3 * W;
	S += (4 - S % 4) % 4;
	int size = S*H;
	in.seekg(offset, ios_base::beg);
	unsigned char *data = new unsigned char[size];
	in.read(reinterpret_cast<char*>(data), size);
	if (!in) {
		delete[] data;
		return 0;
	}

	for (int j = 0; j < H; ++j) {
		for (int i = 0; i < W; ++i) {
			int index = S*j + 3 * i;
			swap(data[index + 0], data[index + 2]);
		}
	}

	*width = W;
	*height = H;
	return data;
}

/////////////////////////////////////

class Client {
  public:
    Client(void);
    ~Client(void);
	void createframebuffer();
    void draw(double dt);
	void drawToBuffer(double dt);
    void keypress(SDL_Keycode kc);
    void resize(int W, int H);
    void mousedrag(int x, int y, bool lbutton);
	void mousewheel(Sint32 y);
	void drawToShadowBuffer(double dt);
  private:
	  GLuint torus_vertex_buffer,
		  torus_normal_buffer,
		  torus_face_buffer,
		  plane_vertex_buffer,
		  plane_normal_buffer,
		  plane_face_buffer,
		  sphere_vertex_buffer,
		  sphere_vertex_texturebuffer,
		  sphere_normal_buffer,
		  texcoord_buffer,
		  texcoord_buffer1,
		  skytext_buffer,
		  texture_buffer,
		  skytexture_buffer0,
		  skytexture_buffer1,
		  skytexture_buffer2,
		  skytexture_buffer3,
		  skytexture_buffer4,
		  skytexture_buffer5,
		  spheretextbuffer,
		  spherenormbuffer,
		  frame_buffer,
		  depth_buffer,
		  shadow_frame_buffer,
		  depth_texture_buffer,
		  sphere_face_buffer,
		  spheretextcoordbuffer,
		  frame_buffer_texture_buffer,
		  sphere_normal_texturebuffer,
		  vbo,
		  vao;
	  GLint program,
		  program1,
		  skyshader,
		  shadowshader,
		  aposition,
		  atexture_coord,
		  anormal,
		  B,
		  upersp_matrix,
		  uview_matrix,
		  sview_matrix,
		  umodel_matrix,
		  unormal_matrix,
		  ucolor,
		  udiffuse_color,
		  ulight_color,
		  ulight_position,
		  uambient_color,
		  ucam_position,
		  aposition1,
		  anormal1,
		  upersp_matrix1,
		  uview_matrix1,
		  umodel_matrix1,
		  unormal_matrix1,
		  sview_matrix1,
		  ucolor1,
		  udiffuse_color1,
		  ulight_color1,
		  ulight_position1,
		  uambient_color1,
		  ucam_position1,
		  aposition2,
		  anormal2,
		  upersp_matrix2,
		  sview_matrix2,
		  uview_matrix2,
		  umodel_matrix2,
		  unormal_matrix2,
		  ucolor2,
		  udiffuse_color2,
		  ulight_color2,
		  ulight_position2,
		  uambient_color2,
		  ucam_position2,
		  aposition3,
		  upersp_matrix3,
		  sview_matrix3,
		  uview_matrix3,
		  umodel_matrix3,
		  usampler,
		  usamplerSky,
		  shadow_sampler,
		  shadow_sampler1;

    int torus_face_count;
	int plane_face_count;
	int sphere_face_count;
	int viewport_save[4];
    float time;
    float aspect;
	float fov;
	void loadTorusMeshesToGPU();
	void loadSphereMeshesToGPU();
	void loadPlaneMeshesToGPU();
	void drawTorus();
	void drawSphere(int times, int othertimes, GLint shaderprog);
	void drawPlane();
	Matrix V;
	Matrix P;
	Matrix sV;
	Matrix Bmat;
	Hcoord look, eye, light, shadowlook;
	const Hcoord EY;
	const Hcoord Origin;
	
};


Client::Client(void) :EY(Hcoord(0, 1, 0, 0)), Origin(0), look(Hcoord(0, 0, -1, 0)), eye(Hcoord(0, 0, 0, 1)), light(Hcoord(0, 0, -7, 1)), fov(80.0f), shadowlook(Hcoord(0, 0, -1, 0)) {
	// (I) create shader program
	GLint value;

	// (I.A) compile fragment shader
	const char *fragment_shader_text =
		"#version 130\n\
	uniform vec3 light_color;\
	uniform vec3 ambient_color;\
	  uniform vec3 diffuse_color;\
uniform sampler2D shadow_sampler;\
		in vec4 normal_vector;\
	  in vec4 light_vector;\
in vec4 camera_vector;\
in vec4 vshadow_position;\
	out vec4 frag_color;\
	void main(void) {\
	float epsilon = 0.0001;\
		  vec4 m = normalize(normal_vector);\
		  vec4 L = normalize(light_vector);\
		 vec4 v = normalize(camera_vector);\
		vec4 H = normalize(v + L);\
		vec4 r = 2 * dot(m, L) * m - L;\
		  vec3 diffuse = max(dot(m, L), 0) * diffuse_color * light_color;\
		  vec3 ambient = ambient_color * diffuse_color;\
          vec2 uv = vshadow_position.xy/vshadow_position.w;\
          float shadow_depth = texture(shadow_sampler,uv).z;\
          float point_depth = vshadow_position.z/vshadow_position.w;\
		  point_depth = point_depth - epsilon;\
		  vec3 specular = light_color * pow(dot(H, m), 100) * light_color;\
           if(shadow_depth < point_depth || uv.x > 1 || uv.x < 0 || uv.y > 1 || uv.y < 0 || vshadow_position.w <= 0) {\n\
				specular = vec3(0.0, 0.0, 0.0);\
				diffuse = vec3(0.0, 0.0, 0.0);\
             }\n\
		  frag_color = vec4(diffuse + ambient + specular, 1);\
  }";
  GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fshader,1,&fragment_shader_text,0);
  glCompileShader(fshader);
  glGetShaderiv(fshader,GL_COMPILE_STATUS,&value);
  if (!value) {
    cerr << "fragment shader failed to compile" << endl;
    char buffer[1024];
    glGetShaderInfoLog(fshader,1024,0,buffer);
    cerr << buffer << endl;
  }

  // (I.B) compile vertex shader
  const char *vertex_shader_text =
	  "#version 130\n\
attribute vec4 position;\
  attribute vec4 normal;\
uniform mat4 persp_matrix;\
  uniform mat4 view_matrix;\
  uniform mat4 model_matrix;\
  uniform mat4 normal_matrix;\
  uniform mat4 sview_matrix;\
  uniform mat4 B;\
  uniform vec4 light_position;\
  uniform vec4 cam_position;\
out vec4 normal_vector;\
  out vec4 light_vector;\
out vec4 camera_vector;\
out vec4 vshadow_position;\
void main() {\
	  vec4 P = model_matrix * position;\
	  gl_Position = persp_matrix * view_matrix * P;\
	  normal_vector = normal_matrix * normal;\
	  light_vector = light_position - P;\
     camera_vector = cam_position - P;\
     vshadow_position = B * persp_matrix * sview_matrix * P;\
  }";
  GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vshader,1,&vertex_shader_text,0);
  glCompileShader(vshader);
  glGetShaderiv(vshader,GL_COMPILE_STATUS,&value);
  if (!value) {
    cerr << "vertex shader failed to compile" << endl;
    char buffer[1024];
    glGetShaderInfoLog(vshader,1024,0,buffer);
    cerr << buffer << endl;
  }

  // (I.C) link shaders
  program = glCreateProgram();
  glAttachShader(program,fshader);
  glAttachShader(program,vshader);
  glLinkProgram(program);
  glGetProgramiv(program,GL_LINK_STATUS,&value);
  if (!value) {
    cerr << "shader failed to compile" << endl;
    char buffer[1024];
    glGetShaderInfoLog(vshader,1024,0,buffer);
    cerr << buffer << endl;
  }


  // (II) get shader variable locations
  aposition = glGetAttribLocation(program, "position");
  anormal = glGetAttribLocation(program, "normal");
  upersp_matrix = glGetUniformLocation(program, "persp_matrix");
  uview_matrix = glGetUniformLocation(program, "view_matrix");
  umodel_matrix = glGetUniformLocation(program, "model_matrix");
  unormal_matrix = glGetUniformLocation(program, "normal_matrix");
  sview_matrix3 = glGetUniformLocation(program, "sview_matrix");
  B = glGetUniformLocation(program, "B");

  udiffuse_color = glGetUniformLocation(program, "diffuse_color");
  ulight_color = glGetUniformLocation(program, "light_color");
  ulight_position = glGetUniformLocation(program, "light_position");
  uambient_color = glGetUniformLocation(program, "ambient_color");
  ucam_position = glGetUniformLocation(program, "cam_position");

  ucolor = glGetUniformLocation(program, "color");

  shadow_sampler = glGetUniformLocation(program, "shadow_sampler");



	// (I) create shader program
	GLint value1;

	// (I.A) compile fragment shader
	// string fragtextstr = loadTextFile("diffuse_texture.frag");
	const char *fragtext =
		"#version 130\n\
	uniform vec3 light_color;\
	uniform sampler2D usampler;\
uniform sampler2D shadow_sampler;\
	uniform sampler2D norm_sampler;\
	in vec4 normal_vector;\
	in vec4 light_vector;\
	in vec2 vtexture_coord;\
    in vec4 camera_vector;\
in vec4 vshadow_position;\
	out vec4 frag_color;\
	void main(void) {\
		vec3 diffuse_color = texture(usampler, vtexture_coord).xyz;\
		vec3 normal_texture = texture(norm_sampler, vtexture_coord).xyz;\
		normal_texture = normalize(normal_texture * 2.0 - 1.0);\
		vec4 m = normalize(normal_vector);\
		vec4 L = normalize(light_vector);\
        vec4 v = normalize(camera_vector);\
        vec4 H = normalize(v + L);\
		vec3 r = max(dot(m, L), 0) * diffuse_color * light_color;\
        vec3 specular = light_color * pow(dot(H, m), 100) * light_color;\
		frag_color = vec4(r + specular, 1);\
	}";
	GLuint fshader1 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader1, 1, &fragtext, 0);
	glCompileShader(fshader1);
	glGetShaderiv(fshader1, GL_COMPILE_STATUS, &value1);
	if (!value1) {
		cerr << "fragment shader failed to compile" << endl;
		char buffer[1024];
		glGetShaderInfoLog(fshader1, 1024, 0, buffer);
		cerr << buffer << endl;
	}

	// (I.B) compile vertex shader
	// string verttextstr = loadTextFile("diffuse_texture.vert");
	const char *verttext =
		"#version 130\n\
		attribute vec4 position;\
	attribute vec4 normal;\
	attribute vec2 texture_coord;\
	uniform mat4 persp_matrix;\
	uniform mat4 view_matrix;\
	uniform mat4 model_matrix;\
	uniform mat4 normal_matrix;\
uniform mat4 sview_matrix;\
	uniform vec4 light_position;\
     uniform vec4 cam_position;\
	out vec4 normal_vector;\
	out vec4 light_vector;\
	out vec2 vtexture_coord;\
   out vec4 camera_vector;\
out vec4 vshadow_position;\
	void main() {\
		vec4 P = model_matrix * position;\
		gl_Position = persp_matrix * view_matrix * P;\
		normal_vector = normal_matrix * normal;\
		light_vector = light_position - P;\
		vtexture_coord = texture_coord;\
        camera_vector = cam_position - P;\
	}";
	GLuint vshader1 = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader1, 1, &verttext, 0);
	glCompileShader(vshader1);
	glGetShaderiv(vshader1, GL_COMPILE_STATUS, &value1);
	if (!value1) {
		cerr << "vertex shader failed to compile" << endl;
		char buffer[1024];
		glGetShaderInfoLog(vshader1, 1024, 0, buffer);
		cerr << buffer << endl;
	}

	// (I.C) link shaders
	program1 = glCreateProgram();
	glAttachShader(program1, fshader1);
	glAttachShader(program1, vshader1);
	glLinkProgram(program1);
	glGetProgramiv(program1, GL_LINK_STATUS, &value1);
	if (!value1) {
		cerr << "texture shader failed to compile" << endl;
		char buffer[1024];
		glGetShaderInfoLog(vshader1, 1024, 0, buffer);
		cerr << buffer << endl;
	}
	glDeleteShader(fshader1);
	glDeleteShader(vshader1);

	
  // (IIb) get shader variable locations
  aposition1 = glGetAttribLocation(program1, "position");
  anormal1 = glGetAttribLocation(program1, "normal");
  upersp_matrix1 = glGetUniformLocation(program1, "persp_matrix");
  uview_matrix1 = glGetUniformLocation(program1, "view_matrix");
  umodel_matrix1 = glGetUniformLocation(program1, "model_matrix");
  unormal_matrix1 = glGetUniformLocation(program1, "normal_matrix");
  sview_matrix1 = glGetUniformLocation(program1, "sview_matrix");
  atexture_coord = glGetAttribLocation(program1, "texture_coord");
  ulight_color1 = glGetUniformLocation(program1, "light_color");
  ulight_position1 = glGetUniformLocation(program1, "light_position");
  ucam_position = glGetUniformLocation(program1, "cam_position");
  usampler = glGetUniformLocation(program1, "usampler");

  shadow_sampler1 = glGetUniformLocation(program1, "shadow_sampler");


  // (I) create shader program
  GLint value2;


  //SKyshader
  // (I.A) compile fragment shader
  // string fragtextstr = loadTextFile("diffuse_texture.frag");
  const char *fragmentext =
	  "#version 130\n\
	uniform vec3 light_color;\
	uniform sampler2D usampler;\
	in vec4 normal_vector;\
	in vec4 light_vector;\
	in vec2 vtexture_coord;\
	out vec4 frag_color;\
	void main(void) {\
		vec3 diffuse_color = texture(usampler, vtexture_coord).xyz;\
		vec4 m = normalize(normal_vector);\
		vec4 L = normalize(light_vector);\
		vec3 r = diffuse_color * light_color;\
		frag_color = vec4(r, 1);\
	}";
  GLuint fshader2 = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fshader2, 1, &fragmentext, 0);
  glCompileShader(fshader2);
  glGetShaderiv(fshader2, GL_COMPILE_STATUS, &value2);
  if (!value2) {
	  cerr << "sky fragment shader failed to compile" << endl;
	  char buffer[1024];
	  glGetShaderInfoLog(fshader2, 1024, 0, buffer);
	  cerr << buffer << endl;
  }

  // (I.B) compile vertex shader
  // string verttextstr = loadTextFile("diffuse_texture.vert");
  const char *vertextext =
	  "#version 130\n\
		attribute vec4 position;\
	attribute vec4 normal;\
	attribute vec2 texture_coord;\
	uniform mat4 persp_matrix;\
	uniform mat4 view_matrix;\
	uniform mat4 model_matrix;\
	uniform mat4 normal_matrix;\
	uniform vec4 light_position;\
	out vec4 normal_vector;\
	out vec4 light_vector;\
	out vec2 vtexture_coord;\
	void main() {\
		vec4 P = model_matrix * position;\
		gl_Position = persp_matrix * view_matrix * P;\
		normal_vector = normal_matrix * normal;\
		light_vector = light_position - P;\
		vtexture_coord = texture_coord;\
	}";
  GLuint vshader2 = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vshader2, 1, &vertextext, 0);
  glCompileShader(vshader2);
  glGetShaderiv(vshader2, GL_COMPILE_STATUS, &value2);
  if (!value2) {
	  cerr << "sky vertex shader failed to compile" << endl;
	  char buffer[1024];
	  glGetShaderInfoLog(vshader2, 1024, 0, buffer);
	  cerr << buffer << endl;
  }

  skyshader = glCreateProgram();
  glAttachShader(skyshader, fshader2);
  glAttachShader(skyshader, vshader2);
  glLinkProgram(skyshader);
  glGetProgramiv(skyshader, GL_LINK_STATUS, &value2);
  if (!value2) {
	  cerr << "sky shader failed to compile" << endl;
	  char buffer[1024];
	  glGetShaderInfoLog(vshader2, 1024, 0, buffer);
	  cerr << buffer << endl;
  }
  glDeleteShader(fshader2);
  glDeleteShader(vshader2);

  // (IIb) get shader variable locations
  aposition2 = glGetAttribLocation(skyshader, "position");
  anormal2 = glGetAttribLocation(skyshader, "normal");
  upersp_matrix2 = glGetUniformLocation(skyshader, "persp_matrix");
  uview_matrix2 = glGetUniformLocation(skyshader, "view_matrix");
  umodel_matrix2 = glGetUniformLocation(skyshader, "model_matrix");
  unormal_matrix2 = glGetUniformLocation(skyshader, "normal_matrix");
  atexture_coord = glGetAttribLocation(skyshader, "texture_coord");
  ulight_color2 = glGetUniformLocation(skyshader, "light_color");
  ulight_position2 = glGetUniformLocation(skyshader, "light_position");
  
  usamplerSky = glGetUniformLocation(skyshader, "usampler");


  //Shadow shaders
  const char *shadowverttex =
	  "#version 130\n\
		attribute vec4 position;\
	uniform mat4 persp_matrix;\
	uniform mat4 view_matrix;\
	uniform mat4 model_matrix;\
	void main() {\
		vec4 P = model_matrix * position;\
		gl_Position = persp_matrix * view_matrix * P;\
	}";
  GLuint shadowvshader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shadowvshader, 1, &shadowverttex, 0);
  glCompileShader(shadowvshader);
  glGetShaderiv(shadowvshader, GL_COMPILE_STATUS, &value2);
  if (!value2) {
	  cerr << "vertex shadow shader failed to compile" << endl;
	  char buffer[1024];
	  glGetShaderInfoLog(shadowvshader, 1024, 0, buffer);
	  cerr << buffer << endl;
  }

  const char *shadowfragtex =
	  "#version 130\n\
	  out vec4 frag_color;\
	void main(void) {\
      }";

  GLuint shadowfshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shadowfshader, 1, &shadowfragtex, 0);
  glCompileShader(shadowfshader);
  glGetShaderiv(shadowfshader, GL_COMPILE_STATUS, &value2);
  if (!value2) {
	  cerr << "shadow frag shader failed to compile" << endl;
	  char buffer[1024];
	  glGetShaderInfoLog(shadowfshader, 1024, 0, buffer);
	  cerr << buffer << endl;
  }

  shadowshader = glCreateProgram();
  glAttachShader(shadowshader, shadowfshader);
  glAttachShader(shadowshader, shadowvshader);
  glLinkProgram(shadowshader);
  glGetProgramiv(shadowshader, GL_LINK_STATUS, &value2);
  if (!value2) {
	  cerr << "shadow shader failed to compile" << endl;
	  char buffer[1024];
	  glGetProgramInfoLog(shadowshader, 1024, 0, buffer);
	  cerr << buffer << endl;
  }
  glDeleteShader(shadowfshader);
  glDeleteShader(shadowvshader);

  aposition3 = glGetAttribLocation(shadowshader, "position");
  upersp_matrix3 = glGetUniformLocation(shadowshader, "persp_matrix");
  sview_matrix = glGetUniformLocation(shadowshader, "view_matrix");
  umodel_matrix3 = glGetUniformLocation(shadowshader, "model_matrix");


  // (III) upload all meshes to GPU
  loadTorusMeshesToGPU();
  loadPlaneMeshesToGPU();
  loadSphereMeshesToGPU();

  

  // (IV) upload texture to GPU
  int width, height;
  unsigned char *rgbdata = loadBitmapFile("image.bmp", &width, &height);
  glGenTextures(1, &texture_buffer);
  glBindTexture(GL_TEXTURE_2D, texture_buffer);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  //Sphere Texture
  unsigned char *rgbdata2 = loadBitmapFile("World-Map.bmp", &width, &height);
  glGenTextures(1, &spheretextbuffer);
  glBindTexture(GL_TEXTURE_2D, spheretextbuffer);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbdata2);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  //Sphere Normal Map
  unsigned char *rgbdata3 = loadBitmapFile("NormalMap.bmp", &width, &height);
  glGenTextures(1, &spherenormbuffer);
  glBindTexture(GL_TEXTURE_2D, spherenormbuffer);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbdata2);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  
  //skytextures
  unsigned char *skytexture0data = loadBitmapFile("cube0.bmp", &width, &height);
  glGenTextures(1, &skytexture_buffer0);
  glBindTexture(GL_TEXTURE_2D, skytexture_buffer0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytexture0data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  unsigned char *skytexture1data = loadBitmapFile("cube1.bmp", &width, &height);
  glGenTextures(1, &skytexture_buffer1);
  glBindTexture(GL_TEXTURE_2D, skytexture_buffer1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytexture1data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  unsigned char *skytexture2data = loadBitmapFile("cube2.bmp", &width, &height);
  glGenTextures(1, &skytexture_buffer2);
  glBindTexture(GL_TEXTURE_2D, skytexture_buffer2);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytexture2data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  unsigned char *skytexture3data = loadBitmapFile("cube3.bmp", &width, &height);
  glGenTextures(1, &skytexture_buffer3);
  glBindTexture(GL_TEXTURE_2D, skytexture_buffer3);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytexture3data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  unsigned char *skytexture4data = loadBitmapFile("cube4.bmp", &width, &height);
  glGenTextures(1, &skytexture_buffer4);
  glBindTexture(GL_TEXTURE_2D, skytexture_buffer4);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytexture4data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
 
  unsigned char *skytexture5data = loadBitmapFile("cube5.bmp", &width, &height);
  glGenTextures(1, &skytexture_buffer5);
  glBindTexture(GL_TEXTURE_2D, skytexture_buffer5);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skytexture5data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   // (IV) enable use of z-buffer
  glEnable(GL_DEPTH_TEST);

  //loading shadow frame buffer
  glGenFramebuffers(1, &shadow_frame_buffer);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_frame_buffer);
  glGenTextures(1, &depth_texture_buffer);
  glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024,
	  0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	  GL_TEXTURE_2D, depth_texture_buffer, 0);
  glDrawBuffer(GL_NONE);
  
 

  aspect = 1;
  time = 0;

  Bmat = scale(0.5f) * translate(Hcoord(0.5f, 0.5f, 0.5f));
}


Client::~Client(void) {
  // delete the shader program
  glUseProgram(0);
  GLuint shaders[2];
  GLsizei count;
  glGetAttachedShaders(program,2,&count,shaders);
  for (int i=0; i < count; ++i)
    glDeleteShader(shaders[i]);
  glDeleteProgram(program);

  // delete the vertex, normal, and face buffers for the Torus
  glDeleteBuffers(1,&torus_face_buffer);
  glDeleteBuffers(1,&torus_normal_buffer);
  glDeleteBuffers(1,&torus_vertex_buffer);

  // delete the vertex, normal, and face buffers for the Sphere
  glDeleteBuffers(1, &sphere_face_buffer);
  glDeleteBuffers(1, &sphere_normal_buffer);
  glDeleteBuffers(1, &sphere_vertex_buffer);

  // delete the vertex, normal, and face buffers for the Plane
  glDeleteBuffers(1, &plane_face_buffer);
  glDeleteBuffers(1, &plane_normal_buffer);
  glDeleteBuffers(1, &plane_vertex_buffer);

  glDeleteBuffers(1, &texcoord_buffer);

  // delete texture buffers
  glDeleteTextures(1, &texture_buffer);
  glDeleteTextures(1, &skytexture_buffer0);
  glDeleteTextures(1, &skytexture_buffer1);
  glDeleteTextures(1, &skytexture_buffer2);
  glDeleteTextures(1, &skytexture_buffer3);
  glDeleteTextures(1, &skytexture_buffer4);
  glDeleteTextures(1, &skytexture_buffer5);

  // delete frame buffer
  glDeleteRenderbuffers(1, &depth_buffer);
  glDeleteTextures(1, &frame_buffer_texture_buffer);
  glDeleteFramebuffers(1, &frame_buffer);
  glDeleteFramebuffers(1, &shadow_frame_buffer);
}

void Client::drawToShadowBuffer(double dt) {
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_frame_buffer);
	glGetIntegerv(GL_VIEWPORT, viewport_save);
	glViewport(0, 0, 1024, 1024);

      glClearColor(1,1,1,1);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	//Viewing matrix from lightsource
	sV = scale(1.0f);
	sV.rows[0] = normalize(cross(shadowlook, EY));
	sV.rows[2] = -normalize(shadowlook);
	sV.rows[1] = cross(sV.rows[2], sV.rows[0]);
	sV = sV * translate(Origin - light);

	// set shader uniform variable values
	
	glUseProgram(shadowshader);
	//glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
	P = perspective(fov, aspect, 0.1f);
	glUniformMatrix4fv(sview_matrix, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix3, 1, true, (float*)&P);
	//glUniformMatrix4fv(B, 1, true, (float*)&Bmat);
	const float RATE = 360.0f / 5.0f;
	const Hcoord AXIS(0, 1, 0, 0),
		CENTER(0, 0, -8, 0);
	Matrix R = rotate(RATE*time, AXIS),
		M = translate(CENTER)
		* R
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix3, 1, true, (float*)&M);



	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, torus_vertex_buffer);
	glEnableVertexAttribArray(aposition3);
	glVertexAttribPointer(aposition3, 4, GL_FLOAT, false, 0, 0);

	// draw the mesh for the Torus
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * torus_face_count, GL_UNSIGNED_INT, 0);

	//draw Sphere for texture
	//
	// set shader uniform variable values
	glUseProgram(shadowshader);
	glUniformMatrix4fv(sview_matrix, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix3, 1, true, (float*)&P);

	const Hcoord point(17, 20, -17, 0), axis(1, 0, 0, 0);
	Matrix R2 = rotate(RATE*time, AXIS) * rotate(90.0f, axis),
		M2 = translate(point)
		* R2
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix3, 1, true, (float*)&M2);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_texturebuffer);
	glVertexAttribPointer(aposition3, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition3);


	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

	//function call to draw the Plane
	// set shader uniform variable values
	glUseProgram(shadowshader);

	glUniformMatrix4fv(sview_matrix, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix3, 1, true, (float*)&P);
	const float RATE1 = 360.0f / 5.0f;
	const Hcoord AXIS1(1, 0, 0, 0),
		CENTER1(11, -11, -17, 0);
	Matrix R3 = rotate(RATE1*time, AXIS1),
		M3 = translate(CENTER1)
		* R3
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix3, 1, true, (float*)&M3);



	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition3, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition3);


	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

	//Add tiled plane
	glUniformMatrix4fv(upersp_matrix3, 1, true, (float*)&P);
	const Hcoord CENTER2(11, -6, -17, 0);
	M = translate(CENTER2)
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix3, 1, true, (float*)&M);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition3, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition3);


	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

	//loop through and draw ~50 spheres
	for (int j = 0; j < 25; j++)
		for (int i = 0; i < 25; i++)
			drawSphere(j, i, shadowshader);

	time += dt;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Client::createframebuffer() {
	//Adding frame buffer
	// create a frame buffer object to use a texture
	// (1) frame buffer
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	// (2) texture associated to frame buffer
	glGenTextures(1, &frame_buffer_texture_buffer);
	glBindTexture(GL_TEXTURE_2D, frame_buffer_texture_buffer);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_BUFFER_W, FRAME_BUFFER_H,
		0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// (3) depth buffer
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 1024);
	// (4) attach depth buffer to frame buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, depth_buffer);
	// (5) attach texture to frame buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, frame_buffer_texture_buffer, 0);
	// (6) make sure everything is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "frame buffer failure" << endl;
}

void Client::drawToBuffer(double dt) {
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glViewport(0, 0, FRAME_BUFFER_W, FRAME_BUFFER_H);

	// clear frame buffer and z-buffer
	glClearColor(0.9f, 0.9f, 0.9f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);

	V = scale(1.0f);
	V.rows[0] = normalize(cross(look, EY));
	V.rows[2] = -normalize(look);
	V.rows[1] = cross(V.rows[2], V.rows[0]);
	V = V * translate(Origin - eye);

	//Viewing matrix from lightsource
	sV = scale(1.0f);
	sV.rows[0] = normalize(cross(shadowlook, EY));
	sV.rows[2] = -normalize(shadowlook);
	sV.rows[1] = cross(sV.rows[2], sV.rows[0]);
	sV = sV * translate(Origin - light);

	// set shader uniform variable values
	glUseProgram(program);
	glUniform3f(ulight_color, 1, 1, 1);
	glUniform3f(uambient_color, 0, 0.1, 0.1);
	glUniform4f(ulight_position, light.x, light.y, light.z, light.w);
	glUniform4f(ucam_position, eye.x, eye.y, eye.z, eye.w);
	P = perspective(fov, aspect, 0.1f);
	glUniformMatrix4fv(upersp_matrix, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix, 1, true, (float*)&V);
	const float RATE = 360.0f / 5.0f;
	const Hcoord AXIS(0, 1, 0, 0),
		CENTER(0, 0, -8, 0);
	Matrix R = rotate(RATE*time, AXIS),
		M = translate(CENTER)
		* R
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix, 1, true, (float*)&R);
	glUniformMatrix4fv(sview_matrix3, 1, true, (float*)&sV);
	glUniform3f(udiffuse_color, 1, 1, 0);


	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, torus_vertex_buffer);
	glVertexAttribPointer(aposition, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition);
	glBindBuffer(GL_ARRAY_BUFFER, torus_normal_buffer);
	glVertexAttribPointer(anormal, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal);

	// draw the mesh for the Torus
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * torus_face_count, GL_UNSIGNED_INT, 0);

	//draw Sphere for lightsource
	//
	// set shader uniform variable values
	glUseProgram(program);
	glUniformMatrix4fv(sview_matrix3, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix, 1, true, (float*)&V);

	const Hcoord AXIS1(0, 1, 0, 0);
	Matrix R1 = rotate(RATE*time, AXIS1),
		M1 = translate(light)
		* R1
		* scale(0.1f);
	glUniformMatrix4fv(umodel_matrix, 1, true, (float*)&M1);
	glUniformMatrix4fv(unormal_matrix, 1, true, (float*)&R1);
	glUniform3f(udiffuse_color, 0, 0, 0);


	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
	glVertexAttribPointer(aposition, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_buffer);
	glVertexAttribPointer(anormal, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * sphere_face_count, GL_UNSIGNED_INT, 0);

	//draw Sphere for texture
	//
	// set shader uniform variable values
	glUseProgram(program1);
	glUniformMatrix4fv(sview_matrix1, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix1, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix1, 1, true, (float*)&V);

	const Hcoord point(17, 20, -17, 0), axis(1, 0, 0, 0);
	Matrix R2 = rotate(RATE*time, AXIS1) * rotate(90.0f, axis),
		M2 = translate(point)
		* R2
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix1, 1, true, (float*)&M2);
	glUniformMatrix4fv(unormal_matrix1, 1, true, (float*)&R2);


	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_texturebuffer);
	glVertexAttribPointer(aposition1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition1);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_texturebuffer);
	glVertexAttribPointer(anormal1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal1);

	glBindBuffer(GL_ARRAY_BUFFER, spheretextcoordbuffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	//select texture to use
	//glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, spheretextbuffer);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

	//function call to draw the Plane
	drawPlane();

	//loop through and draw ~50 spheres
	for (int j = 0; j < 25; j++)
		for (int i = 0; i < 25; i++)
			drawSphere(j, i, program);

	time += dt;
}

void Client::draw(double dt) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(viewport_save[0], viewport_save[1], viewport_save[2], viewport_save[3]);

  // clear frame buffer and z-buffer
  glClearColor(0.9f,0.9f,0.9f,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearDepth(1);
  glClear(GL_DEPTH_BUFFER_BIT);

  V = scale(1.0f);
  V.rows[0] = normalize(cross(look, EY));
  V.rows[2] = -normalize(look);
  V.rows[1] = cross(V.rows[2], V.rows[0]);
  V = V * translate(Origin - eye);

  //Viewing matrix from lightsource
  sV = scale(1.0f);
  sV.rows[0] = normalize(cross(shadowlook, EY));
  sV.rows[2] = -normalize(shadowlook);
  sV.rows[1] = cross(sV.rows[2], sV.rows[0]);
  sV = sV * translate(Origin - light);

  // set shader uniform variable values
  glUseProgram(program);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
  glUniform1i(shadow_sampler, 1);

  
  glUniform3f(ulight_color, 1, 1, 1);
  glUniform3f(uambient_color, 0, 0.1, 0.1);
  glUniform4f(ulight_position, light.x, light.y, light.z, light.w);
  glUniform4f(ucam_position, eye.x, eye.y, eye.z, eye.w);
  P = perspective(fov,aspect,0.1f);
  
  glUniformMatrix4fv(sview_matrix3, 1, true, (float*)&sV);
  glUniformMatrix4fv(upersp_matrix,1,true,(float*)&P);
  glUniformMatrix4fv(uview_matrix,1,true,(float*)&V);
  glUniformMatrix4fv(B, 1, true, (float*)&Bmat);
  const float RATE = 360.0f/5.0f;
  const Hcoord AXIS(0,1,0,0),
               CENTER(0,0,-8,0);
  Matrix R = rotate(RATE*time,AXIS),
         M = translate(CENTER)
             * R
             * scale(1.5f);
  glUniformMatrix4fv(umodel_matrix,1,true,(float*)&M);
  glUniformMatrix4fv(unormal_matrix,1,true,(float*)&R);
  glUniform3f(udiffuse_color,1,1,0);


  // set shader attributes
  glBindBuffer(GL_ARRAY_BUFFER, torus_vertex_buffer);
  glVertexAttribPointer(aposition,4,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(aposition);
  glBindBuffer(GL_ARRAY_BUFFER, torus_normal_buffer);
  glVertexAttribPointer(anormal,4,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(anormal);

  // draw the mesh for the Torus
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus_face_buffer);
  glDrawElements(GL_TRIANGLES,3* torus_face_count,GL_UNSIGNED_INT,0);

  //draw Sphere for lightsource
  //
  // set shader uniform variable values
  glUseProgram(program);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
  glUniform1i(shadow_sampler, 1);
  glUniformMatrix4fv(upersp_matrix, 1, true, (float*)&P);
  glUniformMatrix4fv(sview_matrix3, 1, true, (float*)&sV);
  glUniformMatrix4fv(uview_matrix, 1, true, (float*)&V);
  glUniformMatrix4fv(B, 1, true, (float*)&Bmat);

  const Hcoord AXIS1(0, 1, 0, 0);
  Matrix R1 = rotate(RATE*time, AXIS1),
	  M1 = translate(light)
	  * R1
	  * scale(0.1f);
  glUniformMatrix4fv(umodel_matrix, 1, true, (float*)&M1);
  glUniformMatrix4fv(unormal_matrix, 1, true, (float*)&R1);
  glUniform3f(udiffuse_color, 0, 0, 0);


  // set shader attributes
  glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
  glVertexAttribPointer(aposition, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(aposition);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_buffer);
  glVertexAttribPointer(anormal, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(anormal);

  // draw the mesh
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_face_buffer);
  glDrawElements(GL_TRIANGLES, 3 * sphere_face_count, GL_UNSIGNED_INT, 0);

  //draw frame buffer plane
  //
  glUseProgram(program1);
  
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
  glUniform1i(shadow_sampler1, 1);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(usampler, 0);
  
  glUniformMatrix4fv(sview_matrix1, 1, true, (float*)&sV);
  glUniformMatrix4fv(upersp_matrix1, 1, true, (float*)&P);
  glUniformMatrix4fv(uview_matrix1, 1, true, (float*)&V);
  M = translate(Hcoord(5, -6, -17, 0))
	  * scale(1.5f);
  glUniformMatrix4fv(umodel_matrix1, 1, true, (float*)&M);
  glUniformMatrix4fv(unormal_matrix1, 1, true, (float*)&M);
  glUniformMatrix4fv(B, 1, true, (float*)&Bmat);

  // set shader attributes
  glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
  glVertexAttribPointer(aposition1, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(aposition1);
  glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
  glVertexAttribPointer(anormal1, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(anormal1);
  
  
  glBindTexture(GL_TEXTURE_2D, frame_buffer_texture_buffer);
  glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

  //draw Sphere for texture
  //
  // set shader uniform variable values
  glUseProgram(program1);
  
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
  glUniform1i(shadow_sampler1, 1);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(usampler, 0);
  
  glUniformMatrix4fv(upersp_matrix1, 1, true, (float*)&P);
  glUniformMatrix4fv(uview_matrix1, 1, true, (float*)&V);
  glUniformMatrix4fv(sview_matrix1, 1, true, (float*)&sV);
  glUniformMatrix4fv(B, 1, true, (float*)&Bmat);

  const Hcoord point(17, 20, -17, 0), axis(1, 0, 0, 0);
  Matrix R2 = rotate(RATE*time, AXIS1) * rotate(90.0f, axis),
	  M2 = translate(point)
	  * R2
	  * scale(1.5f);
  glUniformMatrix4fv(umodel_matrix1, 1, true, (float*)&M2);
  glUniformMatrix4fv(unormal_matrix1, 1, true, (float*)&R2);


  // set shader attributes
  glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_texturebuffer);
  glVertexAttribPointer(aposition1, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(aposition1);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_texturebuffer);
  glVertexAttribPointer(anormal1, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(anormal1);

  glBindBuffer(GL_ARRAY_BUFFER, spheretextcoordbuffer);
  glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(atexture_coord);

  //select texture to use
  glBindTexture(GL_TEXTURE_2D, spheretextbuffer);

  // draw the mesh
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
  glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

  //function call to draw the Plane
  drawPlane();

  //loop through and draw ~50 spheres
  for (int j=0; j < 25; j++)
	  for (int i=0; i < 25; i++)
			drawSphere(j, i, program);

  time += dt;
}


void Client::keypress(SDL_Keycode kc) {
  // respond to keyboard input
  //   kc: SDL keycode (e.g., SDLK_SPACE, SDLK_a, SDLK_s)
	
	float XLAT = 1.0f;
	Hcoord u = normalize(cross(look, EY));
	Hcoord v = normalize(cross(look, u));

	switch (kc)
	{
	case SDLK_w:
		//Up
		eye = eye - XLAT * v;
		break;
	case SDLK_s:
		//down
		eye = eye + XLAT * v;
		break;
	case SDLK_a:
		//left
		eye = eye - XLAT * u;
		break;
	case SDLK_d:
		//right
		eye = eye + XLAT * u;
		break;
	case SDLK_q:
		//move in
		eye = eye + XLAT * look;
		break;
	case SDLK_e:
		//move out
		eye = eye - XLAT * look;
		break;
	case SDLK_UP:
		//move light up
		light = light - XLAT * v;
		break;
	case SDLK_DOWN:
		//move light down
		light = light + XLAT * v;
		break;
	case SDLK_LEFT:
		//move light left
		light = light - XLAT * u;
		break;
	case SDLK_RIGHT:
		//move light right
		light = light + XLAT * u;
		break;
	case SDLK_PAGEUP:
		//move light in
		light = light + XLAT * shadowlook;
		break;
	case SDLK_PAGEDOWN:
		//move light out
		light = light - XLAT * shadowlook;
		break;
	default:
		break;
	}
}


void Client::resize(int W, int H) {
  aspect = float(W)/float(H);
  glViewport(0,0,W,H);
}


void Client::mousedrag(int x, int y, bool left_button) {
  // respond to mouse click
  //   (x,y): click location (in window coordinates)

	Hcoord u = normalize(cross(look, EY));
	Hcoord v = normalize(cross(look, u));
	if (left_button)
	{
		Matrix R1 = rotate(- y, u);
		Matrix R2 = rotate(x, v);
		look = R2 * R1 * look;
	}
}

void Client::mousewheel(Sint32 y)
{
	const float upperLimit = 130.0f;
	const float lowerLimit = 20.0f;
	float increment = 1.0f;
	if (y == 1)
	{
		if(fov > lowerLimit)
			//scroll up, zoom in
			fov = fov - increment;
	}
	else
	{
		if(fov < upperLimit)
			//scroll down, zoom out
			fov = fov + increment;
	}
}

void Client::loadTorusMeshesToGPU(void)
{
	Torus torus(THICKNESS, MESH_SIZE);
	torus_face_count = torus.faceCount();

	// (III.A) vertex buffer for the Torus
	glGenBuffers(1, &torus_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, torus_vertex_buffer);
	int vertex_buffer_size = sizeof(Hcoord)*torus.vertexCount();
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, torus.vertexArray(), GL_STATIC_DRAW);

	// (III.B) normal buffer for the Torus
	glGenBuffers(1, &torus_normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, torus_normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, torus.normalArray(), GL_STATIC_DRAW);

	// (III.C) face buffer for the Torus
	glGenBuffers(1, &torus_face_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus_face_buffer);
	int face_buffer_size = sizeof(Mesh3D::Face)*torus_face_count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_buffer_size, torus.faceArray(), GL_STATIC_DRAW);
}

void Client::loadPlaneMeshesToGPU(void)
{
	Plane plane(MESH_SIZE);
	plane_face_count = plane.faceCount();

	// (III.A) vertex buffer for the Plane
	glGenBuffers(1, &plane_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	int vertex_buffer_size = sizeof(Hcoord)*plane.vertexCount();
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, plane.vertexArray(), GL_STATIC_DRAW);

	// (III.B) normal buffer for the Plane
	glGenBuffers(1, &plane_normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glBufferData(GL_ARRAY_BUFFER,vertex_buffer_size, plane.normalArray(),GL_STATIC_DRAW);

	// (III.C) face buffer for the Plane
	glGenBuffers(1, &plane_face_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	int face_buffer_size = sizeof(Mesh3D::Face)*plane_face_count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,face_buffer_size, plane.faceArray(),GL_STATIC_DRAW);

	// (III.D) texture coordinate buffer
	glGenBuffers(1, &texcoord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
	float *texcoord = new float[2 * plane.vertexCount()];
	Matrix Std2Unit = scale(0.5f, 0.5f, 1)
		* translate(Hcoord(1, 1, 0, 0));
	for (int i = 0; i < plane.vertexCount(); ++i) {
		Hcoord uv = Std2Unit * plane.vertexArray()[i];
		texcoord[2 * i + 0] = uv[0];
		texcoord[2 * i + 1] = uv[1];
	}
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float)*plane.vertexCount(), texcoord, GL_STATIC_DRAW);
	delete[] texcoord;

	// (III.E) texture coordinate buffer (tiled)
	glGenBuffers(1, &texcoord_buffer1);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer1);
	float *texcoord1 = new float[2 * plane.vertexCount()];
	Matrix Std2UnitScaled = scale(5.5f, 5.5f, 1)
		* translate(Hcoord(1, 1, 0, 0));
	for (int i = 0; i < plane.vertexCount(); ++i) {
		Hcoord st = Std2UnitScaled * plane.vertexArray()[i];
		texcoord1[2 * i + 0] = st[0];
		texcoord1[2 * i + 1] = st[1];
	}
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float)*plane.vertexCount(), texcoord1, GL_STATIC_DRAW);
	delete[] texcoord1;

	
	//Texture coordinate buffer for skycube
	glGenBuffers(1, &skytext_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	float *texcoord2 = new float[2 * plane.vertexCount() * 6];
	Matrix Std2UnitSky = scale(0.5f, 0.5f, 1)
		* translate(Hcoord(1, 1, 0, 0));
	for (int i = 0; i < plane.vertexCount(); ++i) {
		Hcoord uv = Std2UnitSky * plane.vertexArray()[i];
		texcoord2[2 * i + 0] = uv[0];
		texcoord2[2 * i + 1] = uv[1];
	}
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float)*plane.vertexCount(), texcoord2, GL_STATIC_DRAW);
	delete[] texcoord2;
}

void Client::loadSphereMeshesToGPU(void)
{
	Sphere sphere(MESH_SIZE);
	sphere_face_count = sphere.faceCount();

	// (III.A) vertex buffer for the Sphere
	glGenBuffers(1, &sphere_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
	int vertex_buffer_size = sizeof(Hcoord)*sphere.vertexCount();
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, sphere.vertexArray(), GL_STATIC_DRAW);

	// (III.B) normal buffer for the Sphere
	glGenBuffers(1, &sphere_normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, sphere.normalArray(), GL_STATIC_DRAW);

	// (III.C) face buffer for the Sphere
	glGenBuffers(1, &sphere_face_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_face_buffer);
	int face_buffer_size = sizeof(Mesh3D::Face)*sphere_face_count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_buffer_size, sphere.faceArray(), GL_STATIC_DRAW);


	//Texture coordinate buffer for sphere
	Plane plane(MESH_SIZE);
	const Hcoord trans(1, 1, 0, 0);
	Matrix M = scale(0.5f, 0.5f, 1.0f)
		* translate(trans),
		N = scale(PI, PI * 0.5f, 1.0f)
		* translate(trans);

	glGenBuffers(1, &spheretextcoordbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, spheretextcoordbuffer);
	float *texcoord2 = new float[2 * plane.vertexCount()];
	Matrix Std2UnitSky =  M;
	float *polarVertices = new float[4 * plane.vertexCount()];
	Hcoord *pNormals = new Hcoord[plane.vertexCount()];
	for (int i = 0; i < plane.vertexCount(); ++i) {
		Hcoord uv = Std2UnitSky * plane.vertexArray()[i];
		texcoord2[2 * i + 0] = uv[0];
		texcoord2[2 * i + 1] = uv[1];

		Hcoord st = N*plane.vertexArray()[i];

		polarVertices[4 * i + 1] = sinf(st[1])*cosf(st[0]);
		polarVertices[4 * i + 0] = sinf(st[1])*sinf(st[0]);
		polarVertices[4 * i + 2] = cosf(st[1]);
		polarVertices[4 * i + 3] = 1;

		pNormals[i] = Hcoord(polarVertices[4 * i + 0], polarVertices[4 * i + 1], polarVertices[4 * i + 2], 0);
		//plane.vertexArray
	}
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float)*plane.vertexCount(), texcoord2, GL_STATIC_DRAW);
	delete[] texcoord2;

	// (III.A) vertex buffer for the Sphere
	glGenBuffers(1, &sphere_vertex_texturebuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_texturebuffer);
	int vertex_buffer_size1 = 4 * sizeof(float)*plane.vertexCount();
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size1, polarVertices, GL_STATIC_DRAW);

	// (III.B) normal buffer for the Sphere
	glGenBuffers(1, &sphere_normal_texturebuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_texturebuffer);
	int normal_buffer_size = sizeof(Hcoord)*plane.vertexCount();
	glBufferData(GL_ARRAY_BUFFER, normal_buffer_size, pNormals, GL_STATIC_DRAW);
	delete pNormals;
	delete polarVertices;

}

void Client::drawPlane()
{
	{
	// set shader uniform variable values
	glUseProgram(program);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
    glUniform1i(shadow_sampler, 1);
	glUniform3f(ulight_color, 1, 1, 1);
	glUniform4f(ulight_position, light.x, light.y, light.z, light.w);

	glUniformMatrix4fv(sview_matrix3, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix, 1, true, (float*)&V);
	glUniformMatrix4fv(B, 1, true, (float*)&Bmat);
	//const float RATE = 360.0f / 5.0f;
	const Hcoord AXIS(1, 0, 0, 0),
		CENTER(11, -11, -55, 0);
	Matrix R = rotate(0*time, AXIS),
		M = translate(CENTER)
		* R
		* scale(40.0f);
	glUniformMatrix4fv(umodel_matrix, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix, 1, true, (float*)&R);
	


	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal);
	glUniform3f(udiffuse_color,0.8,0.8,0.8);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);
	}
	
	// set shader uniform variable values
	glUseProgram(program1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
    glUniform1i(shadow_sampler1, 1);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(usampler, 0);
	glUniform3f(ulight_color1, 1, 1, 1);
	glUniform4f(ulight_position1, light.x, light.y, light.z, light.w);

	glUniformMatrix4fv(sview_matrix1, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix1, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix1, 1, true, (float*)&V);
	glUniformMatrix4fv(B, 1, true, (float*)&Bmat);
	const float RATE = 360.0f / 5.0f;
	const Hcoord AXIS(1, 0, 0, 0),
		CENTER(11, -11, -17, 0);
	Matrix R = rotate(RATE*time, AXIS),
		M = translate(CENTER)
		* R
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix1, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix1, 1, true, (float*)&R);
	


	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition1);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal1);

	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_buffer);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);

	//Add tiled plane
	glUniformMatrix4fv(sview_matrix1, 1, true, (float*)&sV);
	glUniformMatrix4fv(upersp_matrix1, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix1, 1, true, (float*)&V);
	glUniformMatrix4fv(B, 1, true, (float*)&Bmat);
	const Hcoord CENTER1(11, -6, -17, 0);
		M = translate(CENTER1)
		* scale(1.5f);
	glUniformMatrix4fv(umodel_matrix1, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix1, 1, true, (float*)&M);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition1);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal1, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal1);

	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer1);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_buffer);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);


	//Draw Skybox
	//+X
	glUseProgram(skyshader);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(usamplerSky, 0);
	glUniform3f(ulight_color2, 1, 1, 1);
	glUniform4f(ulight_position2, light.x, light.y, light.z, light.w);

	glUniformMatrix4fv(upersp_matrix2, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix2, 1, true, (float*)&V);
	const Hcoord posX(0, 0, -SKYBOX_DISTANCE, 0), Zaxis(0, 0, 1, 0);
	Matrix T = rotate(90.0f, Zaxis);
	M = translate(posX + eye)
		* T
		* scale(SKYBOX_DISTANCE + epsilon);
	glUniformMatrix4fv(umodel_matrix2, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix2, 1, true, (float*)&T);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition2);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal2);

	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	//glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, skytexture_buffer1);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);


	//-X
	glUniformMatrix4fv(upersp_matrix2, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix2, 1, true, (float*)&V);
	const Hcoord negX(0, 0, SKYBOX_DISTANCE, 0), Yaxis(0, 1, 0, 0);
	T = rotate(180.0f, Yaxis) * rotate(270.0f, Zaxis);
	M = translate(negX + eye)
		* T
		* scale(SKYBOX_DISTANCE + epsilon);
	glUniformMatrix4fv(umodel_matrix2, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix2, 1, true, (float*)&T);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition2);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal2);

	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	//glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, skytexture_buffer0);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);


	//+Z
	glUniformMatrix4fv(upersp_matrix2, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix2, 1, true, (float*)&V);
	const Hcoord posZ(SKYBOX_DISTANCE, 0, 0, 0);
	T = rotate(270.0f, Yaxis);
	M = translate(posZ + eye)
		* T
		* scale(SKYBOX_DISTANCE + epsilon);
	glUniformMatrix4fv(umodel_matrix2, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix2, 1, true, (float*)&T);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition2);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal2);

	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	//glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, skytexture_buffer2);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);



	//-Z
	glUniformMatrix4fv(upersp_matrix2, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix2, 1, true, (float*)&V);
	const Hcoord negZ(-SKYBOX_DISTANCE, 0, 0, 0);
	T = rotate(180.0f, AXIS) * rotate(90.0f, Yaxis);
	M = translate(negZ + eye)
		* T
		* scale(SKYBOX_DISTANCE + epsilon);
	glUniformMatrix4fv(umodel_matrix2, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix2, 1, true, (float*)&T);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition2);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal2);

	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	//glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, skytexture_buffer3);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);


	//+Y
	glUniformMatrix4fv(upersp_matrix2, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix2, 1, true, (float*)&V);
	const Hcoord posY(0, SKYBOX_DISTANCE, 0, 0);
	T = rotate(270.0f, Yaxis) * rotate(90.0f, AXIS);
	M = translate(posY + eye)
		* T
		* scale(SKYBOX_DISTANCE + epsilon);
	glUniformMatrix4fv(umodel_matrix2, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix2, 1, true, (float*)&T);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition2);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal2);

	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	//glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, skytexture_buffer4);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);


	//-Y
	glUniformMatrix4fv(upersp_matrix2, 1, true, (float*)&P);
	glUniformMatrix4fv(uview_matrix2, 1, true, (float*)&V);
	const Hcoord negY(0, -SKYBOX_DISTANCE, 0, 0);
	T = rotate(90.0f, Yaxis) * rotate(270.0f, AXIS);
	M = translate(negY + eye)
		* T
		* scale(SKYBOX_DISTANCE + epsilon);
	glUniformMatrix4fv(umodel_matrix2, 1, true, (float*)&M);
	glUniformMatrix4fv(unormal_matrix2, 1, true, (float*)&T);

	// set shader attributes
	glBindBuffer(GL_ARRAY_BUFFER, plane_vertex_buffer);
	glVertexAttribPointer(aposition2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(aposition2);
	glBindBuffer(GL_ARRAY_BUFFER, plane_normal_buffer);
	glVertexAttribPointer(anormal2, 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(anormal2);

	glBindBuffer(GL_ARRAY_BUFFER, skytext_buffer);
	glVertexAttribPointer(atexture_coord, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(atexture_coord);

	// select the texture to use
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(usamplerSky, 0);
	glBindTexture(GL_TEXTURE_2D, skytexture_buffer5);

	// draw the mesh
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_face_buffer);
	glDrawElements(GL_TRIANGLES, 3 * plane_face_count, GL_UNSIGNED_INT, 0);
	
}

void Client::drawSphere(int times, int othertimes, GLint shaderprog)
{
	// set shader uniform variable values
	glUseProgram(shaderprog);
	
	if(shaderprog == shadowshader)
	{
		
		
		P = perspective(fov, aspect, 0.1f);
		glUniformMatrix4fv(sview_matrix, 1, true, (float*)&sV);
		glUniformMatrix4fv(upersp_matrix3, 1, true, (float*)&P);	
		const float RATE = 0.0f;

		const Hcoord AXIS(0, 1, 0, 0),
			CENTER(-30 + (times * 10), -30 + (othertimes * 10), -40, 0);
		Matrix R = rotate(RATE*time, AXIS),
			M = translate(CENTER)
			* R
			* scale(1.5f);
		glUniformMatrix4fv(umodel_matrix3, 1, true, (float*)&M);

		// set shader attributes
		glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
		glVertexAttribPointer(aposition3, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(aposition3);

		// draw the mesh
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_face_buffer);
		glDrawElements(GL_TRIANGLES, 3 * sphere_face_count, GL_UNSIGNED_INT, 0);
	}
	else
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_texture_buffer);
		glUniform1i(shadow_sampler, 1);
		
		glUniformMatrix4fv(upersp_matrix, 1, true, (float*)&P);	
		glUniformMatrix4fv(uview_matrix, 1, true, (float*)&V);
		glUniformMatrix4fv(sview_matrix3, 1, true, (float*)&sV);
		const float RATE = 0.0f;

		const Hcoord AXIS(0, 1, 0, 0),
			CENTER(-30 + (times * 10), -30 + (othertimes * 10), -40, 0);
		Matrix R = rotate(RATE*time, AXIS),
			M = translate(CENTER)
			* R
			* scale(1.5f);
		glUniformMatrix4fv(umodel_matrix, 1, true, (float*)&M);
		glUniformMatrix4fv(unormal_matrix, 1, true, (float*)&R);
		glUniform3f(udiffuse_color, 1.0f - ((float)times/49.0f), 1.0f - ((float)othertimes / 49.0f), 0.0f);


		// set shader attributes
		glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
		glVertexAttribPointer(aposition, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(aposition);
		glBindBuffer(GL_ARRAY_BUFFER, sphere_normal_buffer);
		glVertexAttribPointer(anormal, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(anormal);

		// draw the mesh
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_face_buffer);
		glDrawElements(GL_TRIANGLES, 3 * sphere_face_count, GL_UNSIGNED_INT, 0);
	}
}






/////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////






int main(int argc, char *argv[]) {

  // SDL: initialize and create a window
  SDL_Init(SDL_INIT_VIDEO);
  const char *title = "Figure Scene";
  int width = 500,
      height = 500;
  SDL_Window *window = SDL_CreateWindow(title,SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,width,height,
                                        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
  SDL_GLContext context = SDL_GL_CreateContext(window);

  // GLEW: get function bindings (if possible)
  glewInit();
  if (!GLEW_VERSION_2_0) {
    cout << "needs OpenGL version 2.0 or better" << endl;
    return -1;
  }

  // animation loop
  bool done = false;
  Client *client = new Client();
  //set up frame buffer
  //client->createframebuffer();
  Uint32 ticks_last = SDL_GetTicks();
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          done = true;
          break;
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_ESCAPE)
            done = true;
          else
            client->keypress(event.key.keysym.sym);
          break;
        case SDL_WINDOWEVENT:
          if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            client->resize(event.window.data1,event.window.data2);
          break;
        case SDL_MOUSEMOTION:
          if ((event.motion.state&SDL_BUTTON_LMASK) != 0
              || (event.motion.state&SDL_BUTTON_RMASK) != 0)
            client->mousedrag(event.motion.xrel,event.motion.yrel,
                              (event.motion.state&SDL_BUTTON_LMASK) != 0);
		  break;
		case SDL_MOUSEWHEEL:
			client->mousewheel(event.wheel.y);
          break;
      }
    }
    Uint32 ticks = SDL_GetTicks();
    double dt = 0.001*(ticks - ticks_last);
    ticks_last = ticks;
	//first pass to shadow map frame buffer
	client->drawToShadowBuffer(dt);
	//draw to frame buffer
	client->drawToBuffer(dt);
	//draw to screen
    client->draw(dt);
    SDL_GL_SwapWindow(window);
  }

  // clean up
  delete client;
  SDL_GL_DeleteContext(context);
  SDL_Quit();
  return 0;
}

