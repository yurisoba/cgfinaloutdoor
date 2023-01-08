#version 430 core

in vec3 f_viewVertex ;
in vec3 f_uv ;

layout (location = 0) out vec4 fragColor ;
layout (location = 1) out vec4 fragNormal;
layout (location = 2) out vec4 ws_coords;
layout(location = 2) uniform int pixelProcessId;
layout(location = 4) uniform sampler2D albedoTexture;

layout(binding = 10) uniform sampler2D airplane_texture;
layout(binding = 11) uniform sampler2D rockTexture;
layout(binding = 12) uniform sampler2D rockNormal;
layout(binding = 24) uniform sampler2DArray albedoTextureArray;


in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec3 T;
} fs_in;

uniform uint features;
#define FEAT(id) ((features & (1 << id)) != 0)

vec4 withFog(vec4 color){
	const vec4 FOG_COLOR = vec4(0.0, 0.0, 0.0, 1) ;
	const float MAX_DIST = 400.0 ;
	const float MIN_DIST = 350.0 ;
	
	float dis = length(f_viewVertex) ;
	float fogFactor = (MAX_DIST - dis) / (MAX_DIST - MIN_DIST) ;
	fogFactor = clamp(fogFactor, 0.0, 1.0) ;
	fogFactor = fogFactor * fogFactor ;
	
	vec4 colorWithFog = mix(FOG_COLOR, color, fogFactor) ;
	return colorWithFog ;
}


void terrainPass(){
	vec4 texel = texture(albedoTexture, f_uv.xy) ;
	fragColor = withFog(texel); 
	fragColor.a = 1.0;	
}

void pureColor(){
	fragColor = withFog(vec4(1.0, 0.0, 0.0, 1.0)) ;
	fragNormal = vec4(1.0);
}

void main(){
    int kss_idx = 0;

	fragNormal = vec4(fs_in.N, 0.0);

	if(pixelProcessId == 5){
		pureColor() ;
	}
	else if(pixelProcessId == 7){
		terrainPass() ;
	}
	else if (pixelProcessId == 10) { //draw airplane
		vec4 texel = texture(airplane_texture, f_uv.xy);
		kss_idx = 1;
		fragColor = texel;
	}
	else if (pixelProcessId == 11) { //draw rock
		vec4 texel = texture(rockTexture, f_uv.xy);
		kss_idx = 1;
		fragColor = withFog(texel);

		if (FEAT(1)) {
			vec3 N = normalize(fs_in.N);
			vec3 T = normalize(fs_in.T);
			vec3 B = normalize(cross(N, T));
			mat3 TBN = mat3(T, B, N);
			vec3 nm = texture(rockNormal, f_uv.xy).xyz * 2.0 - 1.0;
			fragNormal = vec4(normalize(TBN * normalize(nm)), 0.0);
		}
	}
	else if (pixelProcessId == 12) { //draw grass and building
        vec4 texel = texture(albedoTextureArray, f_uv);
		if (texel.a < 0.3)
			discard;
		kss_idx = 2;
		fragColor = withFog(texel);
		//fragColor = texel;
	}
	else{
		pureColor() ;
	}

	ws_coords = vec4(fs_in.V, kss_idx);
}
