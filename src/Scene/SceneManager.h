#pragma once

#include "Model.h"
#include "../Core/Camera.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>

struct GLFWwindow;

class SceneManager {
public:
    // Gestión de Archivos
    static void Save(const std::string& filename, const std::vector<Model>& models);
    static void Load(const std::string& filename, std::vector<Model>& models);
    static void Clear(std::vector<Model>& models);

    // Físicas y Colisiones
    static void CheckCollisionWithPlatform(Model& model, float platformHeight);
    
    // Raycasting (Mouse picking)
    static glm::vec3 GetRayFromMouse(double mouseX, double mouseY, int windowWidth, int windowHeight, const glm::mat4& projection, const glm::mat4& view);
    static bool RayIntersectsBoundingBox(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& minBounds, const glm::vec3& maxBounds);

    // Importar un modelo usando diálogo de archivo
    static void ImportModel(std::vector<Model>& models);

    // Intentar seleccionar un modelo con el mouse
    static int PickModel(GLFWwindow* window, const std::vector<Model>& models, const Camera& camera);
};