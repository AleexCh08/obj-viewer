#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Window {
public:
    static GLFWwindow* Init(int width, int height, const char* title);
private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};