#version 420

layout(location = 0) in vec2 base;

void main()
{
	float y = cos(base[0]) * cos(base[1]);
	gl_Position = vec4(base[0], y, base[1], 1.0);
}
