// Librerias necesarias 
#include "Graphics/Shader.h"
#include "Core/Camera.h"
#include "Scene/Model.h"
#include "Graphics/Grid.h"
#include "Scene/SceneManager.h"
#include "Core/Window.h"
#include "Core/InputController.h"
#include "UI/UIManager.h"

// Librerias estandar
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

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


// Funcion principal para la ejecucion del programa
int main() {
    // Inicializar la ventana
    GLFWwindow* window = Window::Init(800, 600, "ViewerOBJ Pro");
    if (!window) return -1;

    Shader shader(vertexShaderSource, fragmentShaderSource);
    GLuint shaderProgram = shader.ID;
    Camera camera(800, 600, glm::vec3(0.0f, 1.5f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    UIState ui;


    glm::vec3 bgColor(0.46f, 0.46f, 0.46f); // Color de fondo inicial

    std::vector<Model> models; // Vector para almacenar modelos cargados
    int selectedModelIndex = -1; // Índice del modelo seleccionado

    glm::vec2 lastMousePos(0.0f, 0.0f); // Obtener la posición del cursor
    
    // Inicializar Dear ImGui
    UIManager::Init(window);

    // Variables para FPS promedio
    double lastTime = glfwGetTime(); // Tiempo del último frame
    int frameCount = 0; // Contador de frames
    double fps = 0.0f; // FPS promedio

    glEnable(GL_PROGRAM_POINT_SIZE);
    // Renderiza la cuadrícula antes de los modelos
    Grid grid(20.0f, 20, 5);
    // Bucle principal
    while (!glfwWindowShouldClose(window)) {
        // Configurar Dear ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Interfaz de usuario
        ImGui::Begin("Opciones de Renderizado");    
        ImGui::Checkbox("Ver/Ocultar vértices", &ui.showVertices); // Mostrar/ocultar vértices     
        if (ui.showVertices) {
            ImGui::ColorEdit3("Color de vértices", (float*)&ui.vertexColor); // Cambiar color de vértices
            ImGui::SliderFloat("Tamaño de vértices", &ui.vertexSize, 1.0f, 10.0f); // Cambiar tamaño de vértices
        }
        ImGui::Checkbox("Ver/Ocultar alambrado", &ui.showWireframe); // Mostrar/ocultar alambrado
        if (ui.showWireframe) {
            ImGui::ColorEdit3("Color de alambrado", (float*)&ui.wireframeColor); // Cambiar color del alambrado
        }
        if (!models.empty()) { // Verificar si hay modelos cargados                       
            ImGui::Checkbox("Ver/Ocultar relleno de triangulos", &ui.enableColorChange); // Cambiar color del relleno
            if (ui.enableColorChange) {               
                ImGui::ColorEdit3("Color del modelo", (float*)&ui.newColor);               
                for (size_t i = 0; i < models.size(); ++i) { // Actualizar el color de todos los modelos excepto el seleccionado
                    if (static_cast<int>(i) != selectedModelIndex) {
                        models[i].color = ui.newColor;
                    }
                }
            }            
        }
        ImGui::Checkbox("Ver/Ocultar normales", &ui.showNormals); // Mostrar/ocultar normales
        if (ui.showNormals) {
            ImGui::ColorEdit3("Color de las normales", (float*)&ui.normalsColor); // Cambiar el color de las normales
        }
        ImGui::Checkbox("Habilitar Z-buffer", &ui.enableDepthTest); // Habilitar/deshabilitar Z-buffer
        ImGui::Checkbox("Habilitar Back-face Culling", &ui.enableBackFaceCulling); // Habilitar/deshabilitar back-face culling
        ImGui::Checkbox("Mostrar FPS", &ui.showFPS); // Mostrar/ocultar FPS
        ImGui::Checkbox("Habilitar Antialiasing", &ui.enableAntialiasing); // Habilitar/deshabilitar antialiasing
        ImGui::Checkbox("Mostrar Bounding Box", &ui.showBoundingBox); // Habilitar bounding box
        if (ui.showBoundingBox) {
            ImGui::ColorEdit3("Color del Bounding Box", (float*)&ui.boundingBoxColor); // Cambiar color del bounding box
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
        if (ui.showFPS) {
            ImGui::Begin("FPS");
            ImGui::Text("%.2f", fps); // Mostrar FPS promedio
            ImGui::End();
        }

        // Configurar el Z-buffer
        if (ui.enableDepthTest) {
            glEnable(GL_DEPTH_TEST); // Habilitar Z-buffer
            glDepthFunc(GL_LESS);
        } else {
            glDisable(GL_DEPTH_TEST); // Deshabilitar Z-buffer
        }
        
        // Confiugar el back-face culling
        if (ui.enableBackFaceCulling) {
            glEnable(GL_CULL_FACE); // Habilitar Back-face Culling
            glCullFace(GL_BACK);    // Eliminar caras traseras
        } else {        
            glDisable(GL_CULL_FACE); // Deshabilitar Back-face Culling
        }

        // Configurar MSAA (antialiasing)
        if (ui.enableAntialiasing) {
            glEnable(GL_MULTISAMPLE); // Habilitar MSAA
        } else {
            glDisable(GL_MULTISAMPLE); // Deshabilitar MSAA
        }
        
        // Actualizar el color de fondo y limpiar buffers
        InputController::changeBackgroundColor(window, bgColor);
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
                    Model newModel = Model::Process(attrib, shapes, materials, true);
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
            glm::vec3 rayOrigin = camera.eye; // La posición de la cámara es el origen del rayo
            glm::vec3 rayDirection = SceneManager::GetRayFromMouse(mouseX, mouseY, winWidth, winHeight, camera.getProjectionMatrix(), camera.getViewMatrix());

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
                if (SceneManager::RayIntersectsBoundingBox(rayOrigin, rayDirection, minBounds, maxBounds)) {
                    selectedModelIndex = static_cast<int>(i);
                    break;
                }
            }

            // Actualizar colores
            for (size_t i = 0; i < models.size(); ++i) {
                if (static_cast<int>(i) == selectedModelIndex) {
                    models[i].color = glm::vec3(1.0f, 0.0f, 0.0f); // Rojo al seleccionar
                }
                else if(!ui.enableColorChange) {
                    models[i].color = models[i].originalColor; // Restaurar color original
                }
            }
        }

        // Manejo de rotación de escena o transformaciones de objeto
        if (selectedModelIndex == -1) {
            camera.handleInput(window);           
        } else {     
            InputController::handleModelTranslation(window, models[selectedModelIndex]);
            InputController::handleModelScaling(window, models[selectedModelIndex]);
            InputController::handleModelRotation(window, models[selectedModelIndex], lastMousePos, 0.3f);
        }
  
        // Verificar colisión con la plataforma para todos los modelos
        for (auto& model : models) {
            SceneManager::CheckCollisionWithPlatform(model, -0.5f);
        }
        // Renderizar la cuadrícula
        grid.draw(shaderProgram, camera.getViewMatrix(), camera.getProjectionMatrix(), glm::vec3(0.7f, 0.7f, 0.7f));

        // Renderizar todos los modelos cargados
        // Renderizar todos los modelos cargados
        for (size_t i = 0; i < models.size(); ++i) {
            if (ui.showVertices) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                glUniform1f(glGetUniformLocation(shaderProgram, "pointSize"), ui.vertexSize);
            } else if (ui.showWireframe) {           
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glUniform1i(glGetUniformLocation(shaderProgram, "useWireframeColor"), 1);
                glUniform3fv(glGetUniformLocation(shaderProgram, "wireframeColor"), 1, glm::value_ptr(ui.wireframeColor));
            } else {          
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUniform1i(glGetUniformLocation(shaderProgram, "useWireframeColor"), 0);
            }

            glUniform1i(glGetUniformLocation(shaderProgram, "useVertexColor"), ui.showVertices ? 1 : 0);
            glUniform3fv(glGetUniformLocation(shaderProgram, "vertexColor"), 1, glm::value_ptr(ui.vertexColor));
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(models[i].color));

            // Matrices
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));
            
            // Luces (esto podría ir fuera del bucle para optimizar, pero mantenemos estructura)
            glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(1.2f, 1.0f, 2.0f)));
            glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 3.0f)));
            glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
            
            models[i].draw(shaderProgram);

            if (ui.showNormals) {
                models[i].drawDebugNormals(shaderProgram, ui.normalsColor);
            }
            
            if (ui.showBoundingBox && selectedModelIndex == (int)i) {
                models[selectedModelIndex].drawDebugBoundingBox(shaderProgram, ui.boundingBoxColor);             
            }

            if (ui.showWireframe) {
                glDisable(GL_POLYGON_OFFSET_LINE); 
            }
        }
        
        // Guardar, cargar o limpiar escena
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            for (auto& model : models) {
                model.applyTransformations();
            }
            SceneManager::Save("scene.txt", models);
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            selectedModelIndex = -1;
            SceneManager::Load("scene.txt", models);
        }
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            selectedModelIndex = -1;
            SceneManager::Clear(models);
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
