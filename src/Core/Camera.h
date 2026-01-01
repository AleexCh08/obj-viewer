#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../imgui/imgui.h" 

class Camera {
public:
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up;

    float sensitivity;
    float speed;
    float width;
    float height;

    Camera(int screenWidth, int screenHeight, glm::vec3 startEye, glm::vec3 startTarget);

    void handleInput(GLFWwindow* window);
    void ProcessMouseScroll(float yoffset);
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    void updateScreenSize(int newWidth, int newHeight);

private:
    bool isDragging;
    glm::vec2 lastMousePos;
};