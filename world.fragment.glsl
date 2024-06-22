#version 330 core

in vec3 col;
in vec3 normal;

out vec4 frag_color;

float smin(float a, float b, float k)
{
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

void main()
{
	vec3 sun = vec3(0.0f, 1.0f, 0.0f);
	
	float intensity = dot(normal, sun);
	intensity = smin(0.1, intensity, -0.2);
	
	frag_color = vec4(col * intensity, 1.0f);
}