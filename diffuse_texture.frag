#version 130

uniform vec3 light_color;
uniform sampler2D usampler;

in vec4 normal_vector;
in vec4 light_vector;
in vec2 vtexture_coord;

out vec4 frag_color;

void main(void) {
  vec3 diffuse_color = texture(usampler,vtexture_coord).xyz;
  vec4 m = normalize(normal_vector);
  vec4 L = normalize(light_vector);
  vec3 r = max(dot(m,L),0) * diffuse_color * light_color;
  frag_color = vec4(r,1);
}

