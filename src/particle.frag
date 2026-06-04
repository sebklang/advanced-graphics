#version 420

in float life;        // was: lifetime, now matches particle.vert's "out float life"
out vec4 fragColor;

void main()
{
    vec4 sandColor = vec4(0.76, 0.60, 0.42, 1.0);
    fragColor = sandColor; // keep particle visible
}