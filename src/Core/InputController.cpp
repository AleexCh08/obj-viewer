#include "InputController.h"

void InputController::handleModelRotation(GLFWwindow* window, Model& model, glm::vec2& lastMousePos, float sensitivity) {
    static bool isRotating = false;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        
        if (!isRotating) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            
            double x, y; 
            glfwGetCursorPos(window, &x, &y);
            lastMousePos = glm::vec2(x, y);
            isRotating = true;
        }

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        glm::vec2 currentMousePos(mouseX, mouseY);
        glm::vec2 delta = currentMousePos - lastMousePos;
        lastMousePos = currentMousePos; 
        model.rotation.y += delta.x * sensitivity;
        model.rotation.x += delta.y * sensitivity;
        
        model.updateTransformMatrix();

    } else if (isRotating) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        isRotating = false;
    }
}

void InputController::changeBackgroundColor(GLFWwindow* window, glm::vec3& bgColor) {
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) bgColor = glm::vec3(0.15f, 0.15f, 0.15f); // Gris Oscuro (Estilo CAD)
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) bgColor = glm::vec3(0.10f, 0.18f, 0.24f); // Azul Blueprint
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) bgColor = glm::vec3(0.22f, 0.20f, 0.18f); // Estudio Cálido
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) bgColor = glm::vec3(0.46f, 0.46f, 0.46f); // Gris Neutro (Tu original)
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) bgColor = glm::vec3(0.0f, 0.0f, 0.0f);    // Negro Puro
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) bgColor = glm::vec3(0.85f, 0.85f, 0.85f); // Luz de Estudio/Blanco
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) bgColor = glm::vec3(0.15f, 0.18f, 0.20f); // Gris Maya (Estilo Autodesk)
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) bgColor = glm::vec3(0.10f, 0.20f, 0.20f); // Turquesa Oscuro
}