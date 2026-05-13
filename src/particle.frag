#version 420

in float lifetime;
out vec4 fragColor;

void main()
{
    vec4 sandColor = vec4(0.76, 0.60, 0.42, 1.0);
    fragColor = sandColor * vec4(1.0, 1.0, 1.0, 1.0 - lifetime);
}