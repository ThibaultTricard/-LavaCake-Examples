#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
 
struct RayPayload {
	vec3 color;
	int depth;
	bool missed;
  bool lightRay;
};

layout(location = 0) rayPayloadInEXT RayPayload hitValue;
hitAttributeEXT vec3 pos;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, set = 0) buffer Vertices { float v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;

struct material{
	float diffR, diffG, diffB;
	float emR, emG, emB;
};

layout(binding = 5, set = 0) uniform materials 
{
	material[4] mats;
};

layout(binding = 6, set = 0) uniform samplesBuffer 
{
	vec4 dir[64];
	int frameNumber;
} samples;


layout(binding = 7, set = 0) uniform lightSamplesBuffer 
{
  vec4 pos[64];
} lightSamples;

struct Vertex
{
  vec3 pos;
  material mat;
};


Vertex fetch(uint indice){
	Vertex v;
	v.pos.x =vertices.v[indice * 4 ];
	v.pos.y =vertices.v[indice * 4 + 1];
	v.pos.z =vertices.v[indice * 4 + 2];
	int matIndex = int(vertices.v[indice * 4 + 3]);
	v.mat =mats[matIndex];
	return v;
}

void main(){

	const vec3 barycentricCoords = vec3(1.0f - pos.x - pos.y, pos.x, pos.y);

	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 =fetch(index.x);
	Vertex v1 =fetch(index.y);
	Vertex v2 =fetch(index.z);
  

	vec3 n = normalize(cross(v1.pos-v0.pos,v2.pos-v1.pos));
	vec3 tan = normalize(v0.pos-v2.pos);
	vec3 bitan = normalize(cross(n,tan));
	mat3 TBN = mat3(tan, bitan, n);

	float tmin = 0.001;
	float tmax = 10000.0;

	vec3 origin =  v0.pos * barycentricCoords.x  + v1.pos * barycentricCoords.y  + v2.pos * barycentricCoords.z;

	vec3 result = vec3(v0.mat.emR, v0.mat.emG, v0.mat.emB);
	vec3 diffuse = vec3(v0.mat.diffR, v0.mat.diffG, v0.mat.diffB);
	int depth = hitValue.depth;
	hitValue.depth = depth +1;

	hitValue.missed = false;

	vec3 irradiance = vec3(0);

	vec3 dirI = normalize(gl_WorldRayDirectionEXT);

	float phi = 0;

	if(!hitValue.lightRay && v0.mat.emR < 0.5){

		vec2 rand =  origin.xy + fract(sin((origin.z+ samples.frameNumber)*(91.3458)) * 47453.5453  );
		float r = fract(sin(dot(rand.xy ,vec2(12.9898,78.233))) * 43758.5453);

		vec3 LightPos = lightSamples.pos[int(r * 64.0)].xyz;;
		vec3 LightDir = normalize(LightPos - origin);

		hitValue.lightRay = true;
		traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, LightDir, tmax, 0);
		phi = dot(n, LightDir);
		vec3 L = hitValue.color* phi;

		hitValue.lightRay = false;

		if(depth < 8){
			vec3 s = samples.dir[int(r * 64.0)].xyz;

			vec3 d = TBN*s;

			traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, d, tmax, 0);
			float theta = dot(dirI, d);
			L= L + hitValue.color* theta;
		}


	
		hitValue.color = vec3(v0.mat.diffR * L.x, v0.mat.diffG * L.y, v0.mat.diffB * L.z) ;

		hitValue.depth = depth;
	}
	else{
		hitValue.color = vec3(v0.mat.emR,v0.mat.emG,v0.mat.emB);
	}
  //hitValue.color =  vec3(phi);

}
