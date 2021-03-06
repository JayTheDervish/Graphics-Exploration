#version 130

attribute vec4 position;
attribute vec4 normal;

uniform mat4 persp_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat4 normal_matrix;
uniform vec4 light_position;

out vec4 normal_vector;
out vec4 light_vector;

void main() {
  vec4 P = model_matrix * position;
  gl_Position = persp_matrix * view_matrix * P;
  normal_vector = normal_matrix * normal;
  light_vector = light_position - P;
}

