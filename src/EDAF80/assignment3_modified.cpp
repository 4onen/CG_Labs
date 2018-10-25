#include "assignment3_modified.hpp"
#include "interpolation_modified.hpp"
#include "parametric_shapes_modified.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"
#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

#include <array>
#include <stack>

#include "external/lodepng.h"
#include "core/helpers.hpp"

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

edaf80::Assignment3::Assignment3() : mCamera(0.5f * glm::half_pi<float>(),
											 static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
											 0.01f, 1000.0f),
									 inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 3", window_datum, config::msaa_rate);
	if (window == nullptr)
	{
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment3::~Assignment3()
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

void renderTreeWithProgram(const glm::mat4 mvp,
						   const Node *root,
						   const GLuint shader,
						   std::function<void(GLuint)> set_uniforms)
{
	std::stack<std::pair<const Node *, glm::mat4>> renderStack;
	renderStack.push(std::make_pair(root, glm::mat4(1.0f)));

	while (!renderStack.empty())
	{
		const auto current = renderStack.top();
		renderStack.pop();
		const Node *currNode = current.first;
		const glm::mat4 currTransform = current.second * currNode->get_transform();
		currNode->render(mvp, currTransform, shader, set_uniforms);

		for (size_t i = 0; i < currNode->get_children_nb(); ++i)
		{
			renderStack.push(std::make_pair(currNode->get_child(i), currTransform));
		}
	}
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

float randf()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void edaf80::Assignment3::run()
{
	//Generate geometry
	constexpr unsigned char num_meshes = 4;
	std::array<bonobo::mesh_data, num_meshes> shapes;
	shapes[0] = parametric_shapes::createCircleRing(2u, 64u, 0.3f, 1.0f);
	shapes[1] = parametric_shapes::createSphere(64u, 64u, 1.0f);
	shapes[2] = parametric_shapes::createTorus(64u, 64u, 1.0f, 0.3f);
	shapes[3] = parametric_shapes::createZoneplate(64u, 64u, 2.0f, 0.05f);

	const std::array<std::pair<const char *, bonobo::mesh_data *>, 2> textures_to_fetch{
		std::make_pair("stone47", &shapes[0]), std::make_pair("earth", &shapes[1])
		//, std::make_pair("fieldstone",&shapes[2])
		//, std::make_pair("holes",&shapes[3])
	};

	for (auto &tdat : textures_to_fetch)
	{
		const std::string stringName = std::string(tdat.first);
		const GLuint diffuse_tex = bonobo::loadTexture2D(stringName + "_diffuse.png", false);
		if (diffuse_tex == 0u)
			LogError("Couldn't find %s_diffuse.png", tdat.first);
		else
			tdat.second->bindings.insert({"diffuse_texture", diffuse_tex});

		const GLuint bump_tex = bonobo::loadTexture2D(stringName + "_bump.png", false);
		if (bump_tex == 0u)
			LogError("Couldn't find %s_bump.png", tdat.first);
		else
			tdat.second->bindings.insert({"bump_texture", bump_tex});
	}

	{
		const GLuint diffuse_tex = bonobo::loadTexture2D("MediumGrass/Grass_Albedo.png", false);
		if (diffuse_tex == 0u)
			LogError("Couldn't find MediumGrass/Grass_Albedo.png");
		else
			shapes[3].bindings.insert({"diffuse_texture", diffuse_tex});

		const GLuint bump_tex = bonobo::loadTexture2D("MediumGrass/Grass_Normal.png", true);
		if (bump_tex == 0u)
			LogError("Couldn't find MediumGrass/Grass_Normal.png");
		else
			shapes[3].bindings.insert({"bump_texture", bump_tex});

		const GLuint metalness_tex = bonobo::loadTexture2D("MediumGrass/Grass_Metallic.png", false);
		if (metalness_tex == 0u)
			LogError("Couldn't find MediumGrass/Grasss_Metallic.png");
		else
			shapes[3].bindings.insert({"opacity_texture", metalness_tex});
		//I'm hijacking the opacity code to flag presence of my metalness texture.
	}

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.005f;

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

	GLuint normal_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_normal.vert"},
											  {ShaderType::fragment, "EDAF80/my_normal.frag"}},
											 normal_shader);
	if (normal_shader == 0u)
		LogError("Failed to load normal shader");

	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/texcoord.vert"},
											  {ShaderType::fragment, "EDAF80/texcoord.frag"}},
											 texcoord_shader);
	if (texcoord_shader == 0u)
		LogError("Failed to load texcoord shader");

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_phong.vert"},
											  {ShaderType::fragment, "EDAF80/my_phong.frag"}},
											 phong_shader);
	if (phong_shader == 0u)
		LogError("Failed to load phong shader");

	GLuint bump_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_bump.vert"},
											  {ShaderType::fragment, "EDAF80/my_bump.frag"}},
											 bump_shader);
	if (bump_shader == 0u)
		LogError("Failed to load bump shader");

	GLuint cube_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_cubemap.vert"},
											  {ShaderType::fragment, "EDAF80/my_cubemap.frag"}},
											 cube_shader);
	if (cube_shader == 0u)
		LogError("Failed to load cubemap shader");

	GLuint metalness_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_cubemap.vert"},
											  {ShaderType::fragment, "EDAF80/my_metalness.frag"}},
											 metalness_shader);
	if (metalness_shader == 0u)
		LogError("Failed to load cubemap shader");

	GLuint sky_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/my_skybox.vert"},
											  {ShaderType::fragment, "EDAF80/my_skybox.frag"}},
											 sky_shader);
	if (sky_shader == 0u)
		LogError("Failed to load skybox shader");

	auto constexpr no_uniforms = [](GLuint /*program*/) {};

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.0f, 0.0f, 0.02f);
	auto diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 10.0f;
	auto const phong_set_uniforms = [&light_position, &camera_position, &ambient, &diffuse, &specular, &shininess](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};

	Node scene_root;

	const char *cubemapName = "sunset_sky";
	const GLuint aCubemap = myLoadCubemap(std::string(cubemapName), false);
	if (aCubemap == 0u)
	{
		LogError("Couldn't load cubemap %s", cubemapName);
	}

	constexpr unsigned char num_nodes = num_meshes;
	std::array<Node, num_nodes> nodes;
	for (unsigned char i = 0; i < num_nodes; ++i)
	{
		nodes[i] = Node();
		nodes[i].set_geometry(shapes[i]);
		nodes[i].set_program(&fallback_shader, no_uniforms);
		nodes[i].set_rotation_x(-glm::two_pi<float>() * randf());
		nodes[i].set_scaling(glm::vec3(0.5f + randf(), 0.5f + randf(), 0.5f + randf()));
		nodes[i].set_translation(glm::vec3(std::fmaf(static_cast<float>(i), 3.0f, -6.0f), 0.0f, 0.0f));
		if (aCubemap != 0u)
		{
			nodes[i].add_texture("cubemap_texture", aCubemap, GL_TEXTURE_CUBE_MAP);
			nodes[i].do_a_barrel_roll();
		}
		scene_root.add_child(&nodes[i]);
	}

	Node lightIndicator;
	lightIndicator.set_geometry(shapes[1]);
	lightIndicator.set_program(&fallback_shader, no_uniforms);
	lightIndicator.set_scaling(glm::vec3(0.05));

	Node skybox;
	if (aCubemap != 0u)
	{
		const auto quad = parametric_shapes::createQuad(2u, 2u);
		if (quad.vao == 0u)
		{
			LogError("Skybox parametric_shapes::createQuad didn't work.");
		}
		skybox.set_geometry(quad);
		skybox.set_program(&sky_shader, no_uniforms);
		skybox.add_texture("cubemap_texture", aCubemap, GL_TEXTURE_CUBE_MAP);
	}
	else
	{
		LogError("No skybox? A disaster!");
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//Programs are:
	// 0 - fallback_shader
	// 1 - normal_shader
	// 2 - texcoord_shader
	// 3 - phong_shader
	// 4 - bump_shader
	// 5 - cubemap_shader
	unsigned char shader_program_selected = 0;

	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;

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

		auto &io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(ddeltatime, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;

		ImGui_ImplGlfwGL3_NewFrame();

		for (unsigned char i = GLFW_KEY_0; i < GLFW_KEY_9 + 1; ++i)
		{
			if (inputHandler.GetKeycodeState(i) & JUST_PRESSED)
			{
				shader_program_selected = i - GLFW_KEY_0 - 1;
			}
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED)
		{
			polygon_mode = get_next_mode(polygon_mode);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED)
		{
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				LogError("Shader Program Reload Error\nAn error occurred while reloading shader programs; see the logs for details.\nRendering is suspended until the issue is solved. Once fixed, just reload the shaders again.");
		}
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
			break;
		}

		camera_position = mCamera.mWorld.GetTranslation();

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		auto shader = fallback_shader;
		switch (shader_program_selected)
		{
		case 1:
			shader = normal_shader;
			break;
		case 2:
			shader = texcoord_shader;
			break;
		case 3:
			shader = phong_shader;
			break;
		case 4:
			shader = bump_shader;
			break;
		case 5:
			shader = cube_shader;
			break;
		case 6:
			shader = metalness_shader;
			break;
		}
		const glm::mat4 mvp = mCamera.GetWorldToClipMatrix();
		for (auto &node : nodes)
		{
			node.rotate_y(1.0f / 1000.0f * ddeltatime);
		}
		renderTreeWithProgram(mvp, &scene_root, shader, phong_set_uniforms);

		if (shader_program_selected > 1)
		{
			lightIndicator.set_translation(light_position);
			lightIndicator.render(mvp, lightIndicator.get_transform());
		}

		if (shader_program_selected >= 5)
			skybox.render(mvp, glm::mat4(1.0f));

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened)
		{
			ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(specular));
			ImGui::SliderFloat("Shininess", &shininess, 0.0f, 250.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
		ImGui::End();

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
		edaf80::Assignment3 assignment3;
		assignment3.run();
	}
	catch (std::runtime_error const &e)
	{
		LogError(e.what());
	}
	Bonobo::Destroy();
}
