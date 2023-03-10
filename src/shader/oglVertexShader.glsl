#version 430 core

layout(location=0) in vec3 v_vertex;
layout(location=1) in vec3 v_normal ;
layout(location=2) in vec3 v_uv ;
layout(location = 3) in float index;
layout(location = 4) in vec3 v_tangent;


struct RawInstanceProperties {
	vec4 position;
	vec4 boundSphere;

	mat4 rotationMatrix;
};

layout(std430, binding = 1) buffer InstanceData {
	RawInstanceProperties rawInstanceProps[];
};

out vec3 f_viewVertex ;
out vec3 f_uv ;

out VS_OUT{
	vec3 N;
	vec3 L;
	vec3 V;
	vec3 T;
	vec4 lightSpacePos;
}vs_out;



layout(location = 0) uniform mat4 modelMat ;
layout(location = 7) uniform mat4 viewMat ;
layout(location = 8) uniform mat4 projMat ;

layout(location = 1) uniform int vertexProcessIdx ;
layout(location = 5) uniform sampler2D elevationMap ;
layout(location = 6) uniform sampler2D normalMap ;

layout(location = 9) uniform mat4 terrainVToUVMat;

layout(location = 20) uniform mat4 lightSpaceMatrix;

//blinn phong shading
uniform vec3 light_pos = vec3(0.4, 0.5, 0.8);

void commonProcess(){//?n?諸
	/*
	vec4 worldVertex = modelMat * vec4(v_vertex, 1.0) ;
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0) ;

	vec4 viewVertex = viewMat * worldVertex ;
	vec4 viewNormal = viewMat * worldNormal ;
	
	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv ;

	gl_Position = projMat * viewVertex ;*/
	//vec4 worldVertex = modelMat * vec4(v_vertex + v_worldPosOffset.xyz, 1.0);
	vec4 worldVertex = modelMat * vec4(v_vertex , 1.0);
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0);

	vec4 viewVertex = viewMat * worldVertex;
	vec4 viewNormal = viewMat * worldNormal;

	vs_out.V = worldVertex.xyz;
	vs_out.N = worldNormal.xyz;

	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv;
	gl_Position = projMat * viewVertex;

	vs_out.lightSpacePos = lightSpaceMatrix * worldVertex;
}

void grass_building_process() {
	//mat4 rotationMatrix = rawInstanceProps[int(v_worldPosOffset.w)].rotationMatrix;
	mat4 rotationMatrix = rawInstanceProps[int(index)].rotationMatrix;
	//mat4 rotationMatrix = rawInstanceProps[gl_InstanceID].rotationMatrix;
	//mat4 rotationMatrix = mat4(vec4(0, 0, -1, 0), vec4(0, 1, 0, 0), vec4(1, 0, 0, 0), vec4(0, 0, 0, 1));

	vec4 v = rotationMatrix * vec4(v_vertex, 1.0);

	vec4 worldVertex =  modelMat * vec4(v.xyz + rawInstanceProps[int(index)].position.xyz, 1.0) ;
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0) ;

	vec4 viewVertex = viewMat  * worldVertex;
	vec4 viewNormal = viewMat  * worldNormal;

	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv;

	vs_out.V = worldVertex.xyz;
	vs_out.N = worldNormal.xyz;

	gl_Position = projMat * viewVertex;

	//?o???i?H???P?_?O?_?O?p??
	if (v_uv.z != 0)
		vs_out.lightSpacePos = lightSpaceMatrix * worldVertex;
}

void terrainProcess(){
	vec4 worldV = modelMat * vec4(v_vertex, 1.0) ;
	// calculate uv
	vec4 uv = terrainVToUVMat * worldV ;
	uv.y = uv.z ;
	// get height from map
	float h = texture(elevationMap, uv.xy).r ;
	worldV.y = h;		
	// get normal from map
	vec4 normalTex = texture(normalMap, uv.xy) ;
	// [0, 1] -> [-1, 1]
	normalTex = normalTex * 2.0 - 1.0 ;
		
	// transformation	
	vec4 viewVertex = viewMat * worldV ;
	vec4 viewNormal = viewMat * vec4(normalTex.rgb, 0) ;	
	
	f_viewVertex = viewVertex.xyz;
	f_uv = uv.xyz ;


	vs_out.N = normalTex.rgb;
	vs_out.V = worldV.xyz;

	gl_Position = projMat * viewVertex ;
}

void main(){
	vs_out.lightSpacePos = vec4(1);

	if(vertexProcessIdx == 0){
		commonProcess() ;
	}
	else if(vertexProcessIdx == 3){
		terrainProcess() ;
	}
	else if (vertexProcessIdx == 10) { //draw airplane
		commonProcess();
		//grass_building_process();
	}
	else if (vertexProcessIdx == 11) { //draw rock
		commonProcess();
		vs_out.T = v_tangent;
	}
	else if (vertexProcessIdx == 12) { //draw grass and building
		grass_building_process();
	}
	else{
		grass_building_process();
	}	
}
