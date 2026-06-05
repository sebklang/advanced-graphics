#version 420
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 vel;

uniform float deltaTime;

uniform vec3 rockPositions[32];
uniform float rockRadii[32];
uniform int rockCount;

out vec4 outPosition;
out vec4 outVelocity;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(127.1, 311.7))) * 43758.5453);
}


float terrainHeight(float x, float z)
{
    float xfreq = 2.0 * 1.7;
    float zfreq = 2.0 * 2.1;

    return 32.0 * cos(xfreq * x / 256.0) * cos(zfreq * z / 256.0);
}

void collideRock (inout vec3 p, inout vec3 v, vec4 sphere) {
    vec3 c = sphere.xyz;
    float r = sphere.w;

    vec3 q = p - c;
    float dist = length(q);

    if (dist < r && dist > 0.0001) {
        vec3 n = q / dist;

        p = c + n * r;

        float vn = dot(v, n);

        if (vn < 0.0) {
            float restitution = 0.1;
            float friction = 0.65;

            vec3 vN = vn * n;
            vec3 vT = v - vN;

            v = -restitution * vN + friction * vT;
        }
    }
}

void main()
{
	vec3 p = pos.xyz; 
    vec3 v = vel.xyz; 

    float x = pos[0];
	float y = pos[1];
	float z = pos[2];
	float w = pos[3];

    //wind
    vec3 wind = vec3(5.0, 0.0, 5.0); 

    v += wind * deltaTime;

    p += v * deltaTime;

    float elevation = terrainHeight(p.x, p.z);
    p.y = elevation + 1.0; 

    // Terrain bounds (4 chunks * 256 world units each)
    float minBound = 0.0;
    float maxBound = 1024.0;
    // Kill and respawn if outside terrain bounds
    bool outOfBounds = (p.x < minBound || p.x > maxBound || p.z < minBound || p.z > maxBound);


    if (outOfBounds) {
        // Respawn at a random position on the upwind edge (x=0 side)
        float newX = rand(vec2(x + w, z)) * maxBound;
        float newZ = rand(vec2(z + w, x)) * maxBound;
        float newElevation = terrainHeight(newX, newZ);
        outPosition = vec4(newX, newElevation + 1.0, newZ, w);
        outVelocity = vec4(0.0); 
        gl_Position = outPosition;
        return;
    }

	//float dydx  = -xfreq * sin(xfreq * x) * cos(zfreq * z);
	//float dydz  = -zfreq * sin(zfreq * z) * cos(xfreq * x);
    
    for (int i = 0; i < rockCount; ++i) {
        collideRock(p, v, vec4(rockPositions[i], rockRadii[i]));
    }

    outPosition = vec4(p, 1.0);
    outVelocity = vec4(v, 0.0); 
	gl_Position = outPosition;
}