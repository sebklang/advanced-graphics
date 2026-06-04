#include "heightfield.h"
#include "Terrain.h"

float terrainHeight(float x, float z)
{
    /*
    float largeDunes =
        2.0f * sin(0.025f * x + 0.015f * z) +
        1.2f * sin(0.018f * x - 0.030f * z);

    float smallerWaves =
        0.25f * sin(0.20f * x + 0.05f * z);

    float ripples =
        0.05f * sin(1.5f * x + 0.3f * z);

    return largeDunes + smallerWaves + ripples;
    */
	float xfreq = 2 * 1.7;
	float zfreq = 2 * 2.1;
	float y = cos(xfreq * x) * cos(zfreq * z);
    return y;
}

glm::vec3 terrainNormal(float x, float z)
{
    /*
    float eps = 0.5f;

    float hL = terrainHeight(x - eps, z);
    float hR = terrainHeight(x + eps, z);
    float hD = terrainHeight(x, z - eps);
    float hU = terrainHeight(x, z + eps);

    glm::vec3 n = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));
    return n;
    */
    float xfreq = 2 * 1.7;
	float zfreq = 2 * 2.1;
    float dydx  = -xfreq * sin(xfreq * x) * cos(zfreq * z);
	float dydz  = -zfreq * sin(zfreq * z) * cos(xfreq * x);
    auto normal = glm::normalize(glm::vec3(-dydx, 1.0, -dydz));
    return normal;
}
