#version 450

layout( location = 0 ) in vec4 app_position;
layout( location = 1 ) in vec3 app_color;

layout( location = 0 ) out vec3 vert_color;

layout( push_constant ) uniform LightParameters {
  mat4 transform;
} model;


void main() {
  gl_Position = model.transform * app_position;
  vert_color = app_color;
}