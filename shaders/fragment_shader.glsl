
#version 330

uniform sampler2D textureSampler;

in vec2 textureCoord;

out vec4 outColor;


void main()
{
	outColor = texture(textureSampler,textureCoord);
	// outColor = vec4(textureCoord,0.0f,1.0f);
}