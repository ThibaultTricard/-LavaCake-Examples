#version 450

layout( set = 0, binding = 0 ,rgba32f) uniform image2D result;
layout( set = 0, binding = 1 ) uniform UniformBuffer {
  uvec2 dim;
};

layout( location = 0 ) in vec2 texcoord;

layout( location = 0 ) out vec4 frag_color;

void main() {
  vec2 coord = texcoord * vec2(dim);
  frag_color = imageLoad(result,ivec2(int(coord.x), int(coord.y))) ;
}
