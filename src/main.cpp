// Librerias necesarias 
#include "Graphics/Shader.h"
#include "Core/Camera.h"
#include "Scene/Model.h"
#include "Graphics/Grid.h"
#include "Scene/SceneManager.h"
#include "Core/Window.h"
#include "Core/InputController.h"
#include "UI/UIManager.h"
#include "Graphics/Shaders.h"

// Librerias estandar
#include <iostream>
#include <vector>
#include <string>

// Funcion principal para la ejecucion del programa
int main() {
    GLFWwindow* window = Window::Init(800, 600, "ViewerOBJ Pro");
    if (!window) return -1;

    Shader shader(vertexShaderSource, fragmentShaderSource);
    GLuint shaderProgram = shader.ID;
    Camera camera(800, 600, glm::vec3(0.0f, 1.5f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    UIState ui;
    UIManager::Init(window); 
    Grid grid(20.0f, 20, 5);

    glm::vec3 bgColor(0.46f, 0.46f, 0.46f); // Color de fondo inicial
    std::vector<Model> models; // Vector para almacenar modelos cargados
    int selectedModelIndex = -1; // Índice del modelo seleccionado
    glm::vec2 lastMousePos(0.0f, 0.0f); // Obtener la posición del cursor
      
    // Variables para FPS promedio
    double lastTime = glfwGetTime(); // Tiempo del último frame
    int frameCount = 0; // Contador de frames
    double fps = 0.0f; // FPS promedio

    glEnable(GL_PROGRAM_POINT_SIZE);
    
    // Bucle principal
    while (!glfwWindowShouldClose(window)) {
        // Calcular FPS promedio
        frameCount++; 
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 5.0f) { 
            fps = frameCount / (currentTime - lastTime); 
            frameCount = 0; 
            lastTime = currentTime; 
        }

        // Configurar el Z-buffer
        if (ui.enableDepthTest) {
            glEnable(GL_DEPTH_TEST); 
            glDepthFunc(GL_LESS);
        } else {
            glDisable(GL_DEPTH_TEST); 
        }
        
        // Confiugar el back-face culling
        if (ui.enableBackFaceCulling) {
            glEnable(GL_CULL_FACE); 
            glCullFace(GL_BACK);    
        } else {        
            glDisable(GL_CULL_FACE); 
        }

        // Configurar MSAA
        if (ui.enableAntialiasing) {
            glEnable(GL_MULTISAMPLE); 
        } else {
            glDisable(GL_MULTISAMPLE); 
        }
        
        // Actualizar el color de fondo y limpiar buffers
        InputController::changeBackgroundColor(window, bgColor);
        glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        glfwPollEvents();

        // Lógica para seleccionar un modelo al hacer clic
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
            int pickedIndex = SceneManager::PickModel(window, models, camera);
            
            if (pickedIndex != -1) {
                selectedModelIndex = pickedIndex;
            } else {
                selectedModelIndex = -1; 
            }

            for (size_t i = 0; i < models.size(); ++i) {
                if ((int)i == selectedModelIndex) {
                    models[i].color = glm::vec3(1.0f, 0.0f, 0.0f);
                } else if(!ui.enableColorChange) {
                    models[i].color = models[i].originalColor;
                }
            }
        }

        // Manejo de cámara y transformación
        if (selectedModelIndex == -1) {
            camera.handleInput(window);           
        } else {     
            InputController::handleModelTranslation(window, models[selectedModelIndex]);
            InputController::handleModelScaling(window, models[selectedModelIndex]);
            InputController::handleModelRotation(window, models[selectedModelIndex], lastMousePos, 0.3f);
        }
  
        // Verificar colisiones
        for (auto& model : models) {
            SceneManager::CheckCollisionWithPlatform(model, -0.5f);
        }

        // Renderizar la cuadrícula
        grid.draw(shaderProgram, camera.getViewMatrix(), camera.getProjectionMatrix(), glm::vec3(0.7f, 0.7f, 0.7f));

        // Renderizar todos los modelos
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

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));
            
            glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(1.2f, 1.0f, 2.0f)));
            glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 3.0f)));
            glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
            
            models[i].draw(shaderProgram);

            if (ui.showNormals) models[i].drawDebugNormals(shaderProgram, ui.normalsColor);
            if (ui.showBoundingBox && selectedModelIndex == (int)i) models[selectedModelIndex].drawDebugBoundingBox(shaderProgram, ui.boundingBoxColor);
            if (ui.showWireframe) glDisable(GL_POLYGON_OFFSET_LINE); 
        }

        // Atajos del teclado  
        // Detectar si la tecla Control está presionada (Izquierda o Derecha)
        bool ctrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
                           glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

        // IMPORTAR MODELO (Ctrl + O)
        static bool isImporting = false; 
        if (ctrlPressed && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            if (!isImporting) {
                SceneManager::ImportModel(models);
                isImporting = true; 
            }
        } else {
            isImporting = false;
        }

        // GUARDAR ESCENA (Ctrl + S)
        static bool isSaving = false;
        if (ctrlPressed && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            if (!isSaving) {
                for (auto& model : models) model.applyTransformations();
                SceneManager::Save("scene.txt", models);
                isSaving = true;
            }
        } else {
            isSaving = false;
        }

        // CARGAR ESCENA (Ctrl + L)
        static bool isLoading = false;
        if (ctrlPressed && glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
            if (!isLoading) {
                selectedModelIndex = -1;
                SceneManager::Load("scene.txt", models);
                isLoading = true;
            }
        } else {
            isLoading = false;
        }

        // LIMPIAR ESCENA (Ctrl + N)
        static bool isClearing = false;
        if (ctrlPressed && glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            if (!isClearing) {
                selectedModelIndex = -1;
                SceneManager::Clear(models);
                isClearing = true;
            }
        } else {
            isClearing = false;
        }

        // SALIR (Esc)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        UIManager::Render(window, ui, models, selectedModelIndex, fps);
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
