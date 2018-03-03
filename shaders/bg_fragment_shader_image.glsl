
#version 400

uniform sampler2D textureSampler;

in vec2 textureCoord;

out vec4 outColor;

void main() { outColor = texture(textureSampler, textureCoord); }
