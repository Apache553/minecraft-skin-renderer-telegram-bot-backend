
#version 330

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec2 texturePosition;

uniform mat4 viewMatrix;
uniform mat4 projectMatrix;

out vec2 textureCoord;

void main() {
    gl_Position = projectMatrix * viewMatrix * vertexPosition;
    textureCoord = texturePosition;
}
