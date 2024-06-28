#version 330 core

in vec3 col;
in vec3 normal;

//const vec3 col = vec3(1.0, 0.0, 0.0);
//const vec3 normal = vec3(0.0, 1.0, 0.0);

out vec4 frag_color;

float smin(float a, float b, float k)
{
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

void main()
{
	vec3 sun = vec3(1.0, 1.0, 1.0);
	
	float intensity = (dot(normal, sun) + 1.0) / 2.0;
	intensity = smin(0.1, intensity, -0.2);
	
	frag_color = vec4(col * intensity, 1.0);
}