#version 430

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main(void)
{
	TexCoords = texCoords;
	gl_Position = lightSpaceMatrix * model * position;
}
