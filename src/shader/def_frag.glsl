#version 430 core

layout (location = 0) out vec4 fragColor;
layout (binding = 0) uniform sampler2D g_col;
layout (binding = 1) uniform sampler2D g_nom;
layout (binding = 2) uniform sampler2D g_pos;
layout (binding = 3) uniform sampler2D g_contribs;

uniform vec3 light_pos = vec3(0.4, 0.5, 0.8);

uniform uint features;

//blinn phong shading
uniform vec4 la = vec4(0.2, 0.2, 0.2, 1.0);
uniform vec4 ld = vec4(0.64, 0.64, 0.64, 1.0);
uniform vec4 ls = vec4(0.16, 0.16, 0.16, 1.0);
uniform vec4 ksss[] = {
	vec4(0.0),
	vec4(1.0, 1.0, 1.0, 32.0),
	vec4(0.0, 0.0, 0.0, 1.0),
};

void blinnPhong() {
	float shadow = texelFetch(g_contribs, ivec2(gl_FragCoord.xy), 0).r;
	vec3 N = texelFetch(g_nom, ivec2(gl_FragCoord.xy), 0).rgb; // world space normal
	vec4 gp = texelFetch(g_pos, ivec2(gl_FragCoord.xy), 0);
	vec3 ws = gp.rgb; // world space coordinates
	vec3 L = light_pos;

	/*
	N = (viewMat * vec4(N, 0.0)).rgb;
	ws = (viewMat * vec4(ws, 1.0)).rgb;
	L = (viewMat * vec4(L, 1.0)).rgb;
	N = normalize(N);
	*/


	vec3 V = normalize(-ws);
	vec3 ks = ksss[uint(gp.w)].rgb;
	float shininess = ksss[uint(gp.w)].a;

	// output color
	vec4 outColor = vec4(0.0, 0.0, 0.0, 1.0);

	float dist = length(L);
	vec3 H = (normalize(L) + V); //halfway
	L = normalize(L);

	//ambient
	vec3 ka = fragColor.xyz;
	outColor += la * vec4(ka, 1.0);

	//diffuse
	//outColor += white_Id * vec4(kd, 1.0) + max(dot(N, L), 0.0);
	outColor += max(dot(N, L), 0.0) * fragColor * ld * (1.0 - shadow);

	//specular
	float spec = pow(max(dot(N, H), 0.0), shininess);
	outColor += ls * vec4(ks, 1.0) * spec + ls * fragColor * spec * (1.0 - shadow);

	fragColor = outColor;
}

#define FEAT(id) ((features & (1 << id)) != 0)

void main(void)
{
	fragColor = texelFetch(g_col, ivec2(gl_FragCoord.xy), 0);

	if (FEAT(0))
		blinnPhong();

	fragColor = vec4(pow(fragColor.rgb, vec3(0.5)), fragColor.a);

	if (FEAT(13))		// ws_vtx
		fragColor = vec4(normalize(texelFetch(g_pos, ivec2(gl_FragCoord.xy), 0).rgb) * 0.5 + 0.5, 1.0);
	if (FEAT(14))		// normal
		fragColor = vec4(texelFetch(g_nom, ivec2(gl_FragCoord.xy), 0).rgb * 0.5 + 0.5, 1.0);
	if (FEAT(15)) {		// specular
		vec4 kss = ksss[uint(texelFetch(g_pos, ivec2(gl_FragCoord.xy), 0).w)];
		fragColor = vec4(normalize(kss.rgb) * 0.5 + 0.5, 1.0);
	}

}
