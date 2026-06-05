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

private:
    std::vector<Particle> particles;
    int max_size;
    GLuint vaos[2], vbos[2];
    int activeIndex;
	GLuint particleFeedbackProgram; 
};
