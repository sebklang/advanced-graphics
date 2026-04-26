#version 420

precision highp float;

uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;

layout(location = 0) out vec4 fragmentColor;

