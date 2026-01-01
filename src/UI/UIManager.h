#pragma once

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../Scene/Model.h"
#include <Scene/SceneManager.h>
#include <glm/glm.hpp>
#include <vector>

// Esta estructura guardará toda la configuración de tu programa
struct UIState {
    float vertexSize = 5.0f;
    glm::vec3 vertexColor = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 wireframeColor = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 normalsColor = glm::vec3(0.7f, 0.7f, 0.7f);
    glm::vec3 boundingBoxColor = glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec3 newColor = glm::vec3(1.0f, 1.0f, 1.0f); // Color para rellenar

    bool showVertices = false;
    bool showWireframe = false;
    bool showNormals = false;
    bool enableDepthTest = true;
    bool enableBackFaceCulling = true;
    bool showFPS = true;
    bool enableAntialiasing = true;
    bool showBoundingBox = true;
    bool enableColorChange = false;
    bool showPropertiesPanel = true;
};

class UIManager {
public:
    static void Init(GLFWwindow* window);
    static void Shutdown();
    
    // Función principal que dibuja toda la interfaz
    static void Render(GLFWwindow* window, UIState& state, std::vector<Model>& models, int& selectedModelIndex, double fps);
};