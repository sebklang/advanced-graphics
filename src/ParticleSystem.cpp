#include "ParticleSystem.h"
#include <algorithm>  // for std::sort

ParticleSystem::ParticleSystem(int capacity) : max_size(capacity)
{
    gl_data_temp_buffer.resize(max_size);
}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::init_gpu_data()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, max_size * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
}

void ParticleSystem::spawn(Particle p)
{
    if (particles.size() < max_size)
        particles.push_back(p);
}

void ParticleSystem::kill(int id)
{
    particles[id] = particles.back();
    particles.pop_back();
}

void ParticleSystem::process_particles(float dt)
{
    const glm::vec3 gravity = glm::vec3(0.f, -9.8f, 0.f);
    for (auto& p : particles) {
        p.velocity += gravity * dt;
        p.pos += p.velocity * dt;
        p.lifetime += dt;
    }
    for (int i = 0; i < particles.size(); ++i) {
        if (particles[i].lifetime > particles[i].life_length)
            kill(i--);
    }
}

void ParticleSystem::submit_to_gpu(const glm::mat4& viewMat)
{
    unsigned int num_active = particles.size();
    for (int i = 0; i < num_active; ++i) {
        glm::vec3 viewPos = glm::vec3(viewMat * glm::vec4(particles[i].pos, 1.0f));
        float normalizedLife = particles[i].lifetime / particles[i].life_length;
        gl_data_temp_buffer[i] = glm::vec4(viewPos, normalizedLife);
    }
    std::sort(gl_data_temp_buffer.begin(),
        std::next(gl_data_temp_buffer.begin(), num_active),
        [](const glm::vec4& a, const glm::vec4& b) { return a.z < b.z; });

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_active * sizeof(glm::vec4), gl_data_temp_buffer.data());

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, num_active);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}