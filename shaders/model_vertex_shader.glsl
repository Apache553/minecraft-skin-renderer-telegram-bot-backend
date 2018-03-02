
#version 330

layout (location = 0) in vec4 vertexPosition;
layout (location = 1) in vec2 texturePosition;

uniform mat4 viewMatrix;
uniform mat4 projectMatrix;

out vec2 textureCoord;

void main()
{
	gl_Position = projectMatrix * viewMatrix * vertexPosition;
	//gl_Position =  vec4(vertexPosition,0.0f,1.0f);
	// gl_Position=vec4(vertexPosition.x/32.0f,vertexPosition.y/32.0f,0.0f,1.0f);
	textureCoord=texturePosition;
}
