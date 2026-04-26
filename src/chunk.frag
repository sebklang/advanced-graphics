#version 420

precision highp float;

//uniform mat4 viewInverse;
//uniform vec3 viewSpaceLightPosition;

in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;
in vec3 localPosition;

uniform vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3)); // in view space
uniform vec3 lightColor = vec3(1.0);
uniform vec3 ambientColor = 0.5 * vec3(210, 170, 109) / 255;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	vec3 N = normalize(viewSpaceNormal);
	vec3 L = normalize(lightDir);


	float diffuse = max(dot(N, L), 0.0);
	float albedo = (localPosition.y + 1.5) / 3;

	vec3 color = albedo * (ambientColor + diffuse * lightColor);
	//vec3 color = albedo * (ambientColor + 1.0);

	fragmentColor = vec4(color, 1.0);
	return;
}
