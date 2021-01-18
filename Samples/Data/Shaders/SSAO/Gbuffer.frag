#version 450

layout( location = 0 ) in vec3 vert_position;
layout( location = 1 ) in vec3 vert_normal;

layout( location = 0 ) out vec4 O_position;
layout( location = 1 ) out vec4 O_normal;
layout( location = 2 ) out vec4 O_albedo;

void main() {
  O_position	= vec4(vert_position.xy, vert_position.z, 1.0);
  O_normal = vec4(normalize( vert_normal ),0.0);
  O_albedo = vec4(1.0);
}
