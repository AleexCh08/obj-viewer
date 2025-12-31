#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class Grid {
public:
    GLuint VAO, VBO;
    
    // Constructor (reemplaza a initGrid)
    Grid(float size, int divisions, int subDivisions);
    
    // Función de dibujado (reemplaza a renderGrid)
    void draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color);
};