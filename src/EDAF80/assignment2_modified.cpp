#include "assignment2_modified.hpp"
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
#include "external/imgui_impl_glfw_gl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

#include <array>

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

edaf80::Assignment2::Assignment2() : mCamera(0.5f * glm::half_pi<float>(),
											 static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
											 0.01f, 1000.0f),
									 inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 2", window_datum, config::msaa_rate);
	if (window == nullptr)
	{
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment2::~Assignment2()
{
	Log::View::Destroy();
}

void renderTree(const glm::mat4 mvp,
				const Node *root,
				const glm::mat4 parent_transform = glm::mat4(1.0f))
{
	for (size_t i = 0; i < root->get_children_nb(); ++i)
	{
		renderTree(mvp, root->get_child(i), parent_transform * root->get_transform());
	}
	root->render(mvp, parent_transform * root->get_transform());
}

void renderTreeWithProgram(const glm::mat4 mvp,
						   const Node *root,
						   const GLuint shader,
						   std::function<void(GLuint)> set_uniforms,
						   const glm::mat4 parent_transform = glm::mat4(1.0f))
{
	for (size_t i = 0; i < root->get_children_nb(); ++i)
	{
		renderTreeWithProgram(mvp, root->get_child(i), shader, set_uniforms, parent_transform * root->get_transform());
	}
	root->render(mvp, parent_transform * root->get_transform(), shader, set_uniforms);
}

float pickSphereRad()
{
	return 0.5 + static_cast<float>(std::rand()) / static_cast<float>(2u * RAND_MAX);
}

//Rotation matrix to align positive y-axis to d.
const glm::mat4 rotationAlignPosY(const glm::vec3 d)
{
	const glm::vec3 v = glm::vec3(-d.z, 0.0f, d.x);
	const float c = d.y;
	const float k = 1.0f / (1.0f + c);

	return glm::mat4(v.x * v.x * k + c, v.y * v.x * k - v.z, v.z * v.x * k + v.y, 0.0f, v.x * v.y * k + v.z, v.y * v.y * k + c, v.z * v.y * k - v.x, 0.0f, v.x * v.z * k - v.y, v.y * v.z * k + v.x, v.z * v.z * k + c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

//Rotation matrix to align positive z-axis to d.
const glm::mat4 rotationAlignPosZ(const glm::vec3 d)
{
	const glm::vec3 v = glm::vec3(d.y, -d.x, 0.0f);
	const float c = d.z;
	const float k = 1.0f / (1.0f + c);

	return glm::mat4(v.x * v.x * k + c, v.y * v.x * k - v.z, v.z * v.x * k + v.y, 0.0f, v.x * v.y * k + v.z, v.y * v.y * k + c, v.z * v.y * k - v.x, 0.0f, v.x * v.z * k - v.y, v.y * v.z * k + v.x, v.z * v.z * k + c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

void edaf80::Assignment2::run()
{
	// Load the sphere geometry
	constexpr size_t num_shapes = 18;
	bonobo::mesh_data shapes[num_shapes];
	shapes[0] = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	shapes[1] = parametric_shapes::createQuad(1u, 1u);
	shapes[2] = parametric_shapes::createSphere(3u, 3u, 1.0f);
	shapes[3] = parametric_shapes::createSphere(4u, 4u, 1.0f);
	shapes[4] = parametric_shapes::createSphere(8u, 8u, pickSphereRad());
	shapes[5] = parametric_shapes::createSphere(16u, 16u, pickSphereRad());
	shapes[6] = parametric_shapes::createSphere(32u, 32u, pickSphereRad());
	shapes[7] = parametric_shapes::createSphere(64u, 64u, pickSphereRad());
	shapes[8] = parametric_shapes::createSphere(64u, 3u, pickSphereRad());
	shapes[9] = parametric_shapes::createSphere(3u, 64u, pickSphereRad());
	shapes[10] = parametric_shapes::createTorus(4u, 4u, 2.0f * pickSphereRad(), pickSphereRad());
	shapes[11] = parametric_shapes::createTorus(8u, 8u, 2.0f, pickSphereRad());
	shapes[12] = parametric_shapes::createTorus(16u, 16u, 2.0f * pickSphereRad(), pickSphereRad());
	shapes[13] = parametric_shapes::createTorus(32u, 32u, 2.0f * pickSphereRad(), pickSphereRad());
	shapes[14] = parametric_shapes::createTorus(64u, 64u, 2.0f * pickSphereRad(), pickSphereRad());
	shapes[15] = parametric_shapes::createTorus(64u, 4u, 2.0f * pickSphereRad(), pickSphereRad());
	shapes[16] = parametric_shapes::createTorus(4u, 64u, 2.0f * pickSphereRad(), pickSphereRad());
	shapes[17] = parametric_shapes::createZoneplate(128u, 128u, 4.0f * pickSphereRad(), 0.5f * pickSphereRad());

	for (size_t i = 0; i < num_shapes; ++i)
	{
		if (shapes[i].vao == 0u)
		{
			LogError("One of these procedural shapes generated badly. It was the %dth one.", i);
			return;
		}
	}

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;

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

	GLuint diffuse_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/diffuse.vert"},
											  {ShaderType::fragment, "EDAF80/diffuse.frag"}},
											 diffuse_shader);
	if (diffuse_shader == 0u)
		LogError("Failed to load diffuse shader");

	GLuint normal_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/normal.vert"},
											  {ShaderType::fragment, "EDAF80/normal.frag"}},
											 normal_shader);
	if (normal_shader == 0u)
		LogError("Failed to load normal shader");

	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/texcoord.vert"},
											  {ShaderType::fragment, "EDAF80/texcoord.frag"}},
											 texcoord_shader);
	if (texcoord_shader == 0u)
		LogError("Failed to load texcoord shader");

	GLuint tangent_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/tangent.vert"},
											  {ShaderType::fragment, "EDAF80/tangent.frag"}},
											 tangent_shader);
	if (tangent_shader == 0u)
		LogError("Failed to load tangent shader");

	GLuint binormal_shader = 0u;
	program_manager.CreateAndRegisterProgram({{ShaderType::vertex, "EDAF80/binormal.vert"},
											  {ShaderType::fragment, "EDAF80/binormal.frag"}},
											 binormal_shader);
	if (binormal_shader == 0u)
		LogError("Failed to load binormal shader");

	auto const set_uniforms = [](GLuint program) {
		const glm::vec3 light_position(-2.0f, 4.0f, 2.0f);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.5f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = true;

	Node root;

	constexpr size_t num_nodes = 18;
	std::array<Node, num_nodes> nodes;
	for (size_t i = 0; i < num_nodes; ++i)
	{
		nodes[i] = Node();
		nodes[i].set_geometry(shapes[i]);
	}
	//circle_rings
	root.add_child(&nodes[0]);
	//quad
	nodes[1].set_translation(glm::vec3(6.0f, -0.5f, 0.0f));
	root.add_child(&nodes[1]);
	//zoneplate
	nodes[17].set_translation(glm::vec3(4.0f, 0.0f, 0.0f));
	root.add_child(&nodes[17]);

	//spheres
	Node sphere_parent;
	for (size_t i = 2; i < 10; ++i)
	{
		nodes[i].set_translation(glm::vec3(0.0f, 14.0f - 2.0f * static_cast<float>(i), 0.0f));
		sphere_parent.add_child(&nodes[i]);
	}
	sphere_parent.set_translation(glm::vec3(-4.0f, 0.0f, 0.0f));
	root.add_child(&sphere_parent);

	//tesselated tori
	Node torus_parent;
	for (size_t i = 10; i < 17; ++i)
	{
		nodes[i].set_translation(glm::vec3(0.0f, 14.0f - 2.0f * static_cast<float>(i - 6), 0.0f));
		nodes[i].set_rotation_x(glm::half_pi<float>());
		torus_parent.add_child(&nodes[i]);
	}
	torus_parent.set_translation(glm::vec3(-8.0f, 0.0f, 0.0f));
	root.add_child(&torus_parent);

	//Interpolation node markers
	constexpr unsigned int max_interpolation_markers = 20;
	int num_control_points = 10;	  // In interface
	float interpolation_speed = 2.0f; // In interface
	Node interpolation_parent;
	//interpolation_parent.set_scaling(glm::vec3(2.0f));
	interpolation_parent.set_translation(glm::vec3(0.0f, 0.0f, 12.0f));
	std::array<Node, max_interpolation_markers> interpolation_markers;
	for (unsigned int i = 0; i < max_interpolation_markers; ++i)
	{
		interpolation_markers[i].set_geometry(shapes[11]);
		interpolation_markers[i].set_program(&normal_shader, set_uniforms);
		interpolation_parent.add_child(&interpolation_markers[i]);
	}
	Node interpolation_runner;
	interpolation_runner.set_geometry(shapes[16]);
	interpolation_runner.set_scaling(glm::vec3(0.1f));
	interpolation_runner.set_program(&normal_shader, set_uniforms);
	interpolation_parent.add_child(&interpolation_runner);
	Node interpolation_derivative;
	interpolation_derivative.set_geometry(shapes[3]);
	interpolation_derivative.set_scaling(glm::vec3(0.5f));
	interpolation_derivative.set_program(&normal_shader, set_uniforms);
	interpolation_runner.add_child(&interpolation_derivative);

	//Cull modes are:
	// 0 - glDisable(GL_CULL_FACE)
	// 1 - glCullFace(GL_FRONT)
	// 2 - glCullFace(GL_BACK)
	uint gl_cull_mode = 0;

	//Programs are:
	// 0 - fallback_shader
	// 1 - diffuse_shader
	// 2 - normal_shader
	// 3 - texcoord_shader
	uint shader_program_selected = 0;

	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	bool show_logs = true;
	bool show_gui = true;

	while (!glfwWindowShouldClose(window))
	{
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick)
		{
			fpsNextTick += 1.0;
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

		if (inputHandler.GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED)
		{
			shader_program_selected = 0;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED)
		{
			shader_program_selected = 1;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED)
		{
			shader_program_selected = 2;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED)
		{
			shader_program_selected = 3;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_5) & JUST_PRESSED)
		{
			shader_program_selected = 4;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_6) & JUST_PRESSED)
		{
			shader_program_selected = 5;
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED)
		{
			polygon_mode = get_next_mode(polygon_mode);
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

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.1f, 0.2f, 0.1f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		auto shader = fallback_shader;
		switch (shader_program_selected)
		{
		case 1:
			shader = diffuse_shader;
			break;
		case 2:
			shader = normal_shader;
			break;
		case 3:
			shader = texcoord_shader;
			break;
		case 4:
			shader = tangent_shader;
			break;
		case 5:
			shader = binormal_shader;
		}
		renderTreeWithProgram(mCamera.GetWorldToClipMatrix(), &root, shader, set_uniforms);

		//Interpolate the movement of a shape between various
		// control points
		std::array<glm::vec3, max_interpolation_markers> control_points;
		for (unsigned int i = 0; i < num_control_points; ++i)
		{
			const float t = 2.0f * glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(num_control_points - 1);

			//Path generator:
			control_points[i] = glm::vec3(2.0f * cos(t), 2.0f * sin(t), t * t * t * t * glm::two_over_pi<float>() / 1024.0f);
			interpolation_markers[i].set_translation(control_points[i]);
		}
		const glm::mat4 wtc = mCamera.GetWorldToClipMatrix();
		const glm::mat4 pt = interpolation_parent.get_transform();
		for (unsigned int i = 0; i < num_control_points; ++i)
		{
			glm::vec3 derivativeDir;
			if (use_linear)
			{
				interpolation_markers[i].set_scaling(glm::vec3(0.025f));
				const glm::vec3 &p0 = control_points[i],
								&p1 = control_points[(i + 1) % num_control_points];
				derivativeDir = glm::normalize(p1 - p0);
			}
			else
			{
				interpolation_markers[i].set_scaling(glm::vec3(0.2f));
				const glm::vec3 &p0 = control_points[(i + num_control_points - 1) % num_control_points],
								&p1 = control_points[i],
								&p2 = control_points[(i + 1) % num_control_points],
								&p3 = control_points[(i + 2) % num_control_points];
				derivativeDir = glm::normalize(interpolation::evalCatmullRomDerivative(p0, p1, p2, p3, catmull_rom_tension, 0.0f));
			}
			interpolation_markers[i].render(wtc, pt * (interpolation_markers[i].get_transform()) * rotationAlignPosZ(derivativeDir), shader, set_uniforms);
		}

		const float applied_speed = 0.1f * static_cast<float>(num_control_points) * interpolation_speed;
		const float f = applied_speed * fmod(nowTime, 1.0f / (applied_speed) * static_cast<double>(num_control_points));
		const float x = f - std::truncf(f);
		const unsigned int i = static_cast<int>(std::floor(f));
		glm::vec3 derivativeDir;
		if (use_linear)
		{
			const glm::vec3 &p0 = control_points[i],
							&p1 = control_points[(i + 1u) % num_control_points];
			interpolation_runner.set_translation(interpolation::evalLERP(p0, p1, x));
			derivativeDir = glm::normalize(p1 - p0);
			interpolation_derivative.set_translation(2.0f * derivativeDir);
		}
		else
		{
			const glm::vec3 &p0 = control_points[i],
							&p1 = control_points[(i + 1) % num_control_points],
							&p2 = control_points[(i + 2) % num_control_points],
							&p3 = control_points[(i + 3) % num_control_points];
			interpolation_runner.set_translation(interpolation::evalCatmullRom(p0, p1, p2, p3, catmull_rom_tension, x));
			derivativeDir =
				glm::normalize(interpolation::evalCatmullRomDerivative(p0, p1, p2, p3, catmull_rom_tension, x));
			interpolation_derivative.set_translation(2.0f * derivativeDir);
		}
		interpolation_runner.render(wtc, pt * interpolation_runner.get_transform() * rotationAlignPosY(derivativeDir), shader, set_uniforms);
		interpolation_derivative.render(wtc, pt * interpolation_runner.get_transform() * interpolation_derivative.get_transform(), shader, set_uniforms);

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened)
		{
			ImGui::Text("CPU Tesselated Poly controls:");
			if (ImGui::Button(("Polygon mode: " + std::to_string(static_cast<uint>(polygon_mode))).c_str()))
			{
				polygon_mode = get_next_mode(polygon_mode);
			}
			if (ImGui::Button(("Cull faces: " + std::to_string(gl_cull_mode)).c_str()))
			{
				gl_cull_mode = (gl_cull_mode + 1) % 3;
				switch (gl_cull_mode)
				{
				case 0:
					glDisable(GL_CULL_FACE);
					break;
				case 1:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);
					break;
				case 2:
					glCullFace(GL_BACK);
					break;
				default:
					break;
				}
			}
			ImGui::Text("");
			ImGui::Text("Interpolation controls:");
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.001f, 1.0f);
			ImGui::SliderInt("Number of interpolation control points", &num_control_points, 1, max_interpolation_markers);
			ImGui::SliderFloat("Interpolation speed", &interpolation_speed, 0.001f, 10.0f);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
		edaf80::Assignment2 assignment2;
		assignment2.run();
	}
	catch (std::runtime_error const &e)
	{
		LogError(e.what());
	}
	Bonobo::Destroy();
}
