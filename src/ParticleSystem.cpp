#include <fstream>
#include <iostream>
#include <algorithm>  // for std::sort
#include "ParticleSystem.h"
#include "Terrain.h"
#include "labhelper.h"

ParticleSystem::ParticleSystem(int capacity, glm::mat4 modelMatrix)
    : max_size(capacity)
    , activeVBOindex(0)
{
    for (int i = 0; i < max_size; i++) {
        float xfreq = 2 * 1.7;
	    float zfreq = 2 * 2.1;
        float lx = (rand() / (float)RAND_MAX) * 4.0f;
        float lz = (rand() / (float)RAND_MAX) * 4.0f;
        float ly = cos(xfreq * lx) * cos(zfreq * lz);
        Particle p;
        p.pos       = modelMatrix * glm::vec4(lx, ly, lz, 1.0f)
                        + glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        //p.velocity  = glm::vec3(0.0f);
        //p.lifetime  = 0.0f;
        //p.life_length = 1e9f;
        spawn(p);
    }
    //std::sort(particles.begin(),
    //    std::next(particles.begin(), max_size),
    //        [](Particle& a, const Particle& b) { return a.z < b.z; });

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
    const char* varyings[] = { "position" };
	glTransformFeedbackVaryings(particleFeedbackProgram, 1, varyings, GL_SEPARATE_ATTRIBS);
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

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbos);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, max_size * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, max_size * sizeof(glm::vec4), particles.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbos[1]);
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
    /*
    const glm::vec3 wind = glm::vec3(2.0f, 0.0f, 0.5f);
    for (auto& p : particles) {
        p.velocity += wind * dt;
		p.velocity *= 0.98f;  // drag to prevent runaway acceleration
		p.pos += p.velocity * dt;
        p.lifetime += dt;

		float groundY = terrainHeight(p.pos.x, p.pos.z);
		if (p.pos.y < groundY) {
			p.pos.y = groundY;
			p.velocity.y = 0.0f;  // settle on surface
		}
    }
    for (int i = 0; i < particles.size(); ++i) {
        if (particles[i].lifetime > particles[i].life_length)
            kill(i--);
    }
    */
    glUseProgram(particleFeedbackProgram); // todo
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[activeVBOindex]);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    //glEnableVertexAttribArray(0);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbos[1 - activeVBOindex]);
    labhelper::setUniformSlow(particleFeedbackProgram, "deltaTime", dt);
	glEnable(GL_RASTERIZER_DISCARD);
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, max_size);
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);
	glUseProgram(0);
	activeVBOindex = 1 - activeVBOindex;
}


void ParticleSystem::draw_particles(const glm::mat4& viewMat, const glm::mat4& projMat, GLuint shaderProgram)
{
    glUseProgram(shaderProgram);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[activeVBOindex]);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

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