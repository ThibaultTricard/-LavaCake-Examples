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
} samples;

layout(binding = 7, set = 0) uniform lightSamplesBuffer 
{
  vec4 dir[64];
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

uint SAMPLE_NUMBER = 2;

void main()
{
  const vec3 barycentricCoords = vec3(1.0f - pos.x - pos.y, pos.x, pos.y);
  
  ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

  Vertex v0 =fetch(index.x);
  Vertex v1 =fetch(index.y);
  Vertex v2 =fetch(index.z);


  

  vec3 n = normalize(cross(v1.pos-v0.pos,v2.pos-v1.pos));
  vec3 tan = normalize(v1.pos-v0.pos);
  vec3 bitan = normalize(cross(n,tan));

  float tmin = 0.001;
  float tmax = 10000.0;

  vec3 origin =  v0.pos * barycentricCoords.x  + v1.pos * barycentricCoords.y  + v2.pos * barycentricCoords.z;

  uint shift = uint(origin.x * 23465.0 + origin.y * 13.0 + origin.z * 563.0) % 64;

  vec3 result = vec3(v0.mat.emR, v0.mat.emG, v0.mat.emB);
  vec3 diffuse = vec3(v0.mat.diffR, v0.mat.diffG, v0.mat.diffB);
  int depth = hitValue.depth;
  hitValue.depth = depth +1;

  hitValue.missed = false;

  vec3 irradiance = vec3(0);


  if(!hitValue.lightRay && v0.mat.emR < 0.5){

    if(depth < 4)
    {
      for(int i = 0; i < 1; i++){
        vec3 rayDir = normalize(lightSamples.dir[i].xyz - origin);

        float theta = dot(rayDir, n);
        hitValue.lightRay = true;
        traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, rayDir, tmax, 0);

        hitValue.lightRay = false;

        irradiance+= hitValue.color * theta;
      }
      irradiance  =  irradiance / 1.0;
  	  
      result = irradiance;

      int lostRays = 0;
      vec3 indirect = vec3(0.0);
      for(int i = 0; i < SAMPLE_NUMBER; i++)
  	  {


  	  	uint s = (shift* (i+1) *(depth+1)) % 64;

  	  	vec3 rayDir = normalize(tan * samples.dir[s].x + bitan * samples.dir[s].y + n * samples.dir[s].z);

  	  	traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, rayDir, tmax, 0);

  	  	float theta = dot(rayDir, n);

        if(hitValue.missed){
          lostRays++;
          hitValue.missed = false;
        }
  	  	indirect+= hitValue.color* theta;

  	  }

      if(SAMPLE_NUMBER - lostRays != 0){
        indirect = indirect / float(SAMPLE_NUMBER - lostRays);
      }

      result = (result+indirect)*diffuse;

     }
   }
  hitValue.color = result;
  hitValue.depth = depth;

  //hitValue =  barycentricCoords;

}
