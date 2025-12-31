#include "Camera.h"

Camera::Camera(int screenWidth, int screenHeight, glm::vec3 startEye, glm::vec3 startTarget) {
    this->width = (float)screenWidth;
    this->height = (float)screenHeight;
    this->eye = startEye;
    this->target = startTarget;
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Valores por defecto que tenías en el main
    this->sensitivity = 0.4f;
    this->speed = 0.005f;
    
    this->isDragging = false;
    this->lastMousePos = glm::vec2(0.0f, 0.0f);
}

void Camera::updateScreenSize(int newWidth, int newHeight) {
    this->width = (float)newWidth;
    this->height = (float)newHeight;
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(eye, target, up);
}

glm::mat4 Camera::getProjectionMatrix() {
    return glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
}

void Camera::handleInput(GLFWwindow* window) {
    // Verificar si el botón izquierdo del ratón está presionado
    // Y asegurarnos de NO estar tocando la interfaz de ImGui
    if (!ImGui::IsAnyItemHovered() && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!isDragging) {
            // Si no se estaba arrastrando, capturar la posición inicial
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            lastMousePos = glm::vec2(mouseX, mouseY);
            isDragging = true;
        } else {       
            // Si se está arrastrando, calcular el movimiento
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            glm::vec2 currentMousePos(mouseX, mouseY);
            glm::vec2 deltaMouse = currentMousePos - lastMousePos;
            lastMousePos = currentMousePos;

            float angleX = deltaMouse.y * sensitivity;
            float angleY = deltaMouse.x * sensitivity;

            // Rotar alrededor del objetivo
            glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), up);
            glm::vec3 right = glm::normalize(glm::cross(up, glm::normalize(target - eye)));
            glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), right);
            
            // Combinar rotaciones
            glm::mat4 rotation = rotationX * rotationY;

            // Actualizar la posición del ojo (eye)
            eye = glm::vec3(rotation * glm::vec4(eye - target, 1.0f)) + target;

            // Limitar la rotación vertical para que no se invierta la cámara
            if (eye.y < 0.0f) {
                eye.y = 0.0f;
            }
        }
    } else {           
        isDragging = false; 
    }

    // Manejar el movimiento hacia adelante y atrás (Zoom con flechas)
    glm::vec3 direction = glm::normalize(target - eye);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        eye += direction * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        eye -= direction * speed;
    }
}