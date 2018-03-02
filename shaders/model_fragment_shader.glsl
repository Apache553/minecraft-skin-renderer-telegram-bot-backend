
#version 330

uniform sampler2D textureSampler;
uniform int disableTransparent;

in vec2 textureCoord;

out vec4 outColor;

void main()
{
	outColor = texture(textureSampler,textureCoord);
	if(disableTransparent==0&&outColor.a!=1.0f)
	{
	    discard;
	}
	else if(disableTransparent!=0&&outColor.a!=1.0f)
	{
	    outColor=vec4(0.0f,0.0f,0.0f,1.0f);
	}
}
