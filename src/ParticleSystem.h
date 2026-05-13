#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

struct Particle
{
    glm::vec3 pos;
    glm::vec3 velocity;
    float lifetime;
    float life_length;
};

class ParticleSystem
{
public:
    explicit ParticleSystem(int capacity);
    ~ParticleSystem();
    void init_gpu_data();
    void spawn(Particle particle);
    void process_particles(float dt);
    void submit_to_gpu(const glm::mat4& viewMat);

private:
    void kill(int id);

    std::vector<Particle> particles;
    std::vector<glm::vec4> gl_data_temp_buffer;  // only ONE declaration
    int max_size;
    GLuint vao, vbo;
};