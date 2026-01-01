#include "Window.h"

#define STB_IMAGE_IMPLEMENTATION 
#include "../include/stb_image.h"

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

    GLFWimage images[1]; 
    int channels;
    images[0].pixels = stbi_load("icon.png", &images[0].width, &images[0].height, &channels, 4); 

    if (images[0].pixels) {
        glfwSetWindowIcon(window, 1, images); 
        stbi_image_free(images[0].pixels); 
    } else {
        std::cout << "No se pudo cargar icon.png" << std::endl;
    }

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