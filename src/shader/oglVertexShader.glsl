#version 430 core

layout(location=0) in vec3 v_vertex;
layout(location=1) in vec3 v_normal ;
layout(location=2) in vec3 v_uv ;
layout(location = 3) in vec4 v_worldPosOffset; //要改成idx
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
}vs_out;



layout(location = 0) uniform mat4 modelMat ;
layout(location = 7) uniform mat4 viewMat ;
layout(location = 8) uniform mat4 projMat ;

layout(location = 1) uniform int vertexProcessIdx ;
layout(location = 5) uniform sampler2D elevationMap ;
layout(location = 6) uniform sampler2D normalMap ;

layout(location = 9) uniform mat4 terrainVToUVMat;

//blinn phong shading
uniform vec3 light_pos = vec3(0.4, 0.5, 0.8);


void commonProcess(){//要改的
	/*
	vec4 worldVertex = modelMat * vec4(v_vertex, 1.0) ;
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0) ;

	vec4 viewVertex = viewMat * worldVertex ;
	vec4 viewNormal = viewMat * worldNormal ;
	
	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv ;

	gl_Position = projMat * viewVertex ;*/

	vec4 worldVertex = modelMat * vec4(v_vertex + v_worldPosOffset.xyz, 1.0);
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0);

	vec4 viewVertex = viewMat * worldVertex;
	vec4 viewNormal = viewMat * worldNormal;

	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv;

	gl_Position = projMat * viewVertex;
}

void grass_building_process() {
	mat4 rotationMatrix = rawInstanceProps[int(v_worldPosOffset.w)].rotationMatrix;
	//mat4 rotationMatrix = rawInstanceProps[gl_InstanceID].rotationMatrix;
	//mat4 rotationMatrix = mat4(vec4(0, 0, -1, 0), vec4(0, 1, 0, 0), vec4(1, 0, 0, 0), vec4(0, 0, 0, 1));

	vec4 v = rotationMatrix * vec4(v_vertex, 1.0);

	vec4 worldVertex =  modelMat * vec4(v.xyz + v_worldPosOffset.xyz, 1.0) ;
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0) ;

	vec4 viewVertex = viewMat  * worldVertex;
	vec4 viewNormal = viewMat  * worldNormal;

	f_viewVertex = viewVertex.xyz;
	f_uv = v_uv;

	gl_Position = projMat * viewVertex;
}

void blinnPhong() {
	mat4 mv_matrix = viewMat * modelMat;
	vec4 P = mv_matrix * vec4(v_vertex, 1.0);
	
	vs_out.N = mat3(mv_matrix) * v_normal;

	vs_out.L = light_pos - P.xyz;

	vs_out.V = -P.xyz;
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
	vs_out.V = -viewVertex.xyz;

	gl_Position = projMat * viewVertex ;
}

void main(){
	if(vertexProcessIdx == 0){
		commonProcess() ;
	}
	else if(vertexProcessIdx == 3){
		terrainProcess() ;
	}
	else if (vertexProcessIdx == 10) { //draw airplane
		blinnPhong();
		commonProcess();
		//grass_building_process();
	}
	else if (vertexProcessIdx == 11) { //draw rock
		blinnPhong();
		commonProcess();
		vs_out.T = v_tangent;
	}
	else if (vertexProcessIdx == 12) { //draw grass and building
		blinnPhong();
		grass_building_process();
	}
	else{
		grass_building_process();
	}	
}