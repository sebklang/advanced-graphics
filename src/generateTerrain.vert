#version 420
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in vec2 v;

uniform mat4 modelViewProjectionMatrix;

out vec3 position;
out vec3 normal;
out vec2 texCoord;

void main()
{
	float xfreq = 5;
	float zfreq = 3;
	float y     =  cos(xfreq * v[0]) * cos(zfreq * v[1]);
	float dydx  = -xfreq * sin(xfreq * v[0]) * cos(zfreq * v[1]);
	float dydz  = -zfreq * sin(zfreq * v[1]) * cos(xfreq * v[0]);

	position  = vec3(v[0], y, v[1]);
	normal    = normalize(vec3(-dydx, 1.0, -dydz));
	texCoord  = v; // todo

	gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);
}
