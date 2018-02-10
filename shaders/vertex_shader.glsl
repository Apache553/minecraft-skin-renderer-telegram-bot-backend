
#version 330

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 texturePosition;

out vec2 textureCoord;

void main()
{
	gl_Position=vec4(vertexPosition,0.0f,1.0f);
	textureCoord=texturePosition;
}