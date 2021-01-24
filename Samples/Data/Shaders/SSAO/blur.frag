#version 450

layout( location = 0 ) in vec2 uv;

layout( location = 0 ) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D SSAO;

void main() {
	vec2 texelSize = 1.0/ vec2(textureSize(SSAO,0));

	float res = 0.0;

	for(int i= -2 ; i<=2; i++){
		for(int j= -2 ; j<=2; j++){
			vec2 offset = vec2(float(i),float(j)) * texelSize;
			res += texture(SSAO, uv + offset).x;
		}
	}
	res = res/(5.0*5.0);
	FragColor = vec4(res);
}