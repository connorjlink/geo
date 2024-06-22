#version 330 core

const vec3 top_color = vec3(0.016, 0.529, 0.886);
const vec3 bottom_color = vec3(0.007, 0.288, 0.692);

//uniform vec3 sun;
const vec3 sun = vec3(0.0, 1.0, 0.0);

in vec3 pos;
out vec4 out_color;

float sigmoid(float x)
{
	return (2.0 / (1.0 + exp(-15.0 * x))) - 1.0;
}

void main(void)
{
	//gl_FragDepth = gl_DepthRange.far;
	float gradient = dot(normalize(pos), sun);
	gradient = sigmoid(gradient);
	out_color = vec4(mix(bottom_color, top_color, gradient), 1.0);
}