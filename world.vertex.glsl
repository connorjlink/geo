#version 330 core

layout (location = 0) in vec4 pos_in;
layout (location = 1) in vec3 col_in;
//layout (location = 2) in vec3 norm_in;

uniform mat4 world_imvp;

out vec3 col;
out vec3 normal;

void main()
{
	gl_Position = pos_in;
	col = col_in;
	vec4 p = pos_in * world_imvp;
	normal = p.xyz;
}