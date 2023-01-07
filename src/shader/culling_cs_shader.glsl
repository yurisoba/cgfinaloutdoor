#version 430 core
layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;
struct DrawCommand {
	uint count;
	uint instanceCount;
	uint firstIndex;
	uint baseVertex;
	uint baseInstance;
};
//the SSBO for storing draw commands
layout(std430, binding = 3) buffer DrawCommandsBlock {
	DrawCommand commands[];
};

struct RawInstanceProperties {
	vec4 position; 
	vec4 boundSphere;

	mat4 rotationMatrix;
};
struct InstanceProperties {
	//vec4 position;
	float index;
	//vec4 radians;
};

/*the buffer for storing ��whole�� instance position and other necessary information*/
layout(std430, binding = 1) buffer InstanceData {
	RawInstanceProperties rawInstanceProps[];
};

/*the buffer for storing ��visible�� instance position*/
layout(std430, binding = 2) buffer CurrValidInstanceData {
	InstanceProperties currValidInstanceProps[];
};
layout(location = 6) uniform int numMaxInstance;
layout(location = 7) uniform mat4 ViewMat; //player view mat
layout(location = 8) uniform mat4 ProjMat; //player proj mat


void main() {
	uint idx = gl_GlobalInvocationID.x;
	// discarding invalid array-access
	if (idx >= numMaxInstance) {
		return;
	}

	//�o��n���@�˥u�n�P�_position�N�n
	// translate the position to clip space
	//mat4 modelMat = mat4(rawInstanceProps[idx].matCol0, rawInstanceProps[idx].matCol1,
	//	rawInstanceProps[idx].matCol2, rawInstanceProps[idx].matCol3);

	//vec4 clipSpaceV = ProjMat * ViewMat * vec4(rawInstanceProps[idx].position.xyz +
	//	rawInstanceProps[idx].boundSphere.xyz, 1.0);

	vec4 worldSpaceV = ViewMat * vec4(rawInstanceProps[idx].position.xyz +
		rawInstanceProps[idx].boundSphere.xyz, 1.0);

	float world_dist = rawInstanceProps[idx].boundSphere.w;

	
	bool world_Culled = (worldSpaceV.x + world_dist < -400.0) || (worldSpaceV.x - world_dist > 400.0) ||
		(worldSpaceV.y + world_dist < -400.0) || (worldSpaceV.y - world_dist > 400.0) ||
		(worldSpaceV.z + world_dist < -380.0) || (worldSpaceV.z - world_dist > 1.0);


	vec4 clipSpaceV = ProjMat * ViewMat * vec4(rawInstanceProps[idx].position.xyz +
		rawInstanceProps[idx].boundSphere.xyz, 1.0);
	float dist = rawInstanceProps[idx].boundSphere.w / clipSpaceV.w;
	clipSpaceV = clipSpaceV / clipSpaceV.w;
	// determine if it is culled
	bool frustumCulled = (clipSpaceV.x + dist*2  < -1.0) || (clipSpaceV.x - dist*2 > 1.0) || 
		(clipSpaceV.y + dist*3 < -1.0) || (clipSpaceV.y - dist*3 > 1.0) ||
		(clipSpaceV.z + dist*2 < -1.0) || (clipSpaceV.z - dist > 0.9963);

	if (world_Culled == false && frustumCulled == false) {
		// get UNIQUE buffer location for assigning the instance data
		// it also updates instanceCount

		unsigned int type = unsigned int(rawInstanceProps[idx].position.w);
		unsigned int offset = commands[type].baseInstance;
		//if (idx >= 155304 && idx < 155304 + 1010) {
		const unsigned int UNIQUE_IDX = atomicAdd(commands[type].instanceCount, 1);
		// put data into valid-instance buffer

		//currValidInstanceProps[offset + UNIQUE_IDX].position = rawInstanceProps[idx].position; //�`�N�o�䦳�令position
		
		//currValidInstanceProps[offset + UNIQUE_IDX].position.xyz = rawInstanceProps[idx].position.xyz; //�令�u�n�s���ӪF�誺idx�N�n
		//float idx_float = float(idx);
		//currValidInstanceProps[offset + UNIQUE_IDX].position.w = idx_float;

		currValidInstanceProps[offset + UNIQUE_IDX].index = float(idx);
	}

}