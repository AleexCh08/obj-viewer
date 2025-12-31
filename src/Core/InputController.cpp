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
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) bgColor = glm::vec3(0.0f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) bgColor = glm::vec3(0.46f, 0.46f, 0.46f);
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) bgColor = glm::vec3(0.46f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) bgColor = glm::vec3(0.0f, 0.46f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) bgColor = glm::vec3(0.0f, 0.0f, 0.46f);
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) bgColor = glm::vec3(0.46f, 0.46f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) bgColor = glm::vec3(0.46f, 0.0f, 0.46f);
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) bgColor = glm::vec3(0.0f, 0.46f, 0.46f);
}