#include "Camera.h"

Camera::Camera(int screenWidth, int screenHeight, glm::vec3 startEye, glm::vec3 startTarget) {
    this->width = (float)screenWidth;
    this->height = (float)screenHeight;
    this->eye = startEye;
    this->target = startTarget;
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    this->sensitivity = 0.4f;
    this->speed = 0.005f;
    
    this->isDragging = false;
    this->lastMousePos = glm::vec2(0.0f, 0.0f);
}

void Camera::updateScreenSize(int newWidth, int newHeight) {
    this->width = (float)newWidth;
    this->height = (float)newHeight;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(eye, target, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
}

void Camera::handleInput(GLFWwindow* window) {
    if (!ImGui::IsAnyItemHovered() && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!isDragging) {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            lastMousePos = glm::vec2(mouseX, mouseY);
            isDragging = true;
        } else {       
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            glm::vec2 currentMousePos(mouseX, mouseY);
            glm::vec2 deltaMouse = currentMousePos - lastMousePos;
            lastMousePos = currentMousePos;

            float angleX = deltaMouse.y * sensitivity;
            float angleY = deltaMouse.x * sensitivity;

            glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), up);
            glm::vec3 right = glm::normalize(glm::cross(up, glm::normalize(target - eye)));
            glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), right);
            glm::mat4 rotation = rotationX * rotationY;

            eye = glm::vec3(rotation * glm::vec4(eye - target, 1.0f)) + target;
            if (eye.y < 0.1f) {
                eye.y = 0.1f;
            }
        }
    } else {           
        isDragging = false; 
    }
}

void Camera::ProcessMouseScroll(float yoffset) {
    float zoomSpeed = 0.5f; 
    float minHeight = 0.1f; 
    float minDistance = 1.0f; 

    float distance = glm::distance(eye, target);
    float newDistance = distance - (yoffset * zoomSpeed);
    if (newDistance < minDistance) newDistance = minDistance;

    glm::vec3 direction = glm::normalize(eye - target);
    eye = target + (direction * newDistance);

    if (eye.y < minHeight) {
        eye.y = minHeight;

    }
}