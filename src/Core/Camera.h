#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Necesario para detectar si el mouse está sobre la UI
#include "../imgui/imgui.h" 

class Camera {
public:
    // Vectores principales
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up;

    // Configuración
    float sensitivity;
    float speed;
    float width;
    float height;

    // Constructor
    Camera(int screenWidth, int screenHeight, glm::vec3 startEye, glm::vec3 startTarget);

    // Método principal que llamaremos en cada frame
    void handleInput(GLFWwindow* window);

    // Obtener matrices calculadas
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();

    // Actualizar tamaño de pantalla (si redimensionas la ventana)
    void updateScreenSize(int newWidth, int newHeight);

private:
    // Estado interno para el arrastre del mouse
    bool isDragging;
    glm::vec2 lastMousePos;
};