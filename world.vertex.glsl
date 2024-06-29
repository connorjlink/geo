#version 460 core

layout (location = 0) in vec4 pos_in;
layout (location = 1) in vec3 col_in;

out vec3 col;

void main()
{
	gl_Position = pos_in;
	col = col_in;
}