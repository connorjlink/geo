#version 330 core

uniform mat4 sky_imvp;

layout (location = 2) in vec4 position;

out vec3 pos;

void main(void)
{
	gl_Position = position;
	pos = vec4(sky_imvp * position).xyz;
}