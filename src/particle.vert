#version 420
layout(location = 0) in vec4 particle;
uniform mat4 P;
uniform float screen_y; //screen height, used to scale the point size
out float life;


void main()
{
    life = particle.w;
    gl_Position = P * vec4(particle.xyz, 1.0);
    gl_PointSize = screen_y / 400.0;
    //gl_PointSize = 1.0; 
}