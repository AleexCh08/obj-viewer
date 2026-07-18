#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Window {
public:
    static GLFWwindow* Init(int width, int height, const char* title);
    static void ShowSplashScreen(GLFWwindow* window, const char* imagePath);
    static GLFWcursor* customCursor;
private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};