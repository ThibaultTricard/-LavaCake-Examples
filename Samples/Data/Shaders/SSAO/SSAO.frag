#version 450

layout( location = 0 ) in vec2 uv;

layout( location = 0 ) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gDepth;

layout(set = 0, binding = 4) uniform UniformBuffer{
	vec4 samples[64];
	mat4 projection;
    float radius;
    vec4 seed[16];
};


int kernelSize = 64;

float bias = 0.025;

void main() {

    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, uv).xyz;
    vec3 normal = normalize(texture(gNormal, uv).rgb);
    //vec3 randomVec = normalize(texture(texNoise, uv * noiseScale).xyz);
    
    ivec2 t = textureSize(gNormal,0);
    ivec2 S = ivec2(uv * t);
    int s = (S.x %4) * 4 + (S.y%4);

    vec3 randomVec= seed[s].xyz;

    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * samples[i].xyz; // from tangent to view-space
        samplePos = fragPos + samplePos * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    FragColor = vec4(occlusion);
}