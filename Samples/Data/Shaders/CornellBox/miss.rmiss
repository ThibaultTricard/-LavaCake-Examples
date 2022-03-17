#version 460
#extension GL_EXT_ray_tracing : enable
struct RayPayload {
	vec3 color;
	int depth;
	bool missed;
  	bool lightRay;
};
layout(location = 0) rayPayloadInEXT RayPayload hitValue;

void main()
{
    hitValue.color = vec3(0.0, 0.0, 0.0);
    hitValue.missed = true;
}