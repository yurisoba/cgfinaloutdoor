#version 430

in vec2 TexCoords;

uniform sampler2D diffuseTexture;

void main(void)
{
	//�i�H����discard transparent part
	//float alpha = texture(diffuseTexture, TexCoords).a;
	//if (alpha < 0.4)
	//	discard;
}
