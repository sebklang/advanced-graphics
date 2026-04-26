#version 420
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in vec2 v;

uniform float x0;
uniform float z0;

out vec3 position;
out vec3 normal;
out vec2 texCoord;

void main()
{
	float x = x0 + v[0];
	float z = z0 + v[1];
	float xfreq = 0.20;
	float zfreq = 0.36;
	float y     =  cos(xfreq * x) * cos(zfreq * z);
	float dydx  = -xfreq * sin(xfreq * x) * cos(zfreq * z);
	float dydz  = -zfreq * sin(zfreq * z) * cos(xfreq * x);

	position  = vec3(x, y, z);
	normal    = normalize(vec3(-dydx, 1.0, -dydz));
	texCoord  = v; // todo

	gl_Position = vec4(position, 1.0);
}
