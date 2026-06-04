#version 420
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in vec4 pos;

uniform float deltaTime;

out vec4 position;

void main()
{
	vec4 velocity = vec4(5.0, 0.0, 5.0, 0.0);

	float x = pos[0];
	float y = pos[1];
	float z = pos[2];
	float w = pos[3];

	float xfreq = 2 * 1.7;
	float zfreq = 2 * 2.1;

	float elevation = 32.0 * cos(xfreq * x / 256.0) * cos(zfreq * z / 256.0);
	//float h = y - elevation;

	//float dydx  = -xfreq * sin(xfreq * x) * cos(zfreq * z);
	//float dydz  = -zfreq * sin(zfreq * z) * cos(xfreq * x);

	vec4 deltaPos = velocity * deltaTime;

	position = vec4(x + deltaPos.x, elevation + 1.0, z + deltaPos.z, w);
	gl_Position = position;
}