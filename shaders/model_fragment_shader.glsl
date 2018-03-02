
#version 330

uniform sampler2D textureSampler;

in vec2 textureCoord;

out vec4 outColor;

void main()
{
	// outColor = vec4(1.0f,1.0f,1.0f,1.0f);
	outColor = texture(textureSampler,textureCoord);
	if(outColor.a==0.0f)
	{
		discard;
	}
	// outColor = vec4(textureCoord,1.0f,1.0f);
}
