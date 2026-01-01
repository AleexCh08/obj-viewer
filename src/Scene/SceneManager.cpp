#include "SceneManager.h"
#include "../Graphics/PickingShader.h"
#include "../Graphics/Shader.h"
#include <filesystem>
#include <sstream>
#include <GLFW/glfw3.h>
#include <Core/Camera.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void SceneManager::Clear(std::vector<Model>& models) {
    for (auto& model : models) {
        glDeleteBuffers(1, &model.VBO);
        glDeleteBuffers(1, &model.EBO);
        glDeleteVertexArrays(1, &model.VAO);
    }
    models.clear();
    std::cout << "Escena eliminada correctamente.\n";
}

void SceneManager::Save(const std::string& filename, const std::vector<Model>& models) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de escena.\n";
        return;
    }

    for (const auto& model : models) {
        if (model.path.empty()) continue;

        file << model.path << " "
             << model.position.x << " " << model.position.y << " " << model.position.z << " "
             << model.rotation.x << " " << model.rotation.y << " " << model.rotation.z << " "
             << model.scale.x << " " << model.scale.y << " " << model.scale.z << " "
             << model.color.r << " " << model.color.g << " " << model.color.b << "\n";
    }
    
    file.close();
    std::cout << "Escena guardada optimizada en " << filename << "\n";
}

void SceneManager::Load(const std::string& filename, std::vector<Model>& models) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se encuentra el archivo de escena.\n";
        return;
    }

    Clear(models); 

    std::string path;
    glm::vec3 pos, rot, scl, col;

    while (file >> path >> pos.x >> pos.y >> pos.z >> rot.x >> rot.y >> rot.z >> scl.x >> scl.y >> scl.z >> col.x >> col.y >> col.z) {
        
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        
        std::string baseDir = std::filesystem::path(path).parent_path().string() + "/";

        if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
            Model newModel = Model::Process(attrib, shapes, materials, true);
            
            newModel.path = path;
            newModel.position = pos;
            newModel.rotation = rot;
            newModel.scale = scl;
            newModel.color = col;
            
            newModel.updateTransformMatrix(); 

            models.push_back(newModel);
        } else {
            std::cerr << "No se pudo recargar el modelo: " << path << "\n";
        }
    }
    
    file.close();
    std::cout << "Escena cargada y restaurada desde " << filename << "\n";
}

void SceneManager::CheckCollisionWithPlatform(Model& model, float platformHeight) {
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

    float minY = FLT_MAX;
    for (const auto& corner : corners) {
        glm::vec4 transformed = model.transformMatrix * glm::vec4(corner, 1.0f);
        minY = glm::min(minY, transformed.y);
    }

    if (minY < platformHeight) {
        float offset = platformHeight - minY;
        model.position.y += offset;
        model.updateTransformMatrix();
    }
}

glm::vec3 SceneManager::GetRayFromMouse(double mouseX, double mouseY, int windowWidth, int windowHeight, const glm::mat4& projection, const glm::mat4& view) {
    float x = (2.0f * static_cast<float>(mouseX)) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / windowHeight;
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
    return glm::normalize(rayWorld);
}

bool SceneManager::RayIntersectsBoundingBox(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& minBounds, const glm::vec3& maxBounds) {
    float tmin = (minBounds.x - rayOrigin.x) / rayDirection.x;
    float tmax = (maxBounds.x - rayOrigin.x) / rayDirection.x;
    if (tmin > tmax) std::swap(tmin, tmax);
    float tymin = (minBounds.y - rayOrigin.y) / rayDirection.y;
    float tymax = (maxBounds.y - rayOrigin.y) / rayDirection.y;
    if (tymin > tymax) std::swap(tymin, tymax);
    if ((tmin > tymax) || (tymin > tmax)) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    float tzmin = (minBounds.z - rayOrigin.z) / rayDirection.z;
    float tzmax = (maxBounds.z - rayOrigin.z) / rayDirection.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);
    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    return true;
}

void SceneManager::ImportModel(std::vector<Model>& models) {
    const char* fileFilter[1] = { "*.obj" };
    const char* filepath = tinyfd_openFileDialog("Selecciona archivo OBJ", "", 1, fileFilter, "Archivos OBJ", 0);

    if (filepath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::filesystem::path objPath(filepath);
        std::string baseDir = objPath.parent_path().string() + "/";

        if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath, baseDir.c_str())) {                   
            Model newModel = Model::Process(attrib, shapes, materials, true);
            newModel.path = std::string(filepath);
            models.push_back(newModel);                    
            std::cout << "Modelo cargado: " << objPath.filename() << std::endl;
        } else {
            std::cerr << "Error cargando el archivo OBJ: " << err << std::endl;
        }
    }
}

int SceneManager::PickModel(GLFWwindow* window, const std::vector<Model>& models, const Camera& camera) {
    static Shader pickingShader(pickingVertexShaderSource, pickingFragmentShaderSource);
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pickingShader.use();
    pickingShader.setMat4("view", camera.getViewMatrix());
    pickingShader.setMat4("projection", camera.getProjectionMatrix());

    for (int i = 0; i < models.size(); ++i) {
        float r = ((i + 1) & 0xFF) / 255.0f;
        float g = (((i + 1) >> 8) & 0xFF) / 255.0f;
        float b = (((i + 1) >> 16) & 0xFF) / 255.0f;
        
        pickingShader.setVec3("pickingColor", glm::vec3(r, g, b));
        
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, models[i].position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, models[i].scale);

        pickingShader.setMat4("model", modelMatrix);
        
        glBindVertexArray(models[i].VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(models[i].indices.size()), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    float dpiScale = (float)width / (float)windowWidth; 
    
    int pixelX = static_cast<int>(xpos * dpiScale);
    int pixelY = height - static_cast<int>(ypos * dpiScale); 

    unsigned char data[4];
    glReadPixels(pixelX, pixelY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

    int pickedID = data[0] + (data[1] * 256) + (data[2] * 256 * 256);

    if (pickedID > 0 && (pickedID - 1) < models.size()) {
        return pickedID - 1;
    }

    return -1; 
}

void SceneManager::DeleteSelectedModel(std::vector<Model>& models, int& selectedIndex) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(models.size())) return;

    Model& model = models[selectedIndex];
    glDeleteBuffers(1, &model.VBO);
    glDeleteBuffers(1, &model.EBO);
    glDeleteVertexArrays(1, &model.VAO);

    models.erase(models.begin() + selectedIndex);

    selectedIndex = -1;
    
    std::cout << "Modelo eliminado correctamente." << std::endl;
}

void SceneManager::AddLight(std::vector<Model>& models) {
    Model lightModel;
    lightModel.isLight = true;
    lightModel.color = glm::vec3(1.0f, 1.0f, 1.0f);
    lightModel.originalColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightModel.position = glm::vec3(2.0f, 2.0f, 2.0f); 
    lightModel.scale = glm::vec3(0.1f); 
    lightModel.path = "Internal:LightSphere";

    const int X_SEGMENTS = 30;
    const int Y_SEGMENTS = 30;

    for (int y = 0; y <= Y_SEGMENTS; ++y) {
        for (int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            lightModel.vertices.push_back(xPos); lightModel.vertices.push_back(yPos); lightModel.vertices.push_back(zPos);
            lightModel.vertices.push_back(xPos); lightModel.vertices.push_back(yPos); lightModel.vertices.push_back(zPos);
        }
    }

    for (int y = 0; y < Y_SEGMENTS; ++y) {
        for (int x = 0; x < X_SEGMENTS; ++x) {
            lightModel.indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            lightModel.indices.push_back(y * (X_SEGMENTS + 1) + x);
            lightModel.indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            lightModel.indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            lightModel.indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            lightModel.indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }

    glGenVertexArrays(1, &lightModel.VAO);
    glGenBuffers(1, &lightModel.VBO);
    glGenBuffers(1, &lightModel.EBO);

    glBindVertexArray(lightModel.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightModel.VBO);
    glBufferData(GL_ARRAY_BUFFER, lightModel.vertices.size() * sizeof(float), lightModel.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightModel.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, lightModel.indices.size() * sizeof(unsigned int), lightModel.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    lightModel.originalVertices = lightModel.vertices;
    models.push_back(lightModel);
    std::cout << "Fuente de luz agregada a la escena.\n";
}