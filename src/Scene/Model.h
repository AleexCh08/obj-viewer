#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <limits>
#include <algorithm> // Para std::max

#include <tinyfiledialogs.h> 
#include "tiny_obj_loader.h" 

class Model {
public:
    // --- Datos del Mesh ---
    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<float> originalVertices;
    std::vector<unsigned int> indices;

    // --- Transformaciones ---
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); 
    glm::vec3 scale    = glm::vec3(1.0f);
    glm::mat4 transformMatrix = glm::mat4(1.0f);

    // --- Propiedades ---
    glm::vec3 color;
    glm::vec3 originalColor;
    glm::vec3 localMinBounds;
    glm::vec3 localMaxBounds;

    // --- Constructor y Destructor ---
    Model();
    // Opcional: Destructor para limpiar buffers automáticamente al cerrar
    // ~Model(); 

    // --- Métodos del Objeto ---
    void setupModel();
    void updateTransformMatrix();
    void applyTransformations();
    void draw(GLuint shaderProgram) const;

    // --- Funciones Estáticas de Ayuda (Antes estaban sueltas en main) ---
    // "Factory" para procesar datos de TinyOBJ y crear un Model
    static Model Process(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, bool normalize);
    
    // Normalizar tamaño
    static void Normalize(Model& model);

    // --- Debug Rendering ---
    void drawDebugNormals(GLuint shaderProgram, const glm::vec3& color) const;
    void drawDebugBoundingBox(GLuint shaderProgram, const glm::vec3& color) const;
};