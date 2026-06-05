#version 420

#define MAX_ROCK_FACES 64
#define MAX_ROCKS 16

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 vel;

uniform float deltaTime;

uniform int numRockFaces;
uniform vec4 rockPlanes[MAX_ROCK_FACES];
uniform int rockCount;
uniform int rockPlaneOffsets[MAX_ROCKS];
uniform int rockPlaneCounts[MAX_ROCKS];
// xyz = normal, w = d

out vec4 outPosition;
out vec4 outVelocity;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(127.1, 311.7))) * 43758.5453);
}

float signedDistanceToPlane(vec3 p, vec4 plane) {
    return dot(p, plane.xyz) - plane.w;
}

void collideConvexRock(inout vec3 p, inout vec3 v, int rockIndex) {
    int offset = rockPlaneOffsets[rockIndex];
    int count = rockPlaneCounts[rockIndex];

    float maxDistance = -1e20;
    vec3 closestNormal = vec3(0.0, 1.0, 0.0);

    for (int face = 0; face < count; ++face) {
        int planeIndex = offset + face;
        if (planeIndex >= numRockFaces) {
            break;
        }

        vec4 plane = rockPlanes[planeIndex];
        float distance = signedDistanceToPlane(p, plane);
        if (distance > maxDistance) {
            maxDistance = distance;
            closestNormal = normalize(plane.xyz);
        }
    }

    float influenceDistance = 8.0;
    if (maxDistance > influenceDistance) {
        return;
    }

    if (maxDistance < 0.0) {
        p -= closestNormal * maxDistance;

        float normalVelocity = dot(v, closestNormal);
        if (normalVelocity < 0.0) {
            vec3 vNormal = normalVelocity * closestNormal;
            vec3 vTangent = v - vNormal;
            v = vTangent * 0.65 - vNormal * 0.1;
        }
    } else {
        float influence = 1.0 - smoothstep(0.0, influenceDistance, maxDistance);
        v += closestNormal * influence * 10.0 * deltaTime;
    }
}

float terrainHeight(float x, float z)
{
    float xfreq = 2.0 * 1.7;
    float zfreq = 2.0 * 2.1;

    return 32.0 * cos(xfreq * x / 256.0) * cos(zfreq * z / 256.0);
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

    for (int i = 0; i < rockCount; i++) {
        collideConvexRock(p, v, i);
    }

    outPosition = vec4(p, 1.0);
    outVelocity = vec4(v, 0.0); 
	gl_Position = outPosition;
}
