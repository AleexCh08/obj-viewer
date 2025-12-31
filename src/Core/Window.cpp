#include "Window.h"

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

GLFWwindow* Window::Init(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW" << std::endl;
        return nullptr;
    }
    glfwWindowHint(GLFW_SAMPLES, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "No se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "No se pudo inicializar Glad" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glEnable(GL_DEPTH_TEST);
    return window;
}