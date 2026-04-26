

#ifdef _WIN32
extern "C" _declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
#endif

#include <GL/glew.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

#include <Model.h>
#include "hdr.h"
#include "fbo.h"




using std::min;
using std::max;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
float currentTime = 0.0f;
float previousTime = 0.0f;
float deltaTime = 0.0f;
bool showUI = false;
int windowWidth, windowHeight;

// Mouse input
ivec2 g_prevMouseCoords = { -1, -1 };
bool g_isMouseDragging = false;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint shaderProgram;       // Shader for rendering the final image
GLuint simpleShaderProgram; // Shader used to draw the shadow map
GLuint backgroundProgram;
GLuint terrainGenProgram;
GLuint chunkProgram;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
float environment_multiplier = 1.5f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
vec3 lightPosition;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);

float point_light_intensity_multiplier = 10000.0f;





///////////////////////////////////////////////////////////////////////////////
// Camera parameters.
///////////////////////////////////////////////////////////////////////////////
vec3 cameraPosition(-70.0f, 50.0f, 70.0f);
vec3 cameraDirection = normalize(vec3(0.0f) - cameraPosition);
float cameraSpeed = 50.f;

vec3 worldUp(0.0f, 1.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
labhelper::Model* fighterModel = nullptr;
labhelper::Model* landingpadModel = nullptr;

mat4 roomModelMatrix;
mat4 landingPadModelMatrix;
mat4 fighterModelMatrix;
mat4 chunkModelMatrix;

float shipSpeed = 50;

constexpr int chunkWQuads = 2;
constexpr int chunkHQuads = 2;
constexpr int chunkNumVertices = 6 * chunkWQuads * chunkHQuads;
constexpr int chunkGridBufSz = chunkNumVertices * 2 * sizeof(float);
constexpr int chunkBufSz = chunkNumVertices * 3 * sizeof(float);
GLuint chunkGridVAO = 0;
std::vector<GLuint> chunkVAOs;

GLuint generateChunk(float x0, float z0) {
	glUseProgram(terrainGenProgram);
	GLuint chunkVBO[3]; // positions, normals, texCoords (order agrees with glTransformFeedbackVaryings)
	glGenBuffers(3, chunkVBO);

	for (int i = 0; i < 2; i++) { // positions and normals
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, chunkVBO[i]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, chunkBufSz, NULL, GL_STATIC_COPY);
	}
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, chunkVBO[2]); // texCoords
	glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, chunkGridBufSz, NULL, GL_STATIC_COPY);

	glBindVertexArray(chunkGridVAO);
	glEnable(GL_RASTERIZER_DISCARD);
	glBeginTransformFeedback(GL_TRIANGLES);
	labhelper::setUniformSlow(terrainGenProgram, "x0", x0);
	labhelper::setUniformSlow(terrainGenProgram, "z0", z0);
	glDrawArrays(GL_TRIANGLES, 0, chunkNumVertices);
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);
	for (int i = 0; i < 3; i++)
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, 0);

	GLuint chunkVAO;
	glGenVertexArrays(1, &chunkVAO);
	glBindVertexArray(chunkVAO);
	glBindBuffer(GL_ARRAY_BUFFER, chunkVBO[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, chunkVBO[1]);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, chunkVBO[2]);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(2);
	// chunkVAO should now have the finished chunkVBO as its array buffers
	glUseProgram(0);
	return chunkVAO;
}

void loadShaders(bool is_reload)
{
	GLuint shader = labhelper::loadShaderProgram("../src/simple.vert", "../src/simple.frag",
		is_reload);
	if (shader != 0)
	{
		simpleShaderProgram = shader;
	}

	shader = labhelper::loadShaderProgram("../src/fullscreenQuad.vert", "../src/background.frag",
		is_reload);
	if (shader != 0)
	{
		backgroundProgram = shader;
	}

	shader = labhelper::loadShaderProgram("../src/shading.vert", "../src/shading.frag", is_reload);
	if (shader != 0)
	{
		shaderProgram = shader;
	}

	shader = labhelper::loadShaderProgram("../src/chunk.vert", "../src/chunk.frag", is_reload);
	if (shader != 0)
	{
		chunkProgram = shader;
	}
	else abort();

	GLuint terrainGenShader = glCreateShader(GL_VERTEX_SHADER);
	std::ifstream vs_file("../src/generateTerrain.vert");
	std::string vs_src(std::istreambuf_iterator<char>(vs_file), std::istreambuf_iterator<char>{});
	const char* vs = vs_src.c_str();
	glShaderSource(terrainGenShader, 1, &vs, nullptr);
	glCompileShader(terrainGenShader);
	terrainGenProgram = glCreateProgram();
	glAttachShader(terrainGenProgram, terrainGenShader);
	glDeleteShader(terrainGenShader);
	if (!is_reload)
		CHECK_GL_ERROR();
	const char* varyings[] = { "position", "normal", "texCoord" };
	glTransformFeedbackVaryings(terrainGenProgram, 3, varyings, GL_SEPARATE_ATTRIBS);
	labhelper::linkShaderProgram(terrainGenProgram, is_reload);
}



///////////////////////////////////////////////////////////////////////////////
/// This function is called once at the start of the program and never again
///////////////////////////////////////////////////////////////////////////////
void initialize()
{
	ENSURE_INITIALIZE_ONLY_ONCE();

	///////////////////////////////////////////////////////////////////////
	//		Load Shaders
	///////////////////////////////////////////////////////////////////////
	loadShaders(false);

	///////////////////////////////////////////////////////////////////////
	// Load models and set up model matrices
	///////////////////////////////////////////////////////////////////////
	fighterModel = labhelper::loadModelFromOBJ("../scenes/space-ship.obj");
	landingpadModel = labhelper::loadModelFromOBJ("../scenes/landingpad.obj");

	roomModelMatrix = mat4(1.0f);
	fighterModelMatrix = translate(15.0f * worldUp);
	landingPadModelMatrix = mat4(1.0f);
	chunkModelMatrix = translate(30.0f * worldUp);

	///////////////////////////////////////////////////////////////////////
	// Load environment map
	///////////////////////////////////////////////////////////////////////
	const int roughnesses = 8;
	std::vector<std::string> filenames;
	for (int i = 0; i < roughnesses; i++)
		filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");

	environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
	irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");
	reflectionMap = labhelper::loadHdrMipmapTexture(filenames);

	// Generate and upload base grid for terrain chunk
	constexpr GLfloat quadW = 1.0 / chunkWQuads;
	constexpr GLfloat quadH = 1.0 / chunkHQuads;
	vec2 triangles[6 * chunkWQuads * chunkHQuads]{};

	for (int i = 0; i < chunkWQuads; i++) {
		for (int j = 0; j < chunkHQuads; j++) {
			int idx = 6 * (chunkHQuads * i + j);
			triangles[idx + 0] = { quadW * (i + 0), quadH * (j + 0) };
			triangles[idx + 1] = { quadW * (i + 0), quadH * (j + 1) };
			triangles[idx + 2] = { quadW * (i + 1), quadH * (j + 1) };
			triangles[idx + 3] = { quadW * (i + 1), quadH * (j + 1) };
			triangles[idx + 4] = { quadW * (i + 1), quadH * (j + 0) };
			triangles[idx + 5] = { quadW * (i + 0), quadH * (j + 0) };
		}
	}

	glGenVertexArrays(1, &chunkGridVAO);
	glBindVertexArray(chunkGridVAO);
	GLuint chunkGridVBO;
	glGenBuffers(1, &chunkGridVBO);
	glBindBuffer(GL_ARRAY_BUFFER, chunkGridVBO);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(triangles), triangles, 0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);
	
	// Generate chunks
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 32; j++) {
			float x0 = i;
			float z0 = j;
			chunkVAOs.push_back(generateChunk(x0, z0));
		}
	}

	glEnable(GL_DEPTH_TEST); // enable Z-buffering
	glEnable(GL_CULL_FACE);  // enables backface culling
}

void debugDrawLight(const glm::mat4& viewMatrix,
	const glm::mat4& projectionMatrix,
	const glm::vec3& worldSpaceLightPos)
{
	mat4 modelMatrix = glm::translate(worldSpaceLightPos);
	glUseProgram(simpleShaderProgram);
	labhelper::setUniformSlow(simpleShaderProgram, "modelViewProjectionMatrix",
		projectionMatrix * viewMatrix * modelMatrix);
	labhelper::setUniformSlow(simpleShaderProgram, "material_color", vec3(1, 1, 1));
	labhelper::debugDrawSphere();
}


void drawBackground(const mat4& viewMatrix, const mat4& projectionMatrix)
{
	glUseProgram(backgroundProgram);
	labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
	labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projectionMatrix * viewMatrix));
	labhelper::setUniformSlow(backgroundProgram, "camera_pos", cameraPosition);
	labhelper::drawFullScreenQuad();
}


///////////////////////////////////////////////////////////////////////////////
/// This function is used to draw the main objects on the scene
///////////////////////////////////////////////////////////////////////////////
void drawScene(GLuint currentShaderProgram,
	const mat4& viewMatrix,
	const mat4& projectionMatrix,
	const mat4& lightViewMatrix,
	const mat4& lightProjectionMatrix)
{
	glUseProgram(currentShaderProgram);
	// Light source
	vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);
	labhelper::setUniformSlow(currentShaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(currentShaderProgram, "point_light_intensity_multiplier",
		point_light_intensity_multiplier);
	labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
	labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightDir",
		normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));


	// Environment
	labhelper::setUniformSlow(currentShaderProgram, "environment_multiplier", environment_multiplier);

	// camera
	labhelper::setUniformSlow(currentShaderProgram, "viewInverse", inverse(viewMatrix));

	// landing pad
	labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix",
		projectionMatrix * viewMatrix * landingPadModelMatrix);
	labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * landingPadModelMatrix);
	labhelper::setUniformSlow(currentShaderProgram, "normalMatrix",
		inverse(transpose(viewMatrix * landingPadModelMatrix)));

	labhelper::render(landingpadModel);

	// Fighter
	labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix",
		projectionMatrix * viewMatrix * fighterModelMatrix);
	labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * fighterModelMatrix);
	labhelper::setUniformSlow(currentShaderProgram, "normalMatrix",
		inverse(transpose(viewMatrix * fighterModelMatrix)));

	labhelper::render(fighterModel);

	// Display chunks
	glUseProgram(chunkProgram);
	labhelper::setUniformSlow(chunkProgram, "modelViewProjectionMatrix",
		projectionMatrix * viewMatrix * chunkModelMatrix);
	labhelper::setUniformSlow(chunkProgram, "modelViewMatrix", viewMatrix * chunkModelMatrix);
	labhelper::setUniformSlow(chunkProgram, "normalMatrix",
		inverse(transpose(viewMatrix * chunkModelMatrix)));

	for (GLuint chunk : chunkVAOs) {
		glBindVertexArray(chunk);
		glDrawArrays(GL_TRIANGLES, 0, chunkNumVertices); // todo: save count somewhere
		glBindVertexArray(0);
	}
	glUseProgram(currentShaderProgram);
}


///////////////////////////////////////////////////////////////////////////////
/// This function will be called once per frame, so the code to set up
/// the scene for rendering should go here
///////////////////////////////////////////////////////////////////////////////
void display(void)
{
	///////////////////////////////////////////////////////////////////////////
	// Check if window size has changed and resize buffers as needed
	///////////////////////////////////////////////////////////////////////////
	{
		int w, h;
		SDL_GetWindowSize(g_window, &w, &h);
		if (w != windowWidth || h != windowHeight)
		{
			windowWidth = w;
			windowHeight = h;
		}
	}


	///////////////////////////////////////////////////////////////////////////
	// setup matrices
	///////////////////////////////////////////////////////////////////////////
	mat4 projMatrix = perspective(radians(45.0f), float(windowWidth) / float(windowHeight), 5.0f, 2000.0f);
	mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);

	vec4 lightStartPosition = vec4(40.0f, 40.0f, 0.0f, 1.0f);
	lightPosition = vec3(rotate(currentTime, worldUp) * lightStartPosition);
	mat4 lightViewMatrix = lookAt(lightPosition, vec3(0.0f), worldUp);
	mat4 lightProjMatrix = perspective(radians(45.0f), 1.0f, 25.0f, 100.0f);

	///////////////////////////////////////////////////////////////////////////
	// Bind the environment map(s) to unused texture units
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);
	glActiveTexture(GL_TEXTURE0);



	///////////////////////////////////////////////////////////////////////////
	// Draw from camera
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawBackground(viewMatrix, projMatrix);
	drawScene(shaderProgram, viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix);
	debugDrawLight(viewMatrix, projMatrix, vec3(lightPosition));



}


///////////////////////////////////////////////////////////////////////////////
/// This function is used to update the scene according to user input
///////////////////////////////////////////////////////////////////////////////
bool handleEvents(void)
{
	// Allow ImGui to capture events.
	ImGuiIO& io = ImGui::GetIO();

	// check events (keyboard among other)
	SDL_Event event;
	bool quitEvent = false;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSdlGL3_ProcessEvent(&event);

		if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
		{
			quitEvent = true;
		}
		else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
		{
			showUI = !showUI;
		}
		else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_PRINTSCREEN)
		{
			labhelper::saveScreenshot();
		}
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT
			&& (!showUI || !io.WantCaptureMouse))
		{
			g_isMouseDragging = true;
			int x;
			int y;
			SDL_GetMouseState(&x, &y);
			g_prevMouseCoords.x = x;
			g_prevMouseCoords.y = y;
		}

		if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			g_isMouseDragging = false;
		}

		if (event.type == SDL_MOUSEMOTION && g_isMouseDragging && !io.WantCaptureMouse)
		{
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
			int delta_x = event.motion.x - g_prevMouseCoords.x;
			int delta_y = event.motion.y - g_prevMouseCoords.y;
			float rotationSpeed = 0.4f;
			mat4 yaw = rotate(rotationSpeed * deltaTime * -delta_x, worldUp);
			mat4 pitch = rotate(rotationSpeed * deltaTime * -delta_y,
				normalize(cross(cameraDirection, worldUp)));
			cameraDirection = vec3(pitch * yaw * vec4(cameraDirection, 0.0f));
			g_prevMouseCoords.x = event.motion.x;
			g_prevMouseCoords.y = event.motion.y;
		}
	}

	// check keyboard state (which keys are still pressed)
	const uint8_t* state = SDL_GetKeyboardState(nullptr);

	static bool was_shift_pressed = state[SDL_SCANCODE_LSHIFT];
	if (was_shift_pressed && !state[SDL_SCANCODE_LSHIFT])
	{
		cameraSpeed *= 5;
	}
	if (!was_shift_pressed && state[SDL_SCANCODE_LSHIFT])
	{
		cameraSpeed /= 5;
	}
	was_shift_pressed = state[SDL_SCANCODE_LSHIFT];


	vec3 cameraRight = normalize(cross(cameraDirection, worldUp));

	if (state[SDL_SCANCODE_W])
	{
		cameraPosition += cameraSpeed * deltaTime * cameraDirection;
	}
	if (state[SDL_SCANCODE_S])
	{
		cameraPosition -= cameraSpeed * deltaTime * cameraDirection;
	}
	if (state[SDL_SCANCODE_A])
	{
		cameraPosition -= cameraSpeed * deltaTime * cameraRight;
	}
	if (state[SDL_SCANCODE_D])
	{
		cameraPosition += cameraSpeed * deltaTime * cameraRight;
	}
	if (state[SDL_SCANCODE_Q])
	{
		cameraPosition -= cameraSpeed * deltaTime * cross(cameraRight, cameraDirection);
	}
	if (state[SDL_SCANCODE_E])
	{
		cameraPosition += cameraSpeed * deltaTime * cross(cameraRight, cameraDirection);
	}
	return quitEvent;
}


///////////////////////////////////////////////////////////////////////////////
/// This function is to hold the general GUI logic
///////////////////////////////////////////////////////////////////////////////
void gui()
{
	// ----------------- Set variables --------------------------
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		ImGui::GetIO().Framerate);
	// ----------------------------------------------------------
}

int main(int argc, char* argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Project");

	initialize();

	bool stopRendering = false;
	auto startTime = std::chrono::system_clock::now();

	while (!stopRendering)
	{
		//update currentTime
		std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
		previousTime = currentTime;
		currentTime = timeSinceStart.count();
		deltaTime = currentTime - previousTime;

		// Inform imgui of new frame
		ImGui_ImplSdlGL3_NewFrame(g_window);

		// check events (keyboard among other)
		stopRendering = handleEvents();

		// render to window
		display();

		// Render overlay GUI.
		if (showUI)
		{
			gui();
		}

		// Render the GUI.
		ImGui::Render();

		// Swap front and back buffer. This frame will now been displayed.
		SDL_GL_SwapWindow(g_window);
	}
	// Free Models
	labhelper::freeModel(fighterModel);
	labhelper::freeModel(landingpadModel);

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
