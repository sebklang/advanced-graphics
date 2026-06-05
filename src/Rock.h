#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Rock {
    glm::vec3 pos;
    float scale;
    float rotation;
};

std::vector<Rock> generateRocks(int count);