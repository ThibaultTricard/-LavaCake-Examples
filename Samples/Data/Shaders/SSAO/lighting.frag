#version 450

layout( location = 0 ) in vec2 uv;

layout( location = 0 ) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gAlbedo;
layout(set = 0, binding = 3) uniform sampler2D SSAO;


layout( set = 0, binding = 4 ) uniform UniformBuffer {
 	vec3 LightPosition;
 	vec3 LightColor;
};


void main() {
	vec3 FragPos = texture(gPosition, uv).rgb;
    vec3 Normal = texture(gNormal, uv).rgb;
    vec3 Diffuse = texture(gAlbedo, uv).rgb;

    float ssao = texture(SSAO, uv).r;

	vec3 ambient = vec3(0.3 * Diffuse * ssao); // here we add occlusion factor
    vec3 lighting  = ambient; 
    vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0) in view-space
    // diffuse
    vec3 lightDir = normalize(LightPosition - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * LightColor;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = LightColor * spec;
    // attenuation
    //float dist = length(light.Position - FragPos);
    //float attenuation = 1.0 / (1.0 + light.Linear * dist + light.Quadratic * dist * dist);
    //diffuse  *= attenuation;
    //specular *= attenuation;
    lighting += diffuse + specular;

    FragColor = vec4(lighting,1.0);
}