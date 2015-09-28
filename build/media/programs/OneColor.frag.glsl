#version 150

uniform sampler2D Texture0;
uniform vec4 Color;

in vec4 oUV0;

out vec4 oFragColor;

void main()
{
	oFragColor = Color;
}
