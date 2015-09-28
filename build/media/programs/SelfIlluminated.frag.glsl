#version 150

uniform sampler2D Texture0;

in vec4 oUV0;

out vec4 oFragColor;

void main()
{
	oFragColor = texture2D(Texture0, oUV0.xy);
}
