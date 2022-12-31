#version 410 core

layout(location = 0) in vec3 iv3vertex;

void main(void)
{
	gl_Position = vec4(iv3vertex, 1);
}