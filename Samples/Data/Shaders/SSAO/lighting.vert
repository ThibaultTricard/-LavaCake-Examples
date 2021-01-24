#version 450

layout( location = 0 ) in vec4 app_position;
layout( location = 1 ) in vec2 app_UV;


layout( location = 0 ) out vec2 uv;

void main() {

	uv = app_UV;
	gl_Position = app_position;
}