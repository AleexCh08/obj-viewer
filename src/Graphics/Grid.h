#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class Grid {
public:
    GLuint VAO, VBO;
    
    Grid(float size, int divisions, int subDivisions);
    void draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color);
};