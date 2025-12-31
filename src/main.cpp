// Librerias necesarias
#include "headers.h"
// Librerias estandar
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

GLuint gridVAO = 0;
GLuint gridVBO = 0;

// Función de callback para manejar cambios en el tamaño del framebuffer
static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Función de inicialización de GLFW y creación de la ventana
static GLFWwindow* initWindow(int width, int height, const char* title) {
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

// Shaders básicos:
// Shader de vertices
const char* vertexShaderSource = 
R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float pointSize;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);    
        gl_PointSize = pointSize;
    }
)";

// Shader de fragmentos
const char* fragmentShaderSource = 
R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;

    uniform vec3 objectColor;
    uniform vec3 vertexColor;
    uniform vec3 wireframeColor;
    uniform vec3 normalsColor;
    uniform vec3 boundingBoxColor;

    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPos;

    uniform bool useVertexColor;
    uniform bool useWireframeColor; 
    uniform bool useNormalsColor;
    uniform bool useBoundingBoxColor;

    uniform bool isGrid;
    uniform vec3 gridColor;

    void main() {
        if (isGrid) {
            FragColor = vec4(gridColor, 1.0); 
            return;
        }
        if (useBoundingBoxColor) {
            FragColor = vec4(boundingBoxColor, 1.0);
            return;
        }

        // Ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Specular
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;

        // Determinar el color final según el modo
        vec3 resultColor = objectColor;
        if (useVertexColor) {
            resultColor = vertexColor;
        } else if (useWireframeColor) {
            resultColor = wireframeColor; // Usar el color del alambrado
        } else {
            resultColor = objectColor;
        }

        vec3 result = (ambient + diffuse + specular) * resultColor;
        vec3 result2 = useNormalsColor ? normalsColor : result;

        FragColor = vec4(result2, 1.0);
    }
)";

// Función para compilar shaders
static GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error compilando shader: " << infoLog << std::endl;
    }

    return shader;
}

// Función para crear un programa de shaders
static GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Error enlazando programa de shaders: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Estructura para almacenar datos del modelo
struct Model {
    GLuint VAO, VBO, EBO;

    std::vector<float> vertices;
    std::vector<float> originalVertices;
    std::vector<unsigned int> indices;
    
    glm::mat4 rotationMatrix = glm::mat4(1.0f); // Matriz de rotación acumulativa
    glm::mat4 translationMatrix = glm::mat4(1.0f); // Matriz de traslación acumulativa
    glm::mat4 scaleMatrix = glm::mat4(1.0f); // Matriz de escala acumulativa
    glm::mat4 transformMatrix = glm::mat4(1.0f); // Matriz de transformación acumulada
    
    glm::vec3 color;         // Color actual del modelo
    glm::vec3 originalColor; // Color original del modelo (desde el MTL)

    glm::vec3 localMinBounds; // Mínimo local del modelo (sin transformar)
    glm::vec3 localMaxBounds; // Máximo local del modelo (sin transformar)
    
    float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;     // Escala en X, Y y Z
    
    Model() : VAO(0), VBO(0), EBO(0), color(0.7f, 0.7f, 0.7f), originalColor(0.7f, 0.7f, 0.7f), localMinBounds(0.0f), localMaxBounds(0.0f) {}

    void setupModel() {
        originalVertices = vertices;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void updateTransformMatrix() {
        transformMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    }

    // Aplicar la matriz de transformación a los vértices originales
    void applyTransformations() {    
        for (size_t i = 0; i < originalVertices.size(); i += 6) {
            glm::vec4 position(
                originalVertices[i],
                originalVertices[i + 1],
                originalVertices[i + 2],
                1.0f
            );

            position = transformMatrix * position;

            vertices[i] = position.x;
            vertices[i + 1] = position.y;
            vertices[i + 2] = position.z;
        }       
    }

    void draw(GLuint shaderProgram) const {
        glUseProgram(shaderProgram);

        GLuint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        glUniform3fv(colorLoc, 1, &color[0]);

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transformMatrix));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Función para normalizar un modelo dentro de un cubo unitario
static void normalizeModel(Model& model) {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        glm::vec3 pos(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }

    glm::vec3 size = maxBounds - minBounds;
    float scale = 1.0f / std::max(size.x, std::max(size.y, size.z)); // Cambio realizado
    glm::vec3 center = (minBounds + maxBounds) * 0.5f;

    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        model.vertices[i + 0] = (model.vertices[i + 0] - center.x) * scale;
        model.vertices[i + 1] = (model.vertices[i + 1] - center.y) * scale;
        model.vertices[i + 2] = (model.vertices[i + 2] - center.z) * scale;
    }
}

// Función para procesar el modelo cargado
static Model processModel(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, bool normalize) {
    Model model;

    // Paso 1: Crear un mapa para calcular normales por vértice
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Paso 2: Procesar cada triángulo
    for (const auto& shape : shapes) {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            unsigned int idx0 = shape.mesh.indices[i + 0].vertex_index;
            unsigned int idx1 = shape.mesh.indices[i + 1].vertex_index;
            unsigned int idx2 = shape.mesh.indices[i + 2].vertex_index;

            glm::vec3 v0(
                attrib.vertices[3 * idx0 + 0],
                attrib.vertices[3 * idx0 + 1],
                attrib.vertices[3 * idx0 + 2]
            );
            glm::vec3 v1(
                attrib.vertices[3 * idx1 + 0],
                attrib.vertices[3 * idx1 + 1],
                attrib.vertices[3 * idx1 + 2]
            );
            glm::vec3 v2(
                attrib.vertices[3 * idx2 + 0],
                attrib.vertices[3 * idx2 + 1],
                attrib.vertices[3 * idx2 + 2]
            );

            // Calcular la normal del triángulo
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));

            // Agregar vértices y acumular normales
            for (int j = 0; j < 3; ++j) {
                unsigned int idx = shape.mesh.indices[i + j].vertex_index;

                glm::vec3 pos(
                    attrib.vertices[3 * idx + 0],
                    attrib.vertices[3 * idx + 1],
                    attrib.vertices[3 * idx + 2]
                );

                if (vertices.size() <= idx || vertices[idx].position != pos) {
                    // Crear un nuevo vértice si es necesario
                    Vertex vertex = { pos, triangleNormal };
                    vertices.push_back(vertex);
                    indices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                else {
                    // Acumular la normal si el vértice ya existe
                    vertices[idx].normal += triangleNormal;
                    indices.push_back(idx);
                }
            }
        }
    }

    // Paso 3: Normalizar las normales por vértice
    for (auto& vertex : vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }

    // Paso 4: Convertir a formato de OpenGL
    for (const auto& vertex : vertices) {
        model.vertices.push_back(vertex.position.x);
        model.vertices.push_back(vertex.position.y);
        model.vertices.push_back(vertex.position.z);

        model.vertices.push_back(vertex.normal.x);
        model.vertices.push_back(vertex.normal.y);
        model.vertices.push_back(vertex.normal.z);
    }

    model.indices = indices;

    // Paso 5: Procesar materiales
    if (!materials.empty()) {
        model.color = glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]);
        model.originalColor = model.color; // Guardar el color original
    }
    else {
        model.color = glm::vec3(0.7f, 0.7f, 0.7f);
        model.originalColor = model.color; // Guardar color gris por defecto
        std::cerr << "No se encontro un archivo MTL asociado. (Gris por defecto)" << std::endl;
    }

    // Paso 6: Normalizar y configurar el modelo
    if (normalize) {
        normalizeModel(model);
    }  
    model.setupModel();

    // Calcular AABB local
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);
    for (size_t i = 0; i < model.originalVertices.size(); i += 6) {
        glm::vec3 pos(
            model.originalVertices[i],
            model.originalVertices[i + 1],
            model.originalVertices[i + 2]
        );
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }
    model.localMinBounds = minBounds;
    model.localMaxBounds = maxBounds;
    
    return model;
}

// Función para trasladar un modelo
static void translateModel(Model& model, glm::vec3 translation) {
    model.translationMatrix = glm::translate(model.translationMatrix, translation);
    model.updateTransformMatrix();
}

// Control de entrada para trasladar el modelo actual
static void handleModelTranslation(GLFWwindow* window, Model& model) {
    glm::vec3 translation(0.0f);
    const float speed = 0.002f; // Velocidad de traslación
    // W: Arriba, S: Abajo, A: Izquierda, D: Derecha, Q: Atras, E: Adelante
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        translation.y += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        translation.y -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        translation.x -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        translation.x += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        translation.z -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        translation.z += speed;
    }
    if (translation != glm::vec3(0.0f)) {
        translateModel(model, translation);
    }
}

// Función para escalar un modelo
static void scaleModel(Model& model, float sx, float sy, float sz) {
    model.scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
    model.updateTransformMatrix();
}

// Manejar la entrada para escalar el modelo
static void handleModelScaling(GLFWwindow* window, Model& model) {
    const float scaleStep = 0.001f; // Incremento para el escalamiento
    static float lastSx = model.scaleX, lastSy = model.scaleY, lastSz = model.scaleZ;

    // Y: Arriba, H: Abajo, G: Izquierda, J: Derecha, T: Atras, U: Adelante
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        model.scaleY = glm::clamp(model.scaleY + scaleStep, 0.1f, 10.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        model.scaleY = glm::clamp(model.scaleY - scaleStep, 0.1f, 10.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        model.scaleX = glm::clamp(model.scaleX + scaleStep, 0.1f, 10.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        model.scaleX = glm::clamp(model.scaleX - scaleStep, 0.1f, 10.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        model.scaleZ = glm::clamp(model.scaleZ + scaleStep, 0.1f, 10.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        model.scaleZ = glm::clamp(model.scaleZ - scaleStep, 0.1f, 10.0f);
    }

    // Aplicar escalamiento solo si hay cambios
    if (model.scaleX != lastSx || model.scaleY != lastSy || model.scaleZ != lastSz) {
        scaleModel(model, model.scaleX, model.scaleY, model.scaleZ);
        lastSx = model.scaleX;
        lastSy = model.scaleY;
        lastSz = model.scaleZ;
    }
}

// Función para rotar un modelo
static void rotateModel(Model& model, glm::vec2 deltaMouse, float sensitivity) {
    float angleX = deltaMouse.y * sensitivity;
    float angleY = deltaMouse.x * sensitivity;

    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));

    model.rotationMatrix = rotationY * rotationX * model.rotationMatrix;
    model.updateTransformMatrix();
}

// Manejar la entrada del ratón para rotar el modelo
static void handleModelRotation(GLFWwindow* window, Model& model, glm::vec2& lastMousePos, float sensitivity) {
    static bool isRotating = false; // Estado de rotación
    static glm::vec2 initialMousePos; // Posición inicial del ratón al presionar CTRL

    // Verificar si la tecla CTRL está presionada
    bool ctrlPressed = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

    if (ctrlPressed && !isRotating) {
        // Iniciar rotación: guardar posición inicial del ratón
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        initialMousePos = glm::vec2(mouseX, mouseY);
        lastMousePos = initialMousePos;
        isRotating = true;
    }
    else if (!ctrlPressed && isRotating) {
        // Finalizar rotación
        isRotating = false;
    }

    if (isRotating) {
        // Calcular movimiento relativo del ratón desde que se presionó CTRL
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        glm::vec2 currentMousePos(mouseX, mouseY);
        glm::vec2 deltaMouse = currentMousePos - lastMousePos;
        lastMousePos = currentMousePos;

        // Aplicar rotación proporcional al movimiento
        rotateModel(model, deltaMouse, sensitivity);
    }
}

// Manejar la entrada del ratón para rotar la escena y el movimiento de la cámara
static void handleSceneAndCamera(GLFWwindow* window, glm::mat4& viewMatrix, glm::vec3& eye, glm::vec3& target, glm::vec3& up, glm::vec2& lastMousePos, float sensitivity, float speed) {  
    static bool isDragging = false; // Variable para rastrear si se está arrastrando el ratón

    // Verificar si el botón izquierdo del ratón está presionado
    if (!ImGui::IsAnyItemHovered() && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!isDragging) {
            // Si no se estaba arrastrando, capturar la posición inicial del ratón
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            lastMousePos = glm::vec2(mouseX, mouseY);
            isDragging = true; // Indicar que se está arrastrando
        } else {       
            // Si se está arrastrando, calcular el movimiento del ratón
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
            glm::mat4 rotation = rotationX * rotationY;

            // Actualizar la posición del ojo
            eye = glm::vec3(rotation * glm::vec4(eye - target, 1.0f)) + target;

            // Limitar la rotación vertical
            if (eye.y < 0.0f) {
                eye.y = 0.0f;
            }

            viewMatrix = glm::lookAt(eye, target, up);
        }
    } else {           
        isDragging = false; // Si el botón izquierdo del ratón no está presionado, dejar de arrastrar
    }

    // Manejar el movimiento de la cámara hacia adelante y atrás
    glm::vec3 direction = glm::normalize(target - eye);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        eye += direction * speed;
        viewMatrix = glm::lookAt(eye, target, up); // Actualizar la vista cuando se mueve la cámara
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        eye -= direction * speed;
        viewMatrix = glm::lookAt(eye, target, up); // Actualizar la vista cuando se mueve la cámara
    }
}

// Eliminar todos los modelos de la escena
static void clearScene(std::vector<Model>& models) {
    for (auto& model : models) {
        glDeleteBuffers(1, &model.VBO);
        glDeleteBuffers(1, &model.EBO);
        glDeleteVertexArrays(1, &model.VAO);
    }
    models.clear();
    std::cout << "Escena eliminada correctamente.\n";
}


// Guardar la escena en un archivo de texto
static void saveScene(const std::string& filename, const std::vector<Model>& models) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo para guardar la escena.\n";
        return;
    }

    file << "# Archivo de escena en formato .OBJ\n";
    file << "# Formato: vertices(v) normales(vn) caras(f)\n";

    size_t vertexOffset = 0; // Desplazamiento para los índices de vértices

    for (const auto& model : models) {
        file << "o " << "\n";
        // Guardar vértices
        for (size_t i = 0; i < model.vertices.size(); i += 6) {
            file << "v " << model.vertices[i] << " " << model.vertices[i + 1] << " " << model.vertices[i + 2] << "\n";
        }

        // Guardar normales
        for (size_t i = 0; i < model.vertices.size(); i += 6) {
            file << "vn " << model.vertices[i + 3] << " " << model.vertices[i + 4] << " " << model.vertices[i + 5] << "\n";
        }

        // Guardar caras (índices)
        for (size_t i = 0; i < model.indices.size(); i += 3) {
            file << "f "
                << static_cast<size_t>(model.indices[i]) + 1 + vertexOffset << "//" << static_cast<size_t>(model.indices[i]) + 1 + vertexOffset << " "
                << static_cast<size_t>(model.indices[i + 1]) + 1 + vertexOffset << "//" << static_cast<size_t>(model.indices[i + 1]) + 1 + vertexOffset << " "
                << static_cast<size_t>(model.indices[i + 2]) + 1 + vertexOffset << "//" << static_cast<size_t>(model.indices[i + 2]) + 1 + vertexOffset << "\n";
        }

        vertexOffset += model.vertices.size() / 6; // Actualizar el desplazamiento para el siguiente modelo      
    }

    file.close();
    std::cout << "Escena guardada en " << filename << "\n";
}

// Cargar la escena de un archivo de texto
static void loadScene(const std::string& filename, std::vector<Model>& models) {
    clearScene(models); // Limpiar la escena actual
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo para cargar la escena.\n";
        return;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        std::cerr << "Error cargando el archivo OBJ: " << err << std::endl;
        return;
    }

    // Procesar cada forma (shape) como un modelo independiente
    for (const auto& shape : shapes) {
        Model newModel = processModel(attrib, { shape }, materials, false); // Procesar cada shape como un modelo
        models.push_back(newModel); // Añadir el modelo a la lista        
    }

    file.close();
    std::cout << "Escena cargada desde " << filename << "\n";
}

// Cambiar el color de fondo
static void changeBackgroundColor(GLFWwindow* window, glm::vec3& bgColor) {
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        bgColor = glm::vec3(0.0f, 0.0f, 0.0f); // Negro
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        bgColor = glm::vec3(0.46f, 0.46f, 0.46f); // Gris
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        bgColor = glm::vec3(0.46f, 0.0f, 0.0f); // Rojo
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        bgColor = glm::vec3(0.0f, 0.46f, 0.0f); // Verde
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        bgColor = glm::vec3(0.0f, 0.0f, 0.46f); // Azul
    }
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        bgColor = glm::vec3(0.46f, 0.46f, 0.0f); // Amarillo
    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
        bgColor = glm::vec3(0.46f, 0.0f, 0.46f); // Magenta
    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
        bgColor = glm::vec3(0.0f, 0.46f, 0.46f); // Cian
    }
}

// Dibujar las normales de cada vertice
static void drawNormals(const Model& model, GLuint shaderProgram, glm::vec3& normalsColor) {
    std::vector<float> normalLines;
    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        // Posición del vértice
        glm::vec3 position(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        // Dirección de la normal
        glm::vec3 normal(model.vertices[i + 3], model.vertices[i + 4], model.vertices[i + 5]);
        // Línea que parte del vértice en la dirección de la normal
        normalLines.push_back(position.x);
        normalLines.push_back(position.y);
        normalLines.push_back(position.z);
        normalLines.push_back(position.x + normal.x * 0.1f);
        normalLines.push_back(position.y + normal.y * 0.1f);
        normalLines.push_back(position.z + normal.z * 0.1f);
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(float), normalLines.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalsColor"), true);
    GLuint normalsColorLoc = glGetUniformLocation(shaderProgram, "normalsColor");
    glUniform3fv(normalsColorLoc, 1, glm::value_ptr(normalsColor));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(normalLines.size() / 3));
    glBindVertexArray(0);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalsColor"), false);
}

// Funcion para dibujar el bounding box del objeto
static void drawBoundingBox(const Model& model, GLuint shaderProgram, const glm::vec3& color) {
    // Inicializar los límites del bounding box
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    // Calcular los límites del bounding box
    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        glm::vec3 pos(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }

    // Esquinas del bounding box
    std::vector<glm::vec3> corners = {
        {minBounds.x, minBounds.y, minBounds.z}, {maxBounds.x, minBounds.y, minBounds.z},
        {maxBounds.x, minBounds.y, minBounds.z}, {maxBounds.x, maxBounds.y, minBounds.z},
        {maxBounds.x, maxBounds.y, minBounds.z}, {minBounds.x, maxBounds.y, minBounds.z},
        {minBounds.x, maxBounds.y, minBounds.z}, {minBounds.x, minBounds.y, minBounds.z},

        {minBounds.x, minBounds.y, maxBounds.z}, {maxBounds.x, minBounds.y, maxBounds.z},
        {maxBounds.x, minBounds.y, maxBounds.z}, {maxBounds.x, maxBounds.y, maxBounds.z},
        {maxBounds.x, maxBounds.y, maxBounds.z}, {minBounds.x, maxBounds.y, maxBounds.z},
        {minBounds.x, maxBounds.y, maxBounds.z}, {minBounds.x, minBounds.y, maxBounds.z},

        {minBounds.x, minBounds.y, minBounds.z}, {minBounds.x, minBounds.y, maxBounds.z},
        {maxBounds.x, minBounds.y, minBounds.z}, {maxBounds.x, minBounds.y, maxBounds.z},
        {maxBounds.x, maxBounds.y, minBounds.z}, {maxBounds.x, maxBounds.y, maxBounds.z},
        {minBounds.x, maxBounds.y, minBounds.z}, {minBounds.x, maxBounds.y, maxBounds.z},
    };

    // Crear y configurar VAO y VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(glm::vec3), corners.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Usar el shader program y configurar el color
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useBoundingBoxColor"), true);
    GLuint colorLoc = glGetUniformLocation(shaderProgram, "boundingBoxColor");
    glUniform3fv(colorLoc, 1, glm::value_ptr(color));
   
    // Evitar z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f); // Ajustar valores para desplazar el bounding box

    // Dibujar el bounding box
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(corners.size()));
    glBindVertexArray(0);

    // Limpiar recursos
    glDisable(GL_POLYGON_OFFSET_FILL); // Desactivar después de dibujar
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glUniform1i(glGetUniformLocation(shaderProgram, "useBoundingBoxColor"), false);
}

// Función para obtener el rayo desde la posición del ratón
static glm::vec3 getRayFromMouse(double mouseX, double mouseY, int windowWidth, int windowHeight, const glm::mat4& projection, const glm::mat4& view) {
    float x = (2.0f * static_cast<float>(mouseX)) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / windowHeight;
    float z = 1.0f; 

    // Coordenadas del rayo en el espacio del clip
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    // Convertir el rayo al espacio del ojo (vista)
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Convertir el rayo al espacio del mundo
    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
    rayWorld = glm::normalize(rayWorld);

    return rayWorld;
}

// Función para detectar colisión entre un rayo y un bounding box
static bool rayIntersectsBoundingBox(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& minBounds, const glm::vec3& maxBounds) {
    float tmin = (minBounds.x - rayOrigin.x) / rayDirection.x;
    float tmax = (maxBounds.x - rayOrigin.x) / rayDirection.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (minBounds.y - rayOrigin.y) / rayDirection.y;
    float tymax = (maxBounds.y - rayOrigin.y) / rayDirection.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (minBounds.z - rayOrigin.z) / rayDirection.z;
    float tzmax = (maxBounds.z - rayOrigin.z) / rayDirection.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    return true;
}

// Inicializar el mallado con divisiones y subdivisiones
void initGrid(float size, int divisions, int subDivisions) {
    // Generar vértices de la cuadrícula
    std::vector<float> vertices;
    float halfSize = size / 2.0f;
    float step = size / divisions;
    float subStep = step / subDivisions;

    // Líneas de las divisiones principales
    for (int i = 0; i <= divisions; ++i) {
        float coord = -halfSize + i * step;

        // Líneas paralelas al eje X
        vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(-halfSize);
        vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(halfSize);

        // Líneas paralelas al eje Z
        vertices.push_back(-halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
        vertices.push_back(halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
    }

    // Líneas de las subdivisiones
    for (int i = 0; i < divisions; ++i) {
        for (int j = 1; j < subDivisions; ++j) {
            float coord = -halfSize + i * step + j * subStep;

            // Líneas paralelas al eje X
            vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(-halfSize);
            vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(halfSize);

            // Líneas paralelas al eje Z
            vertices.push_back(-halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
            vertices.push_back(halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
        }
    }

    // Crear y configurar el VAO y VBO
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// Renderizar el mallado
void renderGrid(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color) {
    glUseProgram(shaderProgram);

    // Configurar uniforms
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint colorLoc = glGetUniformLocation(shaderProgram, "gridColor");
    GLuint isGridLoc = glGetUniformLocation(shaderProgram, "isGrid");

    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));
    if (isGridLoc != -1) glUniform1i(isGridLoc, 1);

    // Dibujar las divisiones principales (más gruesas)
    glLineWidth(2.5f); 
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, 4 * (20 + 1)); // Dibujar las divisiones principales
    glBindVertexArray(0);

    // Dibujar las subdivisiones (más delgadas)
    glLineWidth(0.5f); 
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 4 * (20 + 1), 4 * 20 * (5 - 1)); // Dibujar las subdivisiones
    glBindVertexArray(0);
    if (isGridLoc != -1) glUniform1i(isGridLoc, 0);
}

// Detectar colisón con el mallado
void checkCollisionWithPlatform(Model& model, float platformHeight) {
    // Calcular las 8 esquinas del AABB local
    glm::vec3 corners[8] = {
        model.localMinBounds,
        {model.localMinBounds.x, model.localMinBounds.y, model.localMaxBounds.z},
        {model.localMinBounds.x, model.localMaxBounds.y, model.localMinBounds.z},
        {model.localMinBounds.x, model.localMaxBounds.y, model.localMaxBounds.z},
        {model.localMaxBounds.x, model.localMinBounds.y, model.localMinBounds.z},
        {model.localMaxBounds.x, model.localMinBounds.y, model.localMaxBounds.z},
        {model.localMaxBounds.x, model.localMaxBounds.y, model.localMinBounds.z},
        model.localMaxBounds
    };

    // Transformar las esquinas y encontrar el mínimo Y
    float minY = FLT_MAX;
    for (const auto& corner : corners) {
        glm::vec4 transformed = model.transformMatrix * glm::vec4(corner, 1.0f);
        minY = glm::min(minY, transformed.y);
    }

    // Detectar colisión
    if (minY < platformHeight) {
        float offset = platformHeight - minY;
        model.translationMatrix = glm::translate(model.translationMatrix, glm::vec3(0.0f, offset, 0.0f));
        model.updateTransformMatrix();
    }
}

// Funcion principal para la ejecucion del programa
int main() {
    // Inicializar la ventana
    GLFWwindow* window = initWindow(800, 600, "Cargar OBJ");
    if (!window) return -1;

    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glm::vec3 bgColor(0.46f, 0.46f, 0.46f); // Color de fondo inicial

    std::vector<Model> models; // Vector para almacenar modelos cargados
    int selectedModelIndex = -1; // Índice del modelo seleccionado

    glm::vec3 eye(0.0f, 1.5f, 3.0f); // Posición inicial de la cámara
    glm::vec3 up(0.0f, 1.0f, 0.0f);  // Vector hacia arriba
    glm::vec3 target(0.0f, 0.0f, 0.0f); // Punto hacia el que mira la cámara

    // Configuración de cámara
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 viewMatrix = glm::lookAt(eye, target, up);

    glm::vec2 lastMousePos(0.0f, 0.0f); // Obtener la posición del cursor
    
    // Inicializar Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // Tema oscuro por defecto
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    float vertexSize = 5.0f; // Tamaño inicial de los vértices
    glm::vec3 vertexColor(0.0f, 0.0f, 0.0f); // Color inicial de los vértices 
    glm::vec3 wireframeColor(0.0f, 1.0f, 0.0f); // Color inicial del alambrado   
    glm::vec3 normalsColor(0.7f, 0.7f, 0.7f); // Color inicial de las normales
    glm::vec3 boundingBoxColor(1.0f, 1.0f, 0.0f); // Color inicial para el bounding box
    glm::vec3 newColor(1.0f, 1.0f, 1.0f); // Color inicial de los triangulos

    bool showVertices = false; // Control para mostrar/ocultar vértices
    bool showWireframe = false; // Control para mostrar/ocultar alambrado
    bool showNormals = false; // Control para mostrar/ocultar normales
    bool enableDepthTest = true; // Control para habilitar/deshabilitar Z-buffer
    bool enableBackFaceCulling = true; // Control para habilitar/deshabilitar back-face culling
    bool showFPS = true; // Control para mostrar/ocultar FPS promedio
    bool enableAntialiasing = true; // Control para habilitar/deshabilitar antialiasing
    bool showBoundingBox = false; // Control para mostrar bounding box
    bool enableColorChange = false; // Mostrar/ocultar relleno de triangulos

    // Variables para FPS promedio
    double lastTime = glfwGetTime(); // Tiempo del último frame
    int frameCount = 0; // Contador de frames
    double fps = 0.0f; // FPS promedio

    glEnable(GL_PROGRAM_POINT_SIZE);
    // Renderiza la cuadrícula antes de los modelos
    initGrid(20.0f, 20, 5);
    // Bucle principal
    while (!glfwWindowShouldClose(window)) {
        // Configurar Dear ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Interfaz de usuario
        ImGui::Begin("Opciones de Renderizado");    
        ImGui::Checkbox("Ver/Ocultar vértices", &showVertices); // Mostrar/ocultar vértices     
        if (showVertices) {
            ImGui::ColorEdit3("Color de vértices", (float*)&vertexColor); // Cambiar color de vértices
            ImGui::SliderFloat("Tamaño de vértices", &vertexSize, 1.0f, 10.0f); // Cambiar tamaño de vértices
        }
        ImGui::Checkbox("Ver/Ocultar alambrado", &showWireframe); // Mostrar/ocultar alambrado
        if (showWireframe) {
            ImGui::ColorEdit3("Color de alambrado", (float*)&wireframeColor); // Cambiar color del alambrado
        }
        if (!models.empty()) { // Verificar si hay modelos cargados                       
            ImGui::Checkbox("Ver/Ocultar relleno de triangulos", &enableColorChange); // Cambiar color del relleno
            if (enableColorChange) {               
                ImGui::ColorEdit3("Color del modelo", (float*)&newColor);               
                for (size_t i = 0; i < models.size(); ++i) { // Actualizar el color de todos los modelos excepto el seleccionado
                    if (static_cast<int>(i) != selectedModelIndex) {
                        models[i].color = newColor;
                    }
                }
            }            
        }
        ImGui::Checkbox("Ver/Ocultar normales", &showNormals); // Mostrar/ocultar normales
        if (showNormals) {
            ImGui::ColorEdit3("Color de las normales", (float*)&normalsColor); // Cambiar el color de las normales
        }
        ImGui::Checkbox("Habilitar Z-buffer", &enableDepthTest); // Habilitar/deshabilitar Z-buffer
        ImGui::Checkbox("Habilitar Back-face Culling", &enableBackFaceCulling); // Habilitar/deshabilitar back-face culling
        ImGui::Checkbox("Mostrar FPS", &showFPS); // Mostrar/ocultar FPS
        ImGui::Checkbox("Habilitar Antialiasing", &enableAntialiasing); // Habilitar/deshabilitar antialiasing
        ImGui::Checkbox("Mostrar Bounding Box", &showBoundingBox); // Habilitar bounding box
        if (showBoundingBox) {
            ImGui::ColorEdit3("Color del Bounding Box", (float*)&boundingBoxColor); // Cambiar color del bounding box
        }    
        ImGui::End();

        // Calcular FPS promedio
        frameCount++; // Aumentar el contador de frames
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 5.0f) { // Si han pasado más de 5 segundos
            fps = frameCount / (currentTime - lastTime); // FPS promedio
            frameCount = 0; // Resetear el contador de frames
            lastTime = currentTime; // Actualizar el tiempo del último frame
        }

        // Mostrar FPS si la opción está activada
        if (showFPS) {
            ImGui::Begin("FPS");
            ImGui::Text("%.2f", fps); // Mostrar FPS promedio
            ImGui::End();
        }

        // Configurar el Z-buffer
        if (enableDepthTest) {
            glEnable(GL_DEPTH_TEST); // Habilitar Z-buffer
            glDepthFunc(GL_LESS);
        } else {
            glDisable(GL_DEPTH_TEST); // Deshabilitar Z-buffer
        }
        
        // Confiugar el back-face culling
        if (enableBackFaceCulling) {
            glEnable(GL_CULL_FACE); // Habilitar Back-face Culling
            glCullFace(GL_BACK);    // Eliminar caras traseras
        } else {        
            glDisable(GL_CULL_FACE); // Deshabilitar Back-face Culling
        }

        // Configurar MSAA (antialiasing)
        if (enableAntialiasing) {
            glEnable(GL_MULTISAMPLE); // Habilitar MSAA
        } else {
            glDisable(GL_MULTISAMPLE); // Deshabilitar MSAA
        }
        
        // Actualizar el color de fondo y limpiar buffers
        changeBackgroundColor(window, bgColor);
        glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwPollEvents();
        // Lógica para cargar modelos
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            const char* fileFilter[1] = { "*.obj" };
            const char* filepath = tinyfd_openFileDialog("Selecciona archivo OBJ", "", 1, fileFilter, "Archivos OBJ", 0);

            if (filepath) {
                //models.clear(); // Limpiar la escena
                tinyobj::attrib_t attrib;
                std::vector<tinyobj::shape_t> shapes;
                std::vector<tinyobj::material_t> materials;
                std::string warn, err;

                std::filesystem::path objPath(filepath);
                std::string baseDir = objPath.parent_path().string() + "/";

                if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath, baseDir.c_str())) {                   
                    Model newModel = processModel(attrib, shapes, materials, true);
                    models.push_back(newModel); // Añadir modelo a la lista                    
                    std::filesystem::path path(filepath);
                    std::cout << "Modelo cargado: " << path.filename() << std::endl;
                }
                else {
                    std::cerr << "Error cargando el archivo OBJ: " << err << std::endl;
                }
            }
        }

        // Lógica para seleccionar un modelo al hacer clic
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            int winWidth, winHeight;
            glfwGetWindowSize(window, &winWidth, &winHeight);

            // Obtener el rayo desde el clic del ratón
            glm::vec3 rayOrigin = eye; // La posición de la cámara es el origen del rayo
            glm::vec3 rayDirection = getRayFromMouse(mouseX, mouseY, winWidth, winHeight, projection, viewMatrix);

            // Verificar si el rayo intersecta con algún objeto
            selectedModelIndex = -1;
            for (size_t i = 0; i < models.size(); ++i) {
                glm::vec3 minBounds(std::numeric_limits<float>::max());
                glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

                // Aplicar la matriz de transformación a los vértices originales
                for (size_t j = 0; j < models[i].originalVertices.size(); j += 6) {
                    glm::vec4 position(
                        models[i].originalVertices[j],
                        models[i].originalVertices[j + 1],
                        models[i].originalVertices[j + 2],
                        1.0f
                    );
                    // Aplicar la transformación acumulada
                    position = models[i].transformMatrix * position;

                    // Actualizar los límites del bounding box
                    minBounds = glm::min(minBounds, glm::vec3(position));
                    maxBounds = glm::max(maxBounds, glm::vec3(position));
                }

                // Verificar colisión entre el rayo y el bounding box
                if (rayIntersectsBoundingBox(rayOrigin, rayDirection, minBounds, maxBounds)) {
                    selectedModelIndex = static_cast<int>(i);
                    break;
                }
            }

            // Actualizar colores
            for (size_t i = 0; i < models.size(); ++i) {
                if (static_cast<int>(i) == selectedModelIndex) {
                    models[i].color = glm::vec3(1.0f, 0.0f, 0.0f); // Rojo al seleccionar
                }
                else if(!enableColorChange) {
                    models[i].color = models[i].originalColor; // Restaurar color original
                }
            }
        }

        // Manejo de rotación de escena o transformaciones de objeto
        if (selectedModelIndex == -1) {
            handleSceneAndCamera(window, viewMatrix, eye, target, up, lastMousePos, 0.4f, 0.005f);           
        } else {     
            handleModelTranslation(window, models[selectedModelIndex]);
            handleModelScaling(window, models[selectedModelIndex]);
            handleModelRotation(window, models[selectedModelIndex], lastMousePos, 0.3f);
        }
  
        // Verificar colisión con la plataforma para todos los modelos
        for (auto& model : models) {
            checkCollisionWithPlatform(model, -0.5f);
        }
        // Renderizar la cuadrícula
        renderGrid(shaderProgram, viewMatrix, projection, glm::vec3(0.7f, 0.7f, 0.7f));

        // Renderizar todos los modelos cargados
        for (size_t i = 0; i < models.size(); ++i) {
            if (showVertices) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); // Mostrar solo vertices
                GLuint pointSizeLoc = glGetUniformLocation(shaderProgram, "pointSize");
                glUniform1f(pointSizeLoc, vertexSize); // Pasar el tamaño de los puntos al shader
            } else if (showWireframe) {           
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Mostrar alambrado
                GLuint useWireframeColorLoc = glGetUniformLocation(shaderProgram, "useWireframeColor");
                glUniform1i(useWireframeColorLoc, 1);
                GLuint wireframeColorLoc = glGetUniformLocation(shaderProgram, "wireframeColor");
                glUniform3fv(wireframeColorLoc, 1, glm::value_ptr(wireframeColor));
            } else {          
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Renderizado normal
                GLuint useWireframeColorLoc = glGetUniformLocation(shaderProgram, "useWireframeColor");
                glUniform1i(useWireframeColorLoc, 0);
            }

            // Pasar el estado de vertexColor al shader
            GLuint useVertexColorLoc = glGetUniformLocation(shaderProgram, "useVertexColor");
            glUniform1i(useVertexColorLoc, showVertices ? 1 : 0);

            // Pasar el color de los vértices
            GLuint vertexColorLoc = glGetUniformLocation(shaderProgram, "vertexColor");
            glUniform3fv(vertexColorLoc, 1, glm::value_ptr(vertexColor));

            // Pasar el color del objeto
            GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(models[i].color));

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

            GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
            glUniform3fv(lightPosLoc, 1, glm::value_ptr(glm::vec3(1.2f, 1.0f, 2.0f)));
            GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
            glUniform3fv(viewPosLoc, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 3.0f)));
            GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
            glUniform3fv(lightColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
            
            models[i].draw(shaderProgram);
            // Dibujar las normales
            if (showNormals) {
                drawNormals(models[i], shaderProgram, normalsColor);
            }
            // Dibujar el bounding box 
            if (showBoundingBox && selectedModelIndex == i) {
                drawBoundingBox(models[selectedModelIndex], shaderProgram, boundingBoxColor);             
            }
            // Desactivar offset después de renderizar
            if (showWireframe) {
                glDisable(GL_POLYGON_OFFSET_LINE); 
            }
        }
        
        // Guardar, cargar o limpiar escena
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            for (auto& model : models) {
                model.applyTransformations();
            }
            saveScene("scene.txt", models);
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            selectedModelIndex = -1;
            loadScene("scene.txt", models);
        }
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            selectedModelIndex = -1;
            clearScene(models);
        }

        // Cerrar la ventana
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Renderizar interfaz
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Finalizar Dear ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Finalizar glfw
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
