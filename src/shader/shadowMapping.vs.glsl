#version 430

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoords;
layout(location = 3) in float index;


struct RawInstanceProperties {
	vec4 position;
	vec4 boundSphere;
	mat4 rotationMatrix;
};

layout(std430, binding = 1) buffer InstanceData {
	RawInstanceProperties rawInstanceProps[];
};


out vec2 TexCoords;

uniform int mode;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main(void)
{
	TexCoords = texCoords;
	if (mode == 0)
		gl_Position = lightSpaceMatrix * model * position;
	else {
		mat4 rotationMatrix = rawInstanceProps[int(index)].rotationMatrix;
		vec4 v = rotationMatrix * vec4(position.xyz, 1.0);
		gl_Position =  lightSpaceMatrix * model * vec4(v.xyz + rawInstanceProps[int(index)].position.xyz, 1.0) ;

	}
}
