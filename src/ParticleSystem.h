#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Rock.h"


struct Particle
{
    glm::vec4 pos;
    glm::vec4 vel;
};

struct Face {
    glm::vec3 point;
    glm::vec3 normal;
    float d; // dot(point, normal)
};

struct Obstacle {
    glm::vec3 center;
    float radius;
    std::vector<Face> faces;
};

struct ObstacleVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct ObstacleGpu {
	GLuint vao = 0;
	GLuint vbo = 0;
	GLsizei vertexCount = 0;
};

class ParticleSystem
{
public:
    explicit ParticleSystem(int capacity, glm::mat4 modelMatrix);
    ~ParticleSystem();
    void init_gpu_data();
    void process_particles(float dt);
    void draw_particles(const glm::mat4& viewMat, const glm::mat4& projMat, GLuint shaderProgram);
	int particle_count() const { return (int)particles.size(); }
	void set_obstacles(const std::vector<Rock>& rocks);
    void set_obstacles2();
    void renderRocks(GLuint shaderProgram,
        const glm::mat4& modelMatrix,
        const glm::mat4& viewMatrix,
        const glm::mat4& projectionMatrix
    );
    std::vector<Obstacle> generateSceneRocks();

private:
    std::vector<Particle> particles;
    std::vector<ObstacleGpu> obstacleGpus;
    glm::mat4 obstacleModelMatrix;
    int max_size;
    GLuint vaos[2], vbos[2];
    int activeIndex;
	GLuint particleFeedbackProgram; 
};
