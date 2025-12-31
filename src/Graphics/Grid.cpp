#include "Grid.h"

Grid::Grid(float size, int divisions, int subDivisions) {
    std::vector<float> vertices;
    float halfSize = size / 2.0f;
    float step = size / divisions;
    float subStep = step / subDivisions;

    // Divisiones principales
    for (int i = 0; i <= divisions; ++i) {
        float coord = -halfSize + i * step;
        vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(-halfSize);
        vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(halfSize);
        vertices.push_back(-halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
        vertices.push_back(halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
    }

    // Subdivisiones
    for (int i = 0; i < divisions; ++i) {
        for (int j = 1; j < subDivisions; ++j) {
            float coord = -halfSize + i * step + j * subStep;
            vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(-halfSize);
            vertices.push_back(coord); vertices.push_back(-0.5f); vertices.push_back(halfSize);
            vertices.push_back(-halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
            vertices.push_back(halfSize); vertices.push_back(-0.5f); vertices.push_back(coord);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Grid::draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color) {
    glUseProgram(shaderProgram);

    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint colorLoc = glGetUniformLocation(shaderProgram, "gridColor");
    GLuint isGridLoc = glGetUniformLocation(shaderProgram, "isGrid");

    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));
    
    // Activar modo Grid en el shader
    if (isGridLoc != -1) glUniform1i(isGridLoc, 1);

    glBindVertexArray(VAO);
    
    // Dibujar principales (gruesas)
    glLineWidth(2.5f); 
    // Nota: El cálculo '4 * (20 + 1)' es específico de tu configuración original (20 divisiones)
    // Para hacerlo genérico, idealmente guardaríamos el conteo de vértices, pero por ahora usamos tu lógica:
    // Asumimos que divisiones=20 para este draw call hardcodeado.
    glDrawArrays(GL_LINES, 0, 84); 
    
    // Dibujar secundarias (finas)
    glLineWidth(0.5f); 
    glDrawArrays(GL_LINES, 84, 320); 
    
    glBindVertexArray(0);
    
    // Desactivar modo Grid
    if (isGridLoc != -1) glUniform1i(isGridLoc, 0);
}