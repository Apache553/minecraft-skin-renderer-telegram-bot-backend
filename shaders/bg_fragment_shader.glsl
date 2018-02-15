
#version 400

uniform sampler2D textureSampler;

in vec2 textureCoord;

out vec4 outColor;

subroutine vec4 GetColorType();

subroutine (GetColorType) vec4 Plain()
{
	return vec4(0.4f,0.8f,1.0f,1.0f);
}

subroutine (GetColorType) vec4 Image()
{
	return texture(textureSampler,textureCoord);
}

subroutine uniform GetColorType GetColor;

void main()
{
	outColor=GetColor();
}