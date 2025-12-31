#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "../Scene/Model.h"

class InputController {
public:
    static void handleModelTranslation(GLFWwindow* window, Model& model);
    static void handleModelScaling(GLFWwindow* window, Model& model);
    static void handleModelRotation(GLFWwindow* window, Model& model, glm::vec2& lastMousePos, float sensitivity);
    static void changeBackgroundColor(GLFWwindow* window, glm::vec3& bgColor);
};