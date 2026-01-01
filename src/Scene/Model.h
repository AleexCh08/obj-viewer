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
    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<float> originalVertices;
    std::vector<unsigned int> indices;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); 
    glm::vec3 scale    = glm::vec3(1.0f);
    glm::mat4 transformMatrix = glm::mat4(1.0f);

    glm::vec3 color;
    glm::vec3 originalColor;
    glm::vec3 localMinBounds;
    glm::vec3 localMaxBounds;

    Model();
    std::string path;
    bool isLight = false;

    unsigned int textureID = 0;
    bool hasTexture = false;

    GLuint debugNormalsVAO = 0, debugNormalsVBO = 0;
    GLuint debugBoxVAO = 0, debugBoxVBO = 0;

    void setupModel();
    void updateTransformMatrix();
    void applyTransformations();
    void draw(GLuint shaderProgram) const;

    static Model Process(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, const std::string& baseDir, bool normalize);
    static void Normalize(Model& model);

    void drawDebugNormals(GLuint shaderProgram, const glm::vec3& color);
    void drawDebugBoundingBox(GLuint shaderProgram, const glm::vec3& color);
};