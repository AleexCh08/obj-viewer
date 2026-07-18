#include "Window.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 
#include "../include/stb_image.h"

GLFWcursor* Window::customCursor = nullptr;

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

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);

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

    GLFWimage cursorImg;
    cursorImg.pixels = stbi_load("cursor.png", &cursorImg.width, &cursorImg.height, &channels, 4);
    if (cursorImg.pixels) {
        customCursor = glfwCreateCursor(&cursorImg, 0, 0); 
        if (customCursor) {
            glfwSetCursor(window, customCursor);
        }
        stbi_image_free(cursorImg.pixels);
    } else {
        std::cout << "No se pudo cargar cursor.png. Se usara el cursor por defecto del SO." << std::endl;
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

void Window::ShowSplashScreen(GLFWwindow* window, const char* imagePath) {
    int originalWidth, originalHeight;
    glfwGetWindowSize(window, &originalWidth, &originalHeight);

    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
    int splashSize = 512;
    glfwSetWindowSize(window, splashSize, splashSize);
    glViewport(0, 0, splashSize, splashSize);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowPos(window, (mode->width - splashSize) / 2, (mode->height - splashSize) / 2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(imagePath, &width, &height, &nrChannels, 4); // Forzar 4 canales
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        std::cerr << "Advertencia: No se pudo cargar el splash screen: " << imagePath << std::endl;
        stbi_image_free(data);
    }

    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "   TexCoords = aTexCoords;\n"
        "}\0";
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoords;\n"
        "uniform sampler2D splashTex;\n"
        "void main() {\n"
        "   FragColor = texture(splashTex, TexCoords);\n"
        "}\n\0";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); glCompileShader(fragmentShader);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader); glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader); glDeleteShader(fragmentShader);

    float quadVertices[] = {
         1.0f,  1.0f,  1.0f, 1.0f, // Arriba derecha
         1.0f, -1.0f,  1.0f, 0.0f, // Abajo derecha
        -1.0f, -1.0f,  0.0f, 0.0f, // Abajo izquierda
        -1.0f,  1.0f,  0.0f, 1.0f  // Arriba izquierda 
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))); glEnableVertexAttribArray(1);

    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "splashTex"), 0);

    double startTime = glfwGetTime();
    while (glfwGetTime() - startTime < 2.5) {
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents(); 
    }

    glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram); glDeleteTextures(1, &textureID);

    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE); 
    glfwSetWindowSize(window, originalWidth, originalHeight); 
    glfwSetWindowPos(window, (mode->width - originalWidth) / 2, (mode->height - originalHeight) / 2); 
    glViewport(0, 0, originalWidth, originalHeight);

    glEnable(GL_DEPTH_TEST);
}