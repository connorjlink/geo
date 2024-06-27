#version 330 core

layout (location = 0) in vec4 pos_in;
layout (location = 1) in vec3 col_in;
//layout (location = 2) in vec3 norm_in;

out vec3 col;
//out vec3 normal;

void main()
{
	gl_Position = pos_in;
	col = col_in;
	//normal = norm_in;
}