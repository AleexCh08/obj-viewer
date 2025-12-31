#include "InputController.h"

// Funciones auxiliares privadas (solo visibles en este archivo)
void translate(Model& model, glm::vec3 translation) {
    model.translationMatrix = glm::translate(model.translationMatrix, translation);
    model.updateTransformMatrix();
}

void scale(Model& model, float sx, float sy, float sz) {
    model.scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
    model.updateTransformMatrix();
}

void rotate(Model& model, glm::vec2 deltaMouse, float sensitivity) {
    float angleX = deltaMouse.y * sensitivity;
    float angleY = deltaMouse.x * sensitivity;
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
    model.rotationMatrix = rotationY * rotationX * model.rotationMatrix;
    model.updateTransformMatrix();
}

// --- Implementación pública ---

void InputController::handleModelTranslation(GLFWwindow* window, Model& model) {
    glm::vec3 translation(0.0f);
    const float speed = 0.002f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) translation.y += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) translation.y -= speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) translation.x -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) translation.x += speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) translation.z -= speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) translation.z += speed;
    
    if (translation != glm::vec3(0.0f)) translate(model, translation);
}

void InputController::handleModelScaling(GLFWwindow* window, Model& model) {
    const float scaleStep = 0.001f;
    static float lastSx = model.scaleX, lastSy = model.scaleY, lastSz = model.scaleZ;

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) model.scaleY = glm::clamp(model.scaleY + scaleStep, 0.1f, 10.0f);
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) model.scaleY = glm::clamp(model.scaleY - scaleStep, 0.1f, 10.0f);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) model.scaleX = glm::clamp(model.scaleX + scaleStep, 0.1f, 10.0f);
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) model.scaleX = glm::clamp(model.scaleX - scaleStep, 0.1f, 10.0f);
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) model.scaleZ = glm::clamp(model.scaleZ + scaleStep, 0.1f, 10.0f);
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) model.scaleZ = glm::clamp(model.scaleZ - scaleStep, 0.1f, 10.0f);

    if (model.scaleX != lastSx || model.scaleY != lastSy || model.scaleZ != lastSz) {
        scale(model, model.scaleX, model.scaleY, model.scaleZ);
        lastSx = model.scaleX; lastSy = model.scaleY; lastSz = model.scaleZ;
    }
}

void InputController::handleModelRotation(GLFWwindow* window, Model& model, glm::vec2& lastMousePos, float sensitivity) {
    static bool isRotating = false;
    static glm::vec2 initialMousePos;

    bool ctrlPressed = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
                       (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

    if (ctrlPressed && !isRotating) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        initialMousePos = glm::vec2(mouseX, mouseY);
        lastMousePos = initialMousePos;
        isRotating = true;
    } else if (!ctrlPressed && isRotating) {
        isRotating = false;
    }

    if (isRotating) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        glm::vec2 currentMousePos(mouseX, mouseY);
        glm::vec2 deltaMouse = currentMousePos - lastMousePos;
        lastMousePos = currentMousePos;
        rotate(model, deltaMouse, sensitivity);
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