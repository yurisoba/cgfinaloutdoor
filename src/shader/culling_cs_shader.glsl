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
	vec4 position;
	//vec4 radians;
};

/*the buffer for storing “whole” instance position and other necessary information*/
layout(std430, binding = 1) buffer InstanceData {
	RawInstanceProperties rawInstanceProps[];
};

/*the buffer for storing “visible” instance position*/
layout(std430, binding = 2) buffer CurrValidInstanceData {
	InstanceProperties currValidInstanceProps[];
};
layout(location = 6) uniform int numMaxInstance;
layout(location = 7) uniform mat4 viewProjMat; //player view projection


void main() {
	uint idx = gl_GlobalInvocationID.x;
	// discarding invalid array-access
	if (idx >= numMaxInstance) {
		return;
	}

	//這邊好像一樣只要判斷position就好
	// translate the position to clip space
	//mat4 modelMat = mat4(rawInstanceProps[idx].matCol0, rawInstanceProps[idx].matCol1,
	//	rawInstanceProps[idx].matCol2, rawInstanceProps[idx].matCol3);

	vec4 clipSpaceV = viewProjMat * vec4(rawInstanceProps[idx].position.xyz, 1.0);
	clipSpaceV = clipSpaceV / clipSpaceV.w;
	// determine if it is culled
	bool frustumCulled = (clipSpaceV.x < -1.0) || (clipSpaceV.x > 1.0) || (clipSpaceV.y < -1.0) ||
		(clipSpaceV.y > 1.0) || (clipSpaceV.z < -1.0) || (clipSpaceV.z > 1.0);

	if (frustumCulled == false) {
		// get UNIQUE buffer location for assigning the instance data
		// it also updates instanceCount

		unsigned int type = unsigned int(rawInstanceProps[idx].position.w);
		unsigned int offset = commands[type].baseInstance;
		//if (idx >= 155304 && idx < 155304 + 1010) {
		const unsigned int UNIQUE_IDX = atomicAdd(commands[type].instanceCount, 1);
		// put data into valid-instance buffer

		//currValidInstanceProps[offset + UNIQUE_IDX].position = rawInstanceProps[idx].position; //注意這邊有改成position
		currValidInstanceProps[offset + UNIQUE_IDX].position.xyz = rawInstanceProps[idx].position.xyz; //改成只要存那個東西的idx就好
		float idx_float = float(idx);
		currValidInstanceProps[offset + UNIQUE_IDX].position.w = idx_float;
	}

}