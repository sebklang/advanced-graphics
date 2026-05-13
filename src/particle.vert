#version 420

layout(location = 0) in vec4 particle;

uniform mat4 projectionMatrix;
uniform float screen_x;
uniform float screen_y;

out float lifetime;

void main()
{
    lifetime = particle.w;
    gl_Position = projectionMatrix * vec4(particle.xyz, 1.0);
    gl_PointSize = screen_y / 100.0 * (1.0 - particle.w);
}