#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

struct Particle
{
    glm::vec4 pos;
    //glm::vec3 velocity;
    //float lifetime;
    //float life_length;
};

class ParticleSystem
{
public:
    explicit ParticleSystem(int capacity, glm::mat4 modelMatrix);
    ~ParticleSystem();
    void init_gpu_data();
    void spawn(Particle particle);
    void process_particles(float dt);
    void draw_particles(const glm::mat4& viewMat, const glm::mat4& projMat, GLuint shaderProgram);
	int particle_count() const { return (int)particles.size(); }

private:
    void kill(int id);

    std::vector<Particle> particles;
    int max_size;
    GLuint vao, vbos[2];
    int activeVBOindex;
	GLuint particleFeedbackProgram; 
};