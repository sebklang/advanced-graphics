#include <fstream>
#include <iostream>
#include <algorithm>  // for std::sort
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include "ParticleSystem.h"
#include "Terrain.h"
#include "labhelper.h"
#include <vector>
#include <cmath>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

ParticleSystem::ParticleSystem(int capacity, glm::mat4 modelMatrix)
    : max_size(capacity)
    , obstacleModelMatrix(modelMatrix)
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

static bool intersectThreePlanes(
	const Face& a,
	const Face& b,
	const Face& c,
	glm::vec3& out
) {
	const glm::vec3 n1 = a.normal;
	const glm::vec3 n2 = b.normal;
	const glm::vec3 n3 = c.normal;

	const float denom = glm::dot(n1, glm::cross(n2, n3));

	if(std::abs(denom) < 1e-5f)
		return false;

	out =
		(a.d * glm::cross(n2, n3) +
		 b.d * glm::cross(n3, n1) +
		 c.d * glm::cross(n1, n2)) / denom;

	return true;
}

ObstacleGpu uploadObstacleToGpu(const Obstacle& obstacle)
{
	std::vector<ObstacleVertex> vertices;

	const float eps = 1e-4f;

	for(size_t faceIndex = 0; faceIndex < obstacle.faces.size(); ++faceIndex)
	{
		const Face& face = obstacle.faces[faceIndex];
		glm::vec3 n = glm::normalize(face.normal);

		std::vector<glm::vec3> polygon;

		// Hitta alla vertices på detta face genom att skära
		// detta plan med två andra plan.
		for(size_t j = 0; j < obstacle.faces.size(); ++j)
		{
			if(j == faceIndex)
				continue;

			for(size_t k = j + 1; k < obstacle.faces.size(); ++k)
			{
				if(k == faceIndex)
					continue;

				glm::vec3 p;
				if(!intersectThreePlanes(face, obstacle.faces[j], obstacle.faces[k], p))
					continue;

				// Safety bound så vi inte får enorma numeriska skräppunkter.
				if(glm::length2(p - obstacle.center) > obstacle.radius * obstacle.radius * 4.0f)
					continue;

				// Antagande: normalerna pekar utåt.
				// Då ligger insidan av hindret där dot(p, normal) <= d.
				bool insideAllPlanes = true;
				for(const Face& f : obstacle.faces)
				{
					if(glm::dot(p, f.normal) > f.d + eps)
					{
						insideAllPlanes = false;
						break;
					}
				}

				if(!insideAllPlanes)
					continue;

				// Undvik dubletter.
				bool duplicate = false;
				for(const glm::vec3& existing : polygon)
				{
					if(glm::length2(existing - p) < 1e-6f)
					{
						duplicate = true;
						break;
					}
				}

				if(!duplicate)
					polygon.push_back(p);
			}
		}

		if(polygon.size() < 3)
			continue;

		// Sortera hörnen runt face-normalen.
		glm::vec3 polygonCenter(0.0f);
		for(const glm::vec3& p : polygon)
			polygonCenter += p;
		polygonCenter /= static_cast<float>(polygon.size());

		glm::vec3 tangent = glm::normalize(polygon[0] - polygonCenter);
		glm::vec3 bitangent = glm::normalize(glm::cross(n, tangent));

		std::sort(
			polygon.begin(),
			polygon.end(),
			[&](const glm::vec3& a, const glm::vec3& b)
			{
				glm::vec3 da = a - polygonCenter;
				glm::vec3 db = b - polygonCenter;

				float angleA = std::atan2(
					glm::dot(da, bitangent),
					glm::dot(da, tangent)
				);

				float angleB = std::atan2(
					glm::dot(db, bitangent),
					glm::dot(db, tangent)
				);

				return angleA < angleB;
			}
		);

		// Triangulera face:et som triangle fan.
		for(size_t i = 1; i + 1 < polygon.size(); ++i)
		{
			ObstacleVertex a;
			a.position = polygon[0];
			a.normal = n;
			a.texCoord = glm::vec2(0.0f, 0.0f);

			ObstacleVertex b;
			b.position = polygon[i];
			b.normal = n;
			b.texCoord = glm::vec2(1.0f, 0.0f);

			ObstacleVertex c;
			c.position = polygon[i + 1];
			c.normal = n;
			c.texCoord = glm::vec2(0.0f, 1.0f);

			vertices.push_back(a);
			vertices.push_back(b);
			vertices.push_back(c);
		}
	}

	ObstacleGpu gpu;
	gpu.vertexCount = static_cast<GLsizei>(vertices.size());

	glGenVertexArrays(1, &gpu.vao);
	glBindVertexArray(gpu.vao);

	glGenBuffers(1, &gpu.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);

	glBufferData(
		GL_ARRAY_BUFFER,
		vertices.size() * sizeof(ObstacleVertex),
		vertices.data(),
		GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(ObstacleVertex),
		reinterpret_cast<void*>(offsetof(ObstacleVertex, position))
	);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(ObstacleVertex),
		reinterpret_cast<void*>(offsetof(ObstacleVertex, normal))
	);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(ObstacleVertex),
		reinterpret_cast<void*>(offsetof(ObstacleVertex, texCoord))
	);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return gpu;
}

void renderObstacle(
	const ObstacleGpu& gpu,
	GLuint shaderProgram,
	const glm::mat4& modelMatrix,
	const glm::mat4& viewMatrix,
	const glm::mat4& projectionMatrix
) {
	glUseProgram(shaderProgram);

	glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;
	glm::mat4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelViewMatrix));

	glUniformMatrix4fv(
		glGetUniformLocation(shaderProgram, "modelViewMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(modelViewMatrix)
	);

	glUniformMatrix4fv(
		glGetUniformLocation(shaderProgram, "modelViewProjectionMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(modelViewProjectionMatrix)
	);

	glUniformMatrix4fv(
		glGetUniformLocation(shaderProgram, "normalMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(normalMatrix)
	);

	glUniform3f(
		glGetUniformLocation(shaderProgram, "material_color"),
		0.34f, 0.31f, 0.27f
	);

	glUniform1f(
		glGetUniformLocation(shaderProgram, "material_shininess"),
		8.0f
	);

	glUniform1f(
		glGetUniformLocation(shaderProgram, "material_metalness"),
		0.0f
	);

	glUniform1f(
		glGetUniformLocation(shaderProgram, "material_fresnel"),
		0.04f
	);

	glUniform3f(
		glGetUniformLocation(shaderProgram, "material_emission"),
		0.0f, 0.0f, 0.0f
	);

	glUniform1i(
		glGetUniformLocation(shaderProgram, "has_color_texture"),
		0
	);

	glUniform1i(
		glGetUniformLocation(shaderProgram, "has_emission_texture"),
		0
	);

	glBindVertexArray(gpu.vao);
	glDrawArrays(GL_TRIANGLES, 0, gpu.vertexCount);
	glBindVertexArray(0);
}

void ParticleSystem::renderRocks(GLuint shaderProgram,
	const glm::mat4& modelMatrix,
	const glm::mat4& viewMatrix,
	const glm::mat4& projectionMatrix) {
	for (const auto& gpu : obstacleGpus) {
		renderObstacle(gpu, shaderProgram, modelMatrix, viewMatrix, projectionMatrix);
	}
}

Face makeFace(glm::vec3 center, glm::vec3 normal, float distanceFromCenter) {
    normal = glm::normalize(normal);

    glm::vec3 point = center + distanceFromCenter * normal;

    return Face {
        point,
        normal,
        glm::dot(point, normal)
    };
}

static float randRange(std::mt19937& rng, float a, float b) {
    std::uniform_real_distribution<float> dist(a, b);
    return dist(rng);
}

Obstacle generateRock(std::mt19937& rng, glm::vec3 center, float radius, int extraFaces) {
    Obstacle rock;
    rock.center = center;
    rock.radius = radius;

    // Grundform: ungefär en låda
    std::vector<glm::vec3> normals = {
        { 1,  0,  0},
        {-1,  0,  0},
        { 0,  1,  0},
        { 0, -1,  0},
        { 0,  0,  1},
        { 0,  0, -1}
    };

    // Extra sneda ansikten för mer stenlik form
    for (int i = 0; i < extraFaces; ++i) {
        glm::vec3 n = glm::vec3(
            randRange(rng, -1.0f, 1.0f),
            randRange(rng, -0.4f, 1.0f),
            randRange(rng, -1.0f, 1.0f)
        );

        if (glm::length(n) > 0.001f) {
            normals.push_back(glm::normalize(n));
        }
    }

    for (glm::vec3 n : normals) {
        float dist = radius * randRange(rng, 0.65f, 1.15f);
        rock.faces.push_back(makeFace(center, n, dist));
    }

    return rock;
}

std::vector<Obstacle> ParticleSystem::generateSceneRocks() {
    std::mt19937 rng(1337);
    std::vector<Obstacle> rocks;
    // Stora stenar i scenkoordinater 0..4
    rocks.push_back(generateRock(rng, glm::vec3(0.8f, 0.25f, 1.0f), 0.35f, 2));
    rocks.push_back(generateRock(rng, glm::vec3(2.4f, 0.30f, 1.7f), 0.50f, 4));
    rocks.push_back(generateRock(rng, glm::vec3(3.3f, 0.20f, 3.1f), 0.40f, 3));
    for (const auto& rock : rocks) {
        ObstacleGpu obstacleGpu = uploadObstacleToGpu(rock);
		obstacleGpus.push_back(obstacleGpu);
    }
    return rocks;
}

void ParticleSystem::set_obstacles2() {
    constexpr int maxRockFaces = 64;
    constexpr int maxRocks = 16;

    glUseProgram(particleFeedbackProgram);
    auto rocks = generateSceneRocks();

    std::vector<glm::vec4> planes;
    std::vector<GLint> rockPlaneOffsets;
    std::vector<GLint> rockPlaneCounts;

    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(obstacleModelMatrix)));

    for (const Obstacle& rock : rocks) {
        if ((int)rockPlaneOffsets.size() >= maxRocks) {
            break;
        }

        const GLint start = (GLint)planes.size();
        for (const Face& face : rock.faces) {
            if ((int)planes.size() >= maxRockFaces) {
                break;
            }

            const glm::vec3 worldPoint = glm::vec3(obstacleModelMatrix * glm::vec4(face.point, 1.0f));
            const glm::vec3 worldNormal = glm::normalize(normalMatrix * face.normal);
            const float worldD = glm::dot(worldPoint, worldNormal);

            planes.push_back(glm::vec4(worldNormal, worldD));
        }

        const GLint count = (GLint)planes.size() - start;
        if (count > 0) {
            rockPlaneOffsets.push_back(start);
            rockPlaneCounts.push_back(count);
        }
    }

    glUniform1i(
        glGetUniformLocation(particleFeedbackProgram, "numRockFaces"),
        (int)planes.size()
    );

    glUniform1i(
        glGetUniformLocation(particleFeedbackProgram, "rockCount"),
        (int)rockPlaneOffsets.size()
    );

    if (!planes.empty()) {
        glUniform4fv(
            glGetUniformLocation(particleFeedbackProgram, "rockPlanes"),
            (GLsizei)planes.size(),
            glm::value_ptr(planes[0])
        );
    }

    if (!rockPlaneOffsets.empty()) {
        glUniform1iv(
            glGetUniformLocation(particleFeedbackProgram, "rockPlaneOffsets"),
            (GLsizei)rockPlaneOffsets.size(),
            rockPlaneOffsets.data()
        );

        glUniform1iv(
            glGetUniformLocation(particleFeedbackProgram, "rockPlaneCounts"),
            (GLsizei)rockPlaneCounts.size(),
            rockPlaneCounts.data()
        );
    }

    glUseProgram(0);
}
