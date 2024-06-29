#version 460

in vec3 col;
in vec3 normal;

layout (std430, binding = 2) buffer Buffer 
{
	uint normals[];
};


out vec4 frag_color;

float smin(float a, float b, float k)
{
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

vec3[] normal_lookup = vec3[]
(
	vec3( 0.0,  0.0,  1.0), // close
	vec3( 0.0,  1.0,  0.0), // top
	vec3(-1.0,  0.0,  0.0), // left
	vec3( 1.0,  0.0,  0.0), // right
	vec3( 0.0,  0.0, -1.0), // far
	vec3( 0.0, -1.0,  0.0)  // bottom
);

void main()
{
	const vec3 sun = vec3(1.0, 1.0, 1.0);
	const vec3 normal = normal_lookup[normals[gl_PrimitiveID / 2]];

	float intensity = (dot(normal, sun) + 1.0) / 2.0;

	intensity = smin(0.1, intensity, -0.2);
	
	frag_color = vec4(col * intensity, 1.0);
}