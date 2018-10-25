#include "assignment4_modified.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/ShaderProgramManager.hpp"
#include "external/lodepng.h"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>

#include "parametric_shapes_modified.hpp"
#include "core/node.hpp"

#include <stdexcept>
#include <array>
#include <numeric>
#include <stack>

enum class polygon_mode_t : unsigned int
{
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

static std::vector<u8>
getTextureData(std::string const &filename, u32 &width, u32 &height, bool flip)
{
	auto const path = config::resources_path(filename);
	std::vector<unsigned char> image;
	if (lodepng::decode(image, width, height, path, LCT_RGBA) != 0)
	{
		LogWarning("Couldn't load or decode image file %s", path.c_str());
		return image;
	}
	if (!flip)
		return image;

	auto const channels_nb = 4u;
	auto flipBuffer = std::vector<u8>(width * height * channels_nb);
	for (u32 y = 0; y < height; y++)
		memcpy(flipBuffer.data() + (height - 1 - y) * width * channels_nb, &image[y * width * channels_nb], width * channels_nb);

	return flipBuffer;
}

GLuint
my_loadTextureCubeMap(std::string const &posx, std::string const &negx,
					  std::string const &posy, std::string const &negy,
					  std::string const &posz, std::string const &negz,
					  bool generate_mipmap)
{
	GLuint texture = 0u;
	// Create an OpenGL texture object. Similarly to `glGenVertexArrays()`
	// and `glGenBuffers()` that were used in assignment 2,
	// `glGenTextures()` can create `n` texture objects at once. Here we
	// only one texture object that will contain our whole cube map.
	glGenTextures(1, &texture);
	assert(texture != 0u);

	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	// Set the wrapping properties of the texture; you can have a look on
	// http://docs.gl to learn more about them
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set the minification and magnification properties of the textures;
	// you can have a look on http://docs.gl to lear more about them, or
	// attend EDAN35 in the next period ;-)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, generate_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	const std::array<std::pair<std::string, GLenum>, 6u> directions{std::make_pair(posx, GL_TEXTURE_CUBE_MAP_POSITIVE_X), std::make_pair(negx, GL_TEXTURE_CUBE_MAP_NEGATIVE_X), std::make_pair(posy, GL_TEXTURE_CUBE_MAP_POSITIVE_Y), std::make_pair(negy, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y), std::make_pair(posz, GL_TEXTURE_CUBE_MAP_POSITIVE_Z), std::make_pair(negz, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)};

	for (auto const dir : directions)
	{
		// We need to fill in the cube map using the images passed in as
		// argument. The function `getTextureData()` uses lodepng to read in
		// the image files and return a `std::vector<u8>` containing all the
		// texels.
		u32 width, height;
		auto const data = getTextureData("cubemaps/" + dir.first, width, height, false);
		if (data.empty())
		{
			glDeleteTextures(1, &texture);
			return 0u;
		}
		// With all the texels available on the CPU, we now want to push them
		// to the GPU: this is done using `glTexImage2D()` (among others). You
		// might have thought that the target used here would be the same as
		// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
		// to what is done `bonobo::loadTexture2D()`. However, we want to fill
		// in a cube map, which has six different faces, so instead we specify
		// as the target the face we want to fill in. In this case, we will
		// start by filling the face sitting on the negative side of the
		// x-axis by specifying GL_TEXTURE_CUBE_MAP_NEGATIVE_X.
		glTexImage2D(dir.second,
					 /* mipmap level, you'll see that in EDAN35 */ 0,
					 /* how are the components internally stored */ GL_RGBA,
					 /* the width of the cube map's face */ static_cast<GLsizei>(width),
					 /* the height of the cube map's face */ static_cast<GLsizei>(height),
					 /* must always be 0 */ 0,
					 /* the format of the pixel data: which components are available */ GL_RGBA,
					 /* the type of each component */ GL_UNSIGNED_BYTE,
					 /* the pointer to the actual data on the CPU */ reinterpret_cast<GLvoid const *>(data.data()));
		//Repeat with the remaining faces.
	}

	if (generate_mipmap)
		// Generate the mipmap hierarchy; wait for EDAN35 to understand
		// what it does
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	return texture;
}

GLuint myLoadCubemap(std::string const &folderName, bool generate_mipmap)
{
	return my_loadTextureCubeMap(folderName + "/posx.png", folderName + "/negx.png", folderName + "/posy.png", folderName + "/negy.png", folderName + "/posz.png", folderName + "/negz.png", generate_mipmap);
}

edaf80::Assignment4::Assignment4() : mCamera(0.5f * glm::half_pi<float>(),
											 static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
											 0.01f, 1000.0f),
									 inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 4", window_datum, config::msaa_rate);
	if (window == nullptr)
	{
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment4::~Assignment4()
{
	Log::View::Destroy();
}

void renderTree(const glm::mat4 mvp,
				const Node *root)
{
	std::stack<std::pair<const Node *, glm::mat4>> renderStack;
	renderStack.push(std::make_pair(root, glm::mat4(1.0f)));

	while (!renderStack.empty())
	{
		const auto current = renderStack.top();
		renderStack.pop();
		const Node *currNode = current.first;
		const glm::mat4 currTransform = current.second * currNode->get_transform();
		currNode->render(mvp, currTransform);

		for (size_t i = 0; i < currNode->get_children_nb(); ++i)
		{
			renderStack.push(std::make_pair(currNode->get_child(i), currTransform));
		}
	}
}

void edaf80::Assignment4::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 2.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.01;

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/fallback.vert"},
											  {ShaderType::fragment, "EDAF80/fallback.frag"}},
											 fallback_shader);
	if (fallback_shader == 0u)
	{
		LogError("Failed to load fallback shader");
		return;
	}

	//
	// Insert the creation of other shader programs.
	GLuint ocean_normal_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_ocean.vert"},
											  {ShaderType::fragment, "EDAF80/my_ocean_normal.frag"}},
											 ocean_normal_shader);
	if (ocean_normal_shader == 0u)
	{
		LogError("Failed to load ocean_normal shader");
	}

	GLuint ocean_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_ocean.vert"},
											  {ShaderType::fragment, "EDAF80/my_ocean.frag"}},
											 ocean_shader);
	if (ocean_shader == 0u)
	{
		LogError("Failed to load ocean shader");
	}

	GLuint ocean2_normal_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_ocean2.vert"},
											  {ShaderType::fragment, "EDAF80/my_ocean_normal.frag"}},
											 ocean2_normal_shader);
	if (ocean2_normal_shader == 0u)
	{
		LogError("Failed to load ocean2_normal shader");
	}

	GLuint ocean2_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_ocean2.vert"},
											  {ShaderType::fragment, "EDAF80/my_ocean.frag"}},
											 ocean2_shader);
	if (ocean2_shader == 0u)
	{
		LogError("Failed to load ocean2 shader");
	}

	GLuint ocean3_normal_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_ocean3.vert"},
											  {ShaderType::fragment, "EDAF80/my_ocean_normal.frag"}},
											 ocean3_normal_shader);
	if (ocean3_normal_shader == 0u)
	{
		LogError("Failed to load ocean2_normal shader");
	}

	GLuint ocean3_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_ocean3.vert"},
											  {ShaderType::fragment, "EDAF80/my_ocean.frag"}},
											 ocean3_shader);
	if (ocean3_shader == 0u)
	{
		LogError("Failed to load ocean2 shader");
	}

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_skybox.vert"},
											  {ShaderType::fragment, "EDAF80/my_skybox.frag"}},
											 skybox_shader);
	if (skybox_shader == 0u)
	{
		LogError("Failed to load skybox shader");
	}
	//
	//

	//
	//Uniform setters
	const auto no_uniforms = [](GLuint /*program*/) {};

	auto camera_position = mCamera.mWorld.GetTranslation();
	auto water_deep_color = glm::vec3(0.1f, 0.15f, 0.2f);
	auto water_shallow_color = glm::vec3(0.07f, 0.3f, 0.5f);

	float bumpValueScale = 1.0f;
	auto texScale = glm::vec2(8.0f, 4.0f);
	auto bumpSpeed = glm::vec2(-0.05f, 0.0f);

	float reflectionScale = 1.0f;
	float refractionScale = 1.0f;

	const size_t MAX_WAVES = 6;
	std::vector<float> waveAmplitudes{1.0f, 0.5f};
	std::vector<glm::vec2> waveDirections{glm::vec2(-1.0f, 0.0f), glm::vec2(-0.7f, 0.7f)};
	std::vector<float> waveFrequencies{0.2f, 0.4f};
	std::vector<float> wavePhases{0.5f, 1.3f};
	std::vector<float> waveSpikies{2.0f, 2.0f};
	float waveSpaceScale = 50.0f;
	uint editSelectedWave = 0;

	const auto insertWave = [&waveAmplitudes, &waveDirections, &waveFrequencies, &wavePhases, &waveSpikies](const float amp, const glm::vec2 dir, const float freq, const float phase, const float spike) {
		if (waveAmplitudes.size() < MAX_WAVES)
		{
			waveAmplitudes.push_back(amp);
			waveDirections.push_back(dir);
			waveFrequencies.push_back(freq);
			wavePhases.push_back(phase);
			waveSpikies.push_back(spike);
		}
	};

	const auto deleteWave = [&waveAmplitudes, &waveDirections, &waveFrequencies, &wavePhases, &waveSpikies](const uint waveID) {
		if (waveID >= waveAmplitudes.size())
			return;
		waveAmplitudes.erase(waveAmplitudes.begin() + waveID);
		waveDirections.erase(waveDirections.begin() + waveID);
		waveFrequencies.erase(waveFrequencies.begin() + waveID);
		wavePhases.erase(wavePhases.begin() + waveID);
		waveSpikies.erase(waveSpikies.begin() + waveID);
	};

	auto const ocean_set_uniforms =
		[&camera_position, &water_deep_color, &water_shallow_color, &waveAmplitudes, &waveDirections, &waveFrequencies, &wavePhases, &waveSpikies, &waveSpaceScale, &bumpValueScale, &texScale, &bumpSpeed, &reflectionScale, &refractionScale](GLuint program) {
			glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
			glUniform3fv(glGetUniformLocation(program, "water_deep_color"), 1, glm::value_ptr(water_deep_color));
			glUniform3fv(glGetUniformLocation(program, "water_shallow_color"), 1, glm::value_ptr(water_shallow_color));

			glUniform1fv(glGetUniformLocation(program, "waveSpaceScale"), 1, reinterpret_cast<GLfloat *>(&waveSpaceScale));

			glUniform1f(glGetUniformLocation(program, "nowTime"), static_cast<float>(fmod(GetTimeSeconds(), 100.0)));
			glUniform1fv(glGetUniformLocation(program, "waveBumpScale"), 1, reinterpret_cast<GLfloat *>(&bumpValueScale));
			glUniform2fv(glGetUniformLocation(program, "texScale"), 1, glm::value_ptr(texScale));
			glUniform2fv(glGetUniformLocation(program, "bumpSpeed"), 1, glm::value_ptr(bumpSpeed));

			glUniform1fv(glGetUniformLocation(program, "reflectionScale"), 1, reinterpret_cast<GLfloat *>(&reflectionScale));
			glUniform1fv(glGetUniformLocation(program, "refractionScale"), 1, reinterpret_cast<GLfloat *>(&refractionScale));

			//Waves!
			glUniform1ui(glGetUniformLocation(program, "num_waves"), waveAmplitudes.size());
			glUniform1fv(glGetUniformLocation(program, "waveAmplitudes"), waveAmplitudes.size(), reinterpret_cast<GLfloat *>(waveAmplitudes.data()));
			glUniform2fv(glGetUniformLocation(program, "waveDirections"), waveDirections.size() * 2, reinterpret_cast<GLfloat *>(waveDirections.data()));
			glUniform1fv(glGetUniformLocation(program, "waveFrequencies"), waveFrequencies.size(), reinterpret_cast<GLfloat *>(waveFrequencies.data()));
			glUniform1fv(glGetUniformLocation(program, "wavePhases"), wavePhases.size(), reinterpret_cast<GLfloat *>(wavePhases.data()));
			glUniform1fv(glGetUniformLocation(program, "waveSpikies"), waveSpikies.size(), reinterpret_cast<GLfloat *>(waveSpikies.data()));
		};
	//
	//

	//
	// Load geometry
	bonobo::mesh_data ocean_mesh = parametric_shapes::createOceanplate(50, 50, 40.0f);
	if (ocean_mesh.vao == 0u)
		LogError("Couldn't generate ocean_mesh!");
	const GLuint bump_tex = bonobo::loadTexture2D("waves.png", true);
	if (bump_tex == 0u)
		LogError("Couldn't load waves.png!");
	else
		ocean_mesh.bindings.insert({"bump_texture", bump_tex});

	Node root;

	GLuint current_water_shader = ocean_shader;

	Node ocean;
	ocean.set_geometry(ocean_mesh);
	ocean.set_program(&current_water_shader, ocean_set_uniforms);
	root.add_child(&ocean);

	Node magic;
	magic.set_translation(glm::vec3(0.0f, 40.0f, 0.0f));
	magic.set_rotation_x(glm::half_pi<float>());
	root.add_child(&magic);

	bonobo::mesh_data drop_mesh = parametric_shapes::createSphere(50, 50, 10.0f);
	if (drop_mesh.vao == 0u)
		LogError("Couldn't generate drop_mesh!");
	if (bump_tex != 0u)
		drop_mesh.bindings.insert({"bump_texture", bump_tex});

	Node drop;
	drop.set_geometry(drop_mesh);
	drop.set_program(&current_water_shader, ocean_set_uniforms);
	drop.set_scaling(glm::vec3(0.5));
	magic.add_child(&drop);

	bonobo::mesh_data ring_mesh = parametric_shapes::createTorus(50, 50, 12.5f, 4.0f);
	if (ring_mesh.vao == 0u)
		LogError("Couldn't generate ring_mesh!");
	if (bump_tex != 0u)
		ring_mesh.bindings.insert({"bump_texture", bump_tex});

	Node ring;
	ring.set_geometry(ring_mesh);
	ring.set_program(&current_water_shader, ocean_set_uniforms);
	magic.add_child(&ring);

	Node skybox;
	bonobo::mesh_data skybox_mesh = parametric_shapes::createQuad(1, 1);
	if (skybox_mesh.vao == 0u)
		LogError("Skybox generation failed???");
	else
		skybox.set_geometry(skybox_mesh);
	skybox.set_program(&skybox_shader, no_uniforms);
	//
	//

	//
	// Load cubemap
	GLuint cubemap_texture = myLoadCubemap("cloudyhills", true);
	if (cubemap_texture == 0u)
	{
		LogError("No cubemap? This won't look good.");
	}
	else
	{
		const std::array<Node *, 3> objects{&ocean, &drop, &ring};

		for (auto object : objects)
		{
			if (object->get_indices_nb() > 0)
			{
				object->add_texture("cubemap_texture", cubemap_texture, GL_TEXTURE_CUBE_MAP);
				object->do_a_barrel_roll();
			}
		}
		skybox.add_texture("cubemap_texture", cubemap_texture, GL_TEXTURE_CUBE_MAP);
	}
	//
	//

	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	glCullFace(GL_BACK);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool skybox_enable = true;

	// 0 - OceanV1 Normals
	// 1 - OceanV1
	// 2 - OceanV2 Normals
	// 3 - OceanV2
	// 4 - OceanV3 Normals
	// 5 - OceanV3
	uint shader_mode = 5u;

	while (!glfwWindowShouldClose(window))
	{
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick)
		{
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		//
		// Input handling
		auto &io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(ddeltatime, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED)
		{
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				LogError("Shader Program Reload Error!\nAn error occurred while reloading shader programs; see the logs for details.\nRendering is suspended until the issue is solved. Once fixed, just reload the shaders again.");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED)
		{
			polygon_mode = get_next_mode(polygon_mode);
		}

		ImGui_ImplGlfwGL3_NewFrame();

		switch (polygon_mode)
		{
		case polygon_mode_t::fill:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case polygon_mode_t::line:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case polygon_mode_t::point:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			glPointSize(8.0f);
			break;
		}
		//
		//

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		switch (shader_mode)
		{
		case 0:
			current_water_shader = ocean_normal_shader;
			break;
		case 1:
			current_water_shader = ocean_shader;
			break;
		case 2:
			current_water_shader = ocean2_normal_shader;
			break;
		case 3:
			current_water_shader = ocean2_shader;
			break;
		case 4:
			current_water_shader = ocean3_normal_shader;
			break;
		case 5:
			current_water_shader = ocean3_shader;
			break;
		}

		if (!shader_reload_failed)
		{
			camera_position = mCamera.mWorld.GetTranslation();
			const glm::mat4 mvp = mCamera.GetWorldToClipMatrix();

			renderTree(mvp, &root);

			if (skybox_enable)
			{
				skybox.render(mvp, glm::mat4(1.0f));
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Setup ImGui window
		bool scene_control_opened = ImGui::Begin("Scene Control", &scene_control_opened, ImVec2(300, 100), -1.0f, 0);
		if (scene_control_opened)
		{
			ImGui::Checkbox("Enable skybox", &skybox_enable);

			shader_mode &= 7u;
			if (ImGui::RadioButton("OceanV1 Normals", shader_mode == 0u))
				shader_mode = 0u;
			if (ImGui::RadioButton("OceanV1", shader_mode == 1u))
				shader_mode = 1u;
			if (ImGui::RadioButton("OceanV2 Normals (ABANDONED)", shader_mode == 2u))
				shader_mode = 2u;
			if (ImGui::RadioButton("OceanV2 (ABANDONED)", shader_mode == 3u))
				shader_mode = 3u;
			if (ImGui::RadioButton("OceanV3 Normals", shader_mode == 4u))
				shader_mode = 4u;
			if (ImGui::RadioButton("OceanV3", shader_mode == 5u))
				shader_mode = 5u;
		}
		ImGui::End();

		bool water_control_opened = ImGui::Begin("Water Control", &water_control_opened, ImVec2(300, 100), -1.0f, 0);
		if (water_control_opened)
		{
			ImGui::ColorEdit3("Deep water color", glm::value_ptr(water_deep_color));
			ImGui::ColorEdit3("Shallow water color", glm::value_ptr(water_shallow_color));
			ImGui::SliderFloat("Water space scale", &waveSpaceScale, 0.0f, 100.0f);
			ImGui::SliderFloat("Reflection Scale", &reflectionScale, 0.0f, 1.0f);
			ImGui::SliderFloat("Refraction Scale", &refractionScale, 0.0f, 1.0f);
			ImGui::Spacing();
			ImGui::SliderFloat("Water bumpmap scale", &bumpValueScale, 0.0f, 1.0f);
			ImGui::SliderFloat2("BumpTexScale", &texScale.x, 0.0f, 8.0f);
			ImGui::SliderFloat2("BumpSpeed", &bumpSpeed.x, -1.0f, 1.0f);
		}
		ImGui::End();

		bool wave_control_opened = ImGui::Begin("Wave Control", &wave_control_opened, ImVec2(300, 100), -1.0f, 0);
		if (wave_control_opened)
		{
			if (waveAmplitudes.size() < MAX_WAVES && ImGui::Button("Add wave!"))
			{
				editSelectedWave = static_cast<uint>(waveAmplitudes.size());
				insertWave(1.0f, glm::vec2(1.0f, 1.0f), 1.0f, 1.0f, 1.0f);
			}
			ImGui::Text("Wave count: %u", static_cast<uint>(waveAmplitudes.size()));
			for (uint i = 0; i < waveAmplitudes.size(); ++i)
			{
				if (ImGui::RadioButton(("Wave " + std::to_string(i + 1u)).c_str(), editSelectedWave == i))
				{
					editSelectedWave = i;
				}
			}
			if (editSelectedWave < waveAmplitudes.size())
			{
				const uint i = editSelectedWave;
				ImGui::Text("Wave: %u", i + 1u);
				ImGui::SliderFloat("Amplitude", &waveAmplitudes[i], 0.0f, 2.0f);
				ImGui::SliderFloat2("Direction", &waveDirections[i].x, -1.0f, 1.0f);
				ImGui::SliderFloat("Frequency", &waveFrequencies[i], 0.0, 2.0);
				ImGui::SliderFloat("Phase", &wavePhases[i], -1.0f, 1.0f);
				ImGui::SliderFloat("Spikiness", &waveSpikies[i], 1.0f, 10.0f);
				if (ImGui::SmallButton("Delete Wave"))
				{
					deleteWave(editSelectedWave);
					editSelectedWave = editSelectedWave - 1;
				}
			}
		}
		ImGui::End();

		bool opened2 = ImGui::Begin("Render Time", &opened2, ImVec2(120, 50), -1.0f, 0);
		if (opened2)
		{
			ImGui::Text("%.3f ms", ddeltatime);
			ImGui::Text("%.3f FPS", 1000.0 / ddeltatime);
		}
		ImGui::End();
		//
		//

		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();

		glfwSwapBuffers(window);
		lastTime = nowTime;
	}
}

int main()
{
	Bonobo::Init();
	try
	{
		edaf80::Assignment4 assignment4;
		assignment4.run();
	}
	catch (std::runtime_error const &e)
	{
		LogError(e.what());
	}
	Bonobo::Destroy();
}
