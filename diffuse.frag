#version 130

uniform vec3 light_color;
uniform vec3 diffuse_color;

in vec4 normal_vector;
in vec4 light_vector;

out vec4 frag_color;

void main(void) {
  vec4 m = normalize(normal_vector);
  vec4 L = normalize(light_vector);
  vec3 r = max(dot(m,L),0) * diffuse_color * light_color;
  frag_color = vec4(r,1);
}

