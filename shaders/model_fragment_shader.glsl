
#version 400

in vec2 textureCoord;

out vec4 outColor;

void main()
{
	outColor = vec4(textureCoord,0.0f,1.0f);
}