#version 430 core

in vec3 f_viewVertex ;
in vec3 f_uv ;

layout (location = 0) out vec4 fragColor ;
layout(location = 2) uniform int pixelProcessId;
layout(location = 4) uniform sampler2D albedoTexture;

layout(binding = 10) uniform sampler2D airplane_texture;
layout(binding = 11) uniform sampler2D rockTexture;
layout(binding = 24) uniform sampler2DArray albedoTextureArray;

//blinn phong shading
vec4 la = vec4(0.2, 0.2, 0.2, 1.0);
vec4 ld = vec4(0.64, 0.64, 0.64, 1.0);
vec4 ls = vec4(0.16, 0.16, 0.16, 1.0);



in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
} fs_in;

vec4 blinnPhong(vec4 texel, vec3 ka, vec3 kd, vec3 ks, float shininess) {
	// output color
	vec4 outColor = vec4(0.0, 0.0, 0.0, 1.0);
	//fragColor = texel;
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);
	vec3 H = normalize(L + V); //halfway
	//ambient
	outColor += la * vec4(ka, 1.0);

	//diffuse
	//outColor += white_Id * vec4(kd, 1.0) + max(dot(N, L), 0.0);
	outColor += max(dot(N, L), 0.0) * texel * ld;

	//specular
	float spec = pow(max(dot(N, H), 0.0), shininess);
	outColor += ls * vec4(ks, 1.0) * spec + ls * texel * spec;

	return outColor;
}

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
}

void main(){	
	if(pixelProcessId == 5){
		pureColor() ;
	}
	else if(pixelProcessId == 7){
		terrainPass() ;
	}
	else if (pixelProcessId == 10) { //draw airplane
		vec4 texel = texture(airplane_texture, f_uv.xy);
		vec3 ka = texel.xyz;
		vec3 kd = texel.xyz;
		vec3 ks = vec3(1.0, 1.0, 1.0);
		float shininess = 32.0;
		
		fragColor = blinnPhong(texel, ka, kd, ks, shininess); //texel改成用texture取來就會是正常的blinn phong
	}
	else if (pixelProcessId == 11) { //draw rock
		vec4 texel = texture(rockTexture, f_uv.xy);
		vec3 ka = texel.xyz;
		vec3 kd = texel.xyz;
		vec3 ks = vec3(1.0, 1.0, 1.0);
		float shininess = 32.0;

		fragColor = blinnPhong(texel, ka, kd, ks, shininess); // texel改成用texture取來就會是正常的blinn phong
	}
	else if (pixelProcessId == 12) { //draw grass and building
        vec4 texel = texture(albedoTextureArray, f_uv);
		if (texel.a < 0.3)
			discard;
		vec3 ka = texel.xyz;
		vec3 kd = texel.xyz;
		vec3 ks = vec3(0.0, 0.0, 0.0);
		float shininess = 1.0;

		fragColor = blinnPhong(texel, ka, kd, ks, shininess); // texel改成用texture取來就會是正常的blinn phong
	}
	else{
		pureColor() ;
	}
}
