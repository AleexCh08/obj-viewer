#include "SceneManager.h"
#include <filesystem>
#include <GLFW/glfw3.h>
#include <Core/Camera.h>

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
        std::cerr << "Error: No se pudo abrir el archivo para guardar la escena.\n";
        return;
    }

    file << "# Archivo de escena en formato .OBJ\n";
    file << "# Formato: vertices(v) normales(vn) caras(f)\n";

    size_t vertexOffset = 0;

    for (const auto& model : models) {
        file << "o " << "\n";
        for (size_t i = 0; i < model.vertices.size(); i += 6) {
            file << "v " << model.vertices[i] << " " << model.vertices[i + 1] << " " << model.vertices[i + 2] << "\n";
        }
        for (size_t i = 0; i < model.vertices.size(); i += 6) {
            file << "vn " << model.vertices[i + 3] << " " << model.vertices[i + 4] << " " << model.vertices[i + 5] << "\n";
        }
        for (size_t i = 0; i < model.indices.size(); i += 3) {
            file << "f "
                << static_cast<size_t>(model.indices[i]) + 1 + vertexOffset << "//" << static_cast<size_t>(model.indices[i]) + 1 + vertexOffset << " "
                << static_cast<size_t>(model.indices[i + 1]) + 1 + vertexOffset << "//" << static_cast<size_t>(model.indices[i + 1]) + 1 + vertexOffset << " "
                << static_cast<size_t>(model.indices[i + 2]) + 1 + vertexOffset << "//" << static_cast<size_t>(model.indices[i + 2]) + 1 + vertexOffset << "\n";
        }
        vertexOffset += model.vertices.size() / 6;
    }
    file.close();
    std::cout << "Escena guardada en " << filename << "\n";
}

void SceneManager::Load(const std::string& filename, std::vector<Model>& models) {
    Clear(models);
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        std::cerr << "Error cargando el archivo OBJ: " << err << std::endl;
        return;
    }

    for (const auto& shape : shapes) {
        // Usamos Model::Process que creamos en el paso anterior
        Model newModel = Model::Process(attrib, { shape }, materials, false);
        models.push_back(newModel);     
    }
    std::cout << "Escena cargada desde " << filename << "\n";
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
        model.translationMatrix = glm::translate(model.translationMatrix, glm::vec3(0.0f, offset, 0.0f));
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
            // Nota: Aquí usamos Model::Process tal cual lo definimos antes
            Model newModel = Model::Process(attrib, shapes, materials, true);
            models.push_back(newModel);                    
            std::cout << "Modelo cargado: " << objPath.filename() << std::endl;
        } else {
            std::cerr << "Error cargando el archivo OBJ: " << err << std::endl;
        }
    }
}

int SceneManager::PickModel(GLFWwindow* window, const std::vector<Model>& models, const Camera& camera) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);

    // Obtener el rayo
    glm::vec3 rayOrigin = camera.eye;
    glm::vec3 rayDirection = GetRayFromMouse(mouseX, mouseY, winWidth, winHeight, camera.getProjectionMatrix(), camera.getViewMatrix());

    int selectedIndex = -1;

    // Verificar intersección con cada modelo
    for (size_t i = 0; i < models.size(); ++i) {
        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

        // Calcular el Bounding Box TRANSFORMADO (Esto es pesado, mejor que esté aquí que en el main)
        for (size_t j = 0; j < models[i].originalVertices.size(); j += 6) {
            glm::vec4 position(
                models[i].originalVertices[j],
                models[i].originalVertices[j + 1],
                models[i].originalVertices[j + 2],
                1.0f
            );
            position = models[i].transformMatrix * position;
            minBounds = glm::min(minBounds, glm::vec3(position));
            maxBounds = glm::max(maxBounds, glm::vec3(position));
        }

        if (RayIntersectsBoundingBox(rayOrigin, rayDirection, minBounds, maxBounds)) {
            selectedIndex = static_cast<int>(i);
            break; // Seleccionamos el primero que encontremos
        }
    }
    return selectedIndex;
}