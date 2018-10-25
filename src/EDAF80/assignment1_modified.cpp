#include "config.hpp"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"
#include "core/WindowManager.hpp"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>

#include <stack>
#include <vector>

#include <cstdlib>

#include <glm/gtc/type_ptr.hpp>

//Sets up the camera to follow a particular planet
//Values:
// 0 - No planet
// 1 - Lowas
// 2 - Lolar
// 3 - Lohac
// 4 - Prospit
// 5 - Prospit's moon
// 6 - Derse
// 7 - Dersite moon
int FOLLOW_PLANET = 0;
int FOLLOW_LAST_PLANET = 0;
glm::vec3 FOLLOW_LAST_POS = glm::vec3();
glm::vec3 FOLLOW_POS_RESET = glm::vec3();

//A crash function.
//Saves at least twenty loc over the length of the file.
int crash(const char *msg)
{
    LogError(msg);
    Log::View::Destroy();
    Log::Destroy();
    return EXIT_FAILURE;
}

//A shader compiler function.
//Saves maybe 4 loc?
GLuint getShaderProgramSet(ShaderProgramManager &program_manager, const std::string vertName, const std::string fragName)
{
    GLuint shader = 0u;
    program_manager.CreateAndRegisterProgram(
        {{ShaderType::vertex, "EDAF80/" + vertName + ".vert"}, {ShaderType::fragment, "EDAF80/" + fragName + ".frag"}},
        shader);
    return shader;
}

//! \brief Recursive node tree rendering function with parentTransform
//!
//! @param [in] The root node of the tree we're rendering
//! @param [in] WVP Matrix transforming from world-space to clip-space
//! @param [in] Matrix multiplications to reach parent transformation
void renderNodeTree(const Node *root, const glm::mat4 &worldToClipMatrix, const glm::mat4 &parentTransform)
{
    for (size_t i = 0; i < root->get_children_nb(); i++)
    {
        renderNodeTree(root->get_child(i), worldToClipMatrix, parentTransform * root->get_transform());
    }
    root->render(worldToClipMatrix, parentTransform * root->get_transform());
}

//! \brief Recursive node tree rendering function
//!
//! @param [in] The root node of the tree we're rendering
//! @param [in] WVP Matrix transforming from world-space to clip-space
void renderNodeTree(const Node *root, const glm::mat4 &worldToClipMatrix)
{
    for (size_t i = 0; i < root->get_children_nb(); i++)
    {
        renderNodeTree(root->get_child(i), worldToClipMatrix, root->get_transform());
    }
    root->render(worldToClipMatrix, root->get_transform());
}

//! \brief Recursive node tree rendering function with overwritten program and a parentTransform
//!
//! @param [in] The root node of the tree we're rendering
//! @param [in] WVP Matrix transforming from world-space to clip-space
//! @param [in] Matrix multiplications to reach parent transformation
void renderNodeTreeWithProgram(const Node *root, const glm::mat4 &worldToClipMatrix, GLuint program, std::function<void(GLuint)> const &set_uniforms, const glm::mat4 &parentTransform)
{
    for (size_t i = 0; i < root->get_children_nb(); i++)
    {
        renderNodeTreeWithProgram(root->get_child(i), worldToClipMatrix, program, set_uniforms, parentTransform * root->get_transform());
    }
    root->render(worldToClipMatrix, parentTransform * root->get_transform(), program, set_uniforms);
}

//! \brief Recursive node tree rendering function with overwritten program
//!
//! @param [in] The root node of the tree we're rendering
//! @param [in] WVP Matrix transforming from world-space to clip-space
void renderNodeTreeWithProgram(const Node *root, const glm::mat4 &worldToClipMatrix, GLuint program, std::function<void(GLuint)> const &set_uniforms)
{
    for (size_t i = 0; i < root->get_children_nb(); i++)
    {
        renderNodeTreeWithProgram(root->get_child(i), worldToClipMatrix, program, set_uniforms, root->get_transform());
    }
    root->render(worldToClipMatrix, root->get_transform(), program, set_uniforms);
}

std::pair<glm::vec3, bool> getNodeWorldPosition(const Node *root, const Node *tgt, const glm::mat4 &parentTransform = glm::mat4(1.0f))
{
    if (root == tgt)
        return std::pair<glm::vec3, bool>(glm::vec3(parentTransform * tgt->get_transform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)), true);

    for (size_t i = 0; i < root->get_children_nb(); i++)
    {
        auto r = getNodeWorldPosition(root->get_child(i), tgt, parentTransform * root->get_transform());
        if (r.second)
        {
            return r;
        }
    }
    return std::pair<glm::vec3, bool>(glm::vec3(), false);
}

int main()
{
    Log::Init();
    Log::View::Init();

    //Input and camera gen
    InputHandler input_handler;
    FPSCameraf camera(0.5f * glm::half_pi<float>(),
                      static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
                      0.01f, 1000.0f);
    camera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 18.0f));
    camera.mMouseSensitivity = 0.003f;
    camera.mMovementSpeed = 0.25f * 12.0f;

    //Get a window
    WindowManager window_manager;
    WindowManager::WindowDatum window_datum{input_handler, camera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};
    GLFWwindow *window = window_manager.CreateWindow("MatthewD EDAF80: Assignment 1", window_datum, config::msaa_rate);
    if (window == nullptr)
    {
        return crash("Failed to get a window! Exiting...");
    }

    //Get sphere geometry
    std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("sphere.obj");
    if (objects.empty())
    {
        return crash("Failed to load sphere geometry! Exiting...");
    }
    bonobo::mesh_data const &sphere = objects.front();

    //Get box geometry
    std::vector<bonobo::mesh_data> const objects2 = bonobo::loadObjects("box.obj");
    if (objects2.empty())
    {
        return crash("Failed to load box geometry! Exiting...");
    }
    bonobo::mesh_data const &box = objects2.front();

    //Allocate (on stack) shader program manager
    ShaderProgramManager program_manager;
    //Get default shader program
    GLuint default_shader = getShaderProgramSet(program_manager, "default", "default");
    if (default_shader == 0u)
    {
        return crash("Failed to generate default shader program! Exiting...");
    }
    //Get Skaian planet shader program
    GLuint homestuck_planet_shader = getShaderProgramSet(program_manager, "homestuck", "homestuck_planet");
    if (homestuck_planet_shader == 0u)
    {
        return crash("Failed to generate homestuck_planet shader program! Exiting...");
    }
    //Get Skaian gate shader program
    GLuint homestuck_gate_shader = getShaderProgramSet(program_manager, "homestuck", "homestuck_gate");
    if (homestuck_gate_shader == 0u)
    {
        return crash("Failed to generate homestuck_gate shader program! Exiting...");
    }
    //Get far ring asteroid shader program
    GLuint homestuck_roid_shader = getShaderProgramSet(program_manager, "homestuck", "homestuck_roids");
    if (homestuck_roid_shader == 0u)
    {
        return crash("Failed to generate homestuck_roid shader program! Exiting...");
    }

    auto const planetUniformFuncGen = [](const glm::mat4 &colorSpectra, const float &smoothing = 0.0f, const float &blockify = 0.0f) {
        return [&colorSpectra, &smoothing, &blockify](GLuint program) {
            const GLuint colorSpectraLoc = glGetUniformLocation(program, "colorSpectra");
            glUniformMatrix4fv(colorSpectraLoc, 1, GL_FALSE, &colorSpectra[0][0]);
            const GLuint blockifyLoc = glGetUniformLocation(program, "blockify");
            glUniform1f(blockifyLoc, blockify);
            const GLuint smoothingLoc = glGetUniformLocation(program, "smoothing");
            glUniform1f(smoothingLoc, smoothing);
        };
    };

    //Setup skaia node
    Node skaia_node;
    skaia_node.set_geometry(sphere);
    skaia_node.set_program(
        &homestuck_planet_shader,
        planetUniformFuncGen(
            glm::mat4(
                glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                glm::vec4(0.0f, 0.7f, 0.8f, 0.5f),
                glm::vec4(0.0f, 0.45f, 0.6f, 0.8f),
                glm::vec4(0.0f, 0.2f, 0.4f, 1.0f)),
            0.5f,
            0.0f));
    float const skaia_spin_speed = -glm::half_pi<float>() / 9.0f;
    skaia_node.set_scaling(glm::vec3(2.0f));

    float const planet_distance = 7.0f;

    //Setup lowas node
    Node lowas_node;
    lowas_node.set_geometry(sphere);
    lowas_node.set_program(&homestuck_planet_shader, planetUniformFuncGen(
                                                         glm::mat4(
                                                             glm::vec4(0.29f, 0.309f, 0.309f, 0.4f),
                                                             glm::vec4(0.1f, 0.25f, 0.25f, 0.5f),
                                                             glm::vec4(0.0f, 0.282f, 0.494f, 0.7f),
                                                             glm::vec4(0.0f, 0.0f, 0.0f, 0.9f)),
                                                         0.5f, 0.0f));
    float const lowas_spin_speed = glm::half_pi<float>() / 30.0f;
    lowas_node.set_translation(glm::vec3(0.0f, 0.0f, planet_distance));

    //Setup lolar node
    Node lolar_node;
    lolar_node.set_geometry(sphere);
    lolar_node.set_program(&homestuck_planet_shader, planetUniformFuncGen(
                                                         glm::mat4(
                                                             glm::vec4(0.9f, 0.9f, 0.0f, 0.0f),
                                                             glm::vec4(0.8f, 0.3f, 0.3f, 0.35f),
                                                             glm::vec4(0.0f, 0.6f, 0.7f, 0.4f),
                                                             glm::vec4(0.9f, 0.9f, 0.9f, 0.7f)),
                                                         0.3f, 0.0f));
    float const lolar_spin_speed = glm::pi<float>() / 30.0f;
    lolar_node.set_translation(glm::vec3(-planet_distance, 0.0f, 0.0f));

    //Setup lohac node
    Node lohac_node;
    lohac_node.set_geometry(sphere);
    lohac_node.set_program(&homestuck_planet_shader, planetUniformFuncGen(
                                                         glm::mat4(
                                                             glm::vec4(0.89f, 0.3f, 0.05f, 0.05f),
                                                             glm::vec4(0.89f, 0.05f, 0.0f, 0.6f),
                                                             glm::vec4(0.3f, 0.2f, 0.2f, 0.78f),
                                                             glm::vec4(0.2f, 0.1f, 0.1f, 0.9f)),
                                                         0.9f, 0.9f));
    float const lohac_spin_speed = -glm::half_pi<float>() / 30.0f;
    lohac_node.set_translation(glm::vec3(0.0f, 0.0f, -planet_distance));

    //Planet orbital node
    Node planets;
    float const planets_spin_speed = glm::half_pi<float>() / 60.0f;
    for (Node const *child : {&lowas_node, &lolar_node, &lohac_node})
    {
        planets.add_child(child);
    }

    //Setup propsit node
    auto const prospit_uniforms = planetUniformFuncGen(
        glm::mat4(
            glm::vec4(1.0f, 1.0f, 0.6f, 0.0f),
            glm::vec4(0.889f, 0.798f, 0.2f, 0.4f),
            glm::vec4(0.789f, 0.598f, 0.1f, 1.0f),
            glm::vec4(0.689f, 0.298f, 0.0f, 1.0f)),
        1.0f, 0.8f);
    Node prospit_orbit;
    Node prospit_node;
    prospit_node.set_geometry(sphere);
    prospit_node.set_scaling(glm::vec3(0.1f));
    prospit_node.set_program(&homestuck_planet_shader, prospit_uniforms);
    prospit_node.set_translation(glm::vec3(2.3f, 0.0f, 0.0f));
    prospit_node.set_rotation_y(glm::half_pi<float>() / 2.0f);
    prospit_orbit.add_child(&prospit_node);
    float const prospit_orbit_speed = glm::half_pi<float>() / 10.0f;
    float const prospit_spin_speed = glm::half_pi<float>() / 4.0f;

    Node prospit_barycenter;
    prospit_node.add_child(&prospit_barycenter);
    Node prospit_moon;
    prospit_moon.set_geometry(sphere);
    prospit_moon.set_scaling(glm::vec3(0.33f));
    prospit_moon.set_program(&homestuck_planet_shader, prospit_uniforms);
    prospit_moon.set_translation(glm::vec3(1.0f, 0.0f, 1.0f));
    prospit_barycenter.add_child(&prospit_moon);

    //Setup derse node
    auto const derse_uniforms = planetUniformFuncGen(
        0.6f * glm::mat4(
                   glm::vec4(1.0f, 0.6f, 1.0f, 0.0f),
                   glm::vec4(0.889f, 0.2f, 0.798f, 0.4f),
                   glm::vec4(0.789f, 0.1f, 0.598f, 1.0f),
                   glm::vec4(0.689f, 0.0f, 0.298f, 1.0f)),
        1.0f, 1.0f);
    Node derse_node;
    derse_node.set_geometry(sphere);
    derse_node.set_scaling(glm::vec3(0.1f));
    derse_node.set_program(&homestuck_planet_shader, derse_uniforms);
    derse_node.set_translation(glm::vec3(-12.0f, 0.0f, -12.0f));
    float const derse_spin_speed = 0.5f * prospit_spin_speed;

    Node derse_moon;
    derse_moon.set_geometry(sphere);
    derse_moon.set_scaling(glm::vec3(0.33f));
    derse_moon.set_program(&homestuck_planet_shader, derse_uniforms);
    derse_moon.set_translation(glm::vec3(1.0f, 0.0f, 1.0f));
    derse_node.add_child(&derse_moon);

    Node far_ring_rocks;
    far_ring_rocks.set_geometry(sphere);
    far_ring_rocks.set_program(&homestuck_roid_shader, [](GLuint /*program*/) {});
    far_ring_rocks.set_scaling(glm::vec3(14.0f, 0.05f, 14.0f));

    Node far_ring;
    far_ring.add_child(&derse_node);
    far_ring.add_child(&far_ring_rocks);
    float const far_ring_speed = 0.7 * planets_spin_speed;

    //Setup battlefield
    Node battlefield;
    battlefield.set_geometry(sphere);
    battlefield.set_scaling(glm::vec3(0.3f, 0.3f, 0.3f));
    battlefield.set_program(&homestuck_planet_shader, planetUniformFuncGen(
                                                          glm::mat4(
                                                              glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                                                              glm::vec4(0.0f, 0.8f, 0.0f, 0.3f),
                                                              glm::vec4(1.0f, 1.0f, 1.0f, 0.4f),
                                                              glm::vec4(0.01f, 0.01f, 0.01f, 0.9f)),
                                                          0.6f, 1.0f));

    //Setup Skaian gate nodes
    auto const gate_uniform_func =
        [](GLuint program) {
            double nowTime = GetTimeSeconds();
            double frac = nowTime - floor(nowTime);
            double mod = static_cast<double>((static_cast<uint>(floor(nowTime))) % 100);
            glUniform1f(glGetUniformLocation(program, "t"), mod + frac);
        };
    auto const first_gate_distance_from_skaia = 1.2f;
    auto const gate_distance_increment = 0.5f;
    Node gate_root;
    Node lowas_gate_nodes[7];
    for (size_t i = 0; i < 7; ++i)
    {
        auto &gate_node = lowas_gate_nodes[7 - 1 - i];
        gate_node.set_geometry(box);
        gate_node.set_rotation_x(glm::half_pi<float>());
        gate_node.set_scaling(glm::vec3(0.1f, 1.0f, 0.1f));
        gate_node.set_translation(glm::vec3(0.0f, 0.0f, first_gate_distance_from_skaia + gate_distance_increment * static_cast<float>(7 - i)));
        gate_node.set_program(&homestuck_gate_shader, gate_uniform_func);
        gate_root.add_child(lowas_gate_nodes + i);
    }
    Node lolar_gate_nodes[7];
    for (size_t i = 0; i < 7; ++i)
    {
        auto &gate_node = lolar_gate_nodes[7 - 1 - i];
        gate_node.set_geometry(box);
        gate_node.set_rotation_z(glm::half_pi<float>());
        gate_node.set_rotation_y(-glm::half_pi<float>());
        gate_node.set_scaling(glm::vec3(0.1f, 1.0f, 0.1f));
        gate_node.set_translation(glm::vec3(-first_gate_distance_from_skaia - gate_distance_increment * static_cast<float>(7 - i), 0.0f, 0.0f));
        gate_node.set_program(&homestuck_gate_shader, gate_uniform_func);
        gate_root.add_child(lolar_gate_nodes + i);
    }
    Node lohac_gate_nodes[7];
    for (size_t i = 0; i < 7; ++i)
    {
        auto &gate_node = lohac_gate_nodes[7 - 1 - i];
        gate_node.set_geometry(box);
        gate_node.set_rotation_x(-glm::half_pi<float>());
        gate_node.set_rotation_z(-glm::pi<float>());
        gate_node.set_scaling(glm::vec3(0.1f, 1.0f, 0.1f));
        gate_node.set_translation(glm::vec3(0.0f, 0.0f, -first_gate_distance_from_skaia - gate_distance_increment * static_cast<float>(7 - i)));
        gate_node.set_program(&homestuck_gate_shader, gate_uniform_func);
        gate_root.add_child(lohac_gate_nodes + i);
    }

    //Root scene node
    Node root_node;
    for (Node const *child : {&skaia_node, &planets, &gate_root, &prospit_orbit, &far_ring, &battlefield})
    {
        root_node.add_child(child);
    }

    // Retrieve the actual framebuffer size: for HiDPI monitors, you might
    // end up with a framebuffer larger than what you actually asked for.
    // For example, if you ask for a 1920x1080 framebuffer, you might get a
    // 3840x2160 one instead.
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);
    //Setup clean depth and colors
    glClearDepthf(1.0f);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    bool show_logs = true;
    bool show_gui = true;

    size_t fpsSamples = 0;
    double lastTime = GetTimeSeconds();
    double fpsNextTick = lastTime + 1.0;

    while (!glfwWindowShouldClose(window))
    {
        //Compute timing information
        double const nowTime = GetTimeSeconds();
        double const delta_time = nowTime - lastTime;
        lastTime = nowTime;
        if (nowTime > fpsNextTick)
        {
            fpsNextTick += 1.0;
            fpsSamples = 0;
        }
        ++fpsSamples;

        //Process inputs
        glfwPollEvents();

        ImGuiIO const &io = ImGui::GetIO();
        input_handler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
        input_handler.Advance();
        camera.Update(delta_time, input_handler);

        //Update camera
        std::pair<glm::vec3, bool> r;
        switch (FOLLOW_PLANET)
        {
        case 1: //Lowas
            r = getNodeWorldPosition(&root_node, &lowas_node);
            break;
        case 2: //Lolar
            r = getNodeWorldPosition(&root_node, &lolar_node);
            break;
        case 3: //Lohac
            r = getNodeWorldPosition(&root_node, &lohac_node);
            break;
        case 4: //Prospit
            r = getNodeWorldPosition(&root_node, &prospit_node);
            break;
        case 5: //Prospitan moon
            r = getNodeWorldPosition(&root_node, &prospit_moon);
            break;
        case 6: //Derse
            r = getNodeWorldPosition(&root_node, &derse_node);
            break;
        case 7: //Dersite moon
            r = getNodeWorldPosition(&root_node, &derse_moon);
            break;
        default:
            //Do nothing.
            break;
        }
        if (FOLLOW_PLANET && r.second)
        {
            camera.mWorld.LookAt(r.first);
            if (!FOLLOW_LAST_PLANET && FOLLOW_PLANET)
            {
                FOLLOW_POS_RESET = camera.mWorld.GetTranslation();
            }
            const glm::vec3 cameraToPlanet = r.first - camera.mWorld.GetTranslation();
            const float l = glm::length(cameraToPlanet);
            if (l > 3.0f)
            {
                camera.mWorld.Translate(static_cast<float>(delta_time) * 0.5f * (l - 3.0f) * glm::normalize(cameraToPlanet));
            }
            if (FOLLOW_LAST_PLANET == FOLLOW_PLANET)
            {
                const glm::vec3 moved = r.first - FOLLOW_LAST_POS;
                camera.mWorld.Translate(moved);
            }
            FOLLOW_LAST_POS = r.first;
        }
        else if (!FOLLOW_PLANET && FOLLOW_LAST_PLANET)
        {
            camera.mWorld.SetTranslate(FOLLOW_POS_RESET);
            camera.mWorld.LookAt(glm::vec3(0.0f));
        }
        FOLLOW_LAST_PLANET = FOLLOW_PLANET;

        if (input_handler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
            show_logs = !show_logs;
        if (input_handler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
            show_gui = !show_gui;

        //Start a new DearImGui frame
        ImGui_ImplGlfwGL3_NewFrame();

        //Clear screen
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        //Update transforms
        skaia_node.rotate_y(skaia_spin_speed * delta_time);
        lowas_node.rotate_y(lowas_spin_speed * delta_time);
        lolar_node.rotate_y(lolar_spin_speed * delta_time);
        lohac_node.rotate_y(lohac_spin_speed * delta_time);
        planets.rotate_y(planets_spin_speed * delta_time);
        prospit_orbit.rotate_z(prospit_orbit_speed * delta_time);
        prospit_node.rotate_y(prospit_spin_speed * delta_time);
        prospit_barycenter.rotate_y(-1.1f * prospit_spin_speed * delta_time);
        prospit_moon.rotate_y(-0.5f * prospit_spin_speed * delta_time);
        far_ring.rotate_y(far_ring_speed * delta_time);
        far_ring_rocks.rotate_y(0.1f * far_ring_speed * delta_time);
        derse_node.rotate_y(derse_spin_speed * delta_time);
        gate_root.rotate_y(planets_spin_speed * delta_time);

        //Traverse the scene graph and render all nodes
        renderNodeTree(&root_node, camera.GetWorldToClipMatrix());
        //Now, I could see from the stacks given that a stack-based
        // implementation was preferred, but because of the tree
        // structure the stack should only grow to a logarithm of the
        // number of children. Add to that the fact I can shuttle
        // around a combined matrix of all parent transforms, and I
        // may actually be saving runtime speed through locality of
        // space and fewer matrix multiplications.
        //            I'm going recursive. Sue me.

        //Render the transparent objects after the opaque pass -- DEPRECATED
        //Transparency now handled using "discard" keyword in fragment shader.
        //Unfortunately, gates still need their own pass to fetch current time.
        //glDepthMask(GL_FALSE);
        //glEnable(GL_BLEND);
        //glBlendEquation(GL_FUNC_ADD);
        //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        //far_ring_rocks.render(camera.GetWorldToClipMatrix(),far_ring_rocks.get_transform());
        //glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
        //renderNodeTreeWithProgram(&gate_root,camera.GetWorldToClipMatrix(),homestuck_gate_shader,gate_uniform_func);
        //glDisable(GL_BLEND);
        //glDepthMask(GL_TRUE);

        //Display DearImGui windows
        if (show_logs)
            Log::View::Render();
        if (show_gui)
        {
            ImGui::DragInt("FOLLOW_PLANET", &FOLLOW_PLANET, 0.1f, 0, 7);
            ImGui::Text("%s", (std::to_string(delta_time)).c_str());
            ImGui::Render();
        }

        //Queue the computed frame for display
        glfwSwapBuffers(window);
    }

    Log::View::Destroy();
    Log::Destroy();
    return EXIT_SUCCESS;
}
