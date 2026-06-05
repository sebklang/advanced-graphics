#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include "ParticleSystem.h"
#include "labhelper.h"
#include <cmath>
#include <cstddef>

ParticleSystem::ParticleSystem(int capacity, glm::mat4 modelMatrix)
    : max_size(capacity)
    , activeIndex(0)
{
    for (int i = 0; i < max_size; i++) {
        float xfreq = 2 * 1.7;
	    float zfreq = 2 * 2.1;
        float lx = (rand() / (float)RAND_MAX) * 4.0f;
        float lz = (rand() / (float)RAND_MAX) * 4.0f;
        float ly = cos(xfreq * lx) * cos(zfreq * lz);
        Particle p;
        p.pos  = modelMatrix * glm::vec4(lx, ly, lz, 1.0f)
               + glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        p.vel  = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        particles.push_back(p);
    }
}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::init_gpu_data()
{
    GLuint particleFeedbackShader = glCreateShader(GL_VERTEX_SHADER);    
	std::ifstream vs_file("../src/particleFeedback.vert");
	std::string vs_src(std::istreambuf_iterator<char>(vs_file), std::istreambuf_iterator<char>{});
	const char* vs = vs_src.c_str();
	glShaderSource(particleFeedbackShader, 1, &vs, nullptr);
	glCompileShader(particleFeedbackShader);

    GLint success = GL_FALSE;
    glGetShaderiv(particleFeedbackShader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(particleFeedbackShader, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(logLength, '\0');
        glGetShaderInfoLog(particleFeedbackShader, logLength, nullptr, &log[0]);

        std::cerr << "Shader compile error:\n" << log << '\n';
    }

	particleFeedbackProgram = glCreateProgram();
	glAttachShader(particleFeedbackProgram, particleFeedbackShader);
    const char* varyings[] = { "outPosition", "outVelocity" };
	glTransformFeedbackVaryings(particleFeedbackProgram, 2, varyings, GL_INTERLEAVED_ATTRIBS);
    glLinkProgram(particleFeedbackProgram);

    glGetProgramiv(particleFeedbackProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(particleFeedbackProgram, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(logLength, '\0');
        
        glGetProgramInfoLog(particleFeedbackProgram, logLength, nullptr, &log[0]);

        std::cerr << "Program link error:\n" << log << '\n';
    }

	glDeleteShader(particleFeedbackShader);

    glGenVertexArrays(2, vaos);
    glGenBuffers(2, vbos);

    auto data = particles.data();
    for (int i = 0; i < 2; i++) { // Always 2. Iterating over ping-pong buffers
        glBindVertexArray(vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
        glBufferData(GL_ARRAY_BUFFER, max_size * sizeof(Particle), data, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(
            0, 4, GL_FLOAT, GL_FALSE,
            sizeof(Particle),
            (void*) offsetof(Particle, pos)
        );
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            1, 4, GL_FLOAT, GL_FALSE,
            sizeof(Particle),
            (void*) offsetof(Particle, vel)
        );
        glEnableVertexAttribArray(1);
        data = nullptr;
    }

    glBindVertexArray(0);
}

void ParticleSystem::process_particles(float dt)
{
    glUseProgram(particleFeedbackProgram); // todo
    glBindVertexArray(vaos[activeIndex]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbos[1 - activeIndex]);
    labhelper::setUniformSlow(particleFeedbackProgram, "deltaTime", dt);
	glEnable(GL_RASTERIZER_DISCARD);
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, max_size);
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);
	glUseProgram(0);
	activeIndex = 1 - activeIndex;
}


void ParticleSystem::draw_particles(const glm::mat4& viewMat, const glm::mat4& projMat, GLuint shaderProgram)
{
    glUseProgram(shaderProgram);
    glBindVertexArray(vaos[activeIndex]);

    // Upload projection matrix and screen size uniforms required by particle.vert
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "P"), 1, GL_FALSE, &(projMat * viewMat)[0][0]);

    // These are needed by particle.vert to compute point size
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glUniform1f(glGetUniformLocation(shaderProgram, "screen_x"), (float)viewport[2]);
    glUniform1f(glGetUniformLocation(shaderProgram, "screen_y"), (float)viewport[3]);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_POINTS, 0, max_size);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);

    glUseProgram(0);
}

void ParticleSystem::set_obstacles(const std::vector<Rock>& rocks) {
    glUseProgram(particleFeedbackProgram);
    for (int i = 0; i < (int)rocks.size() && i < 32; i++) {
        std::string posName = "rockPositions[" + std::to_string(i) + "]";
        std::string radName = "rockRadii[" + std::to_string(i) + "]";
        labhelper::setUniformSlow(particleFeedbackProgram, posName.c_str(), rocks[i].pos);
        labhelper::setUniformSlow(particleFeedbackProgram,radName.c_str(), rocks[i].scale * 1.15f);
    }
    labhelper::setUniformSlow(particleFeedbackProgram, "rockCount", (int)rocks.size());
    glUseProgram(0);
}
