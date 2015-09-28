#version 150

uniform mat4 WorldViewProjection;

in vec4 position;
in vec4 uv0;

out vec4 oUV0;

void main()
{
	gl_Position = WorldViewProjection * position;
	oUV0 = uv0;
}
