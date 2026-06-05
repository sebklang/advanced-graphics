#include "Rock.h"
#include "Terrain.h"
#include <cstdlib>

std::vector<Rock> generateRocks(int count) {
    std::vector<Rock> rocks;
    for (int i = 0; i < count; i++) {
        Rock r;
        // Random position within the chunk grid (4x4 chunks, each [0,1])
        float lx = (rand() / (float)RAND_MAX) * 4.0f;
        float lz = (rand() / (float)RAND_MAX) * 4.0f;

        // Convert to world space (same scaling as chunkModelMatrix)
        float wx = lx * 256.0f;
        float wz = lz * 256.0f;
        float wy = terrainHeight(lx, lz) * 32.0f; 

        r.pos      = glm::vec3(wx, wy, wz);
        r.scale    = 2.0f + (rand() / (float)RAND_MAX) * 4.0f;
        r.rotation = (rand() / (float)RAND_MAX) * 6.28318f;
        rocks.push_back(r);
    }
    return rocks;
}