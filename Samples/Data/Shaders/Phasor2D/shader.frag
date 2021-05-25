#version 450

layout( set = 0, binding = 0 ,r32f) uniform imageBuffer phasor;
layout( set = 0, binding = 1 ) uniform UniformBuffer {
  uint sizeX;
  uint sizeY;
};

layout( location = 0 ) in vec2 texcoord;

layout( location = 0 ) out vec4 frag_color;

void main() {
  vec2 coord = texcoord;
  coord = coord  * vec2(sizeX, sizeY) ;
  float p = imageLoad(phasor,int(int(coord.x) + int(coord.y) * sizeX)).x ;
  frag_color = vec4(p,0,0,0);
}
