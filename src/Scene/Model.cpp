#include "Model.h"

Model::Model() : VAO(0), VBO(0), EBO(0), 
                 color(0.7f, 0.7f, 0.7f), originalColor(0.7f, 0.7f, 0.7f), 
                 localMinBounds(0.0f), localMaxBounds(0.0f) {}

void Model::setupModel() {
    originalVertices = vertices;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normales
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Model::updateTransformMatrix() {
    glm::mat4 mat = glm::mat4(1.0f);
    
    // 1. Traslación
    mat = glm::translate(mat, position);
    
    // 2. Rotación (Euler Angles: Y -> X -> Z)
    mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // 3. Escala
    mat = glm::scale(mat, scale);
    
    this->transformMatrix = mat;
}

void Model::applyTransformations() {    
    // Al guardar, "quemamos" la transformación en los vértices
    for (size_t i = 0; i < originalVertices.size(); i += 6) {
        glm::vec4 position(originalVertices[i], originalVertices[i+1], originalVertices[i+2], 1.0f);
        position = transformMatrix * position;
        
        // Actualizamos vértices
        vertices[i] = position.x;
        vertices[i + 1] = position.y;
        vertices[i + 2] = position.z;
        // Nota: Las normales también deberían rotarse, pero para simplificar lo dejamos así por ahora
    }
    // Resetear transformaciones tras aplicar
    position = glm::vec3(0.0f);
    rotation = glm::vec3(0.0f);
    scale = glm::vec3(1.0f);
    updateTransformMatrix();
}

void Model::draw(GLuint shaderProgram) const {
    glUseProgram(shaderProgram);

    GLuint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUniform3fv(colorLoc, 1, &color[0]);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transformMatrix));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// --- Implementación de estáticos ---

void Model::Normalize(Model& model) {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        glm::vec3 pos(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }

    glm::vec3 size = maxBounds - minBounds;
    // Usamos std::max anidado para evitar problemas con listas de inicialización
    float scale = 1.0f / std::max(size.x, std::max(size.y, size.z)); 
    glm::vec3 center = (minBounds + maxBounds) * 0.5f;

    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        model.vertices[i + 0] = (model.vertices[i + 0] - center.x) * scale;
        model.vertices[i + 1] = (model.vertices[i + 1] - center.y) * scale;
        model.vertices[i + 2] = (model.vertices[i + 2] - center.z) * scale;
    }
}

Model Model::Process(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, bool normalize) {
    Model model;
    
    // Recorremos todas las formas y caras
    for (const auto& shape : shapes) {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            // Obtener índices de los 3 vértices del triángulo
            tinyobj::index_t idx0 = shape.mesh.indices[i + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[i + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[i + 2];

            // Obtener posiciones
            glm::vec3 v0(attrib.vertices[3 * idx0.vertex_index + 0], attrib.vertices[3 * idx0.vertex_index + 1], attrib.vertices[3 * idx0.vertex_index + 2]);
            glm::vec3 v1(attrib.vertices[3 * idx1.vertex_index + 0], attrib.vertices[3 * idx1.vertex_index + 1], attrib.vertices[3 * idx1.vertex_index + 2]);
            glm::vec3 v2(attrib.vertices[3 * idx2.vertex_index + 0], attrib.vertices[3 * idx2.vertex_index + 1], attrib.vertices[3 * idx2.vertex_index + 2]);

            // Calcular la normal de la cara (Flat Shading)
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));

            // Agregar los 3 vértices directamente (sin buscar duplicados)
            // Esto asegura bordes duros y definidos
            glm::vec3 vertices[] = {v0, v1, v2};
            
            for (int j = 0; j < 3; ++j) {
                // Posición (x, y, z)
                model.vertices.push_back(vertices[j].x);
                model.vertices.push_back(vertices[j].y);
                model.vertices.push_back(vertices[j].z);
                
                // Normal (nx, ny, nz) - Usamos la misma normal para los 3 vértices de la cara
                model.vertices.push_back(triangleNormal.x);
                model.vertices.push_back(triangleNormal.y);
                model.vertices.push_back(triangleNormal.z);

                // Índice: Simplemente el siguiente disponible
                model.indices.push_back(static_cast<unsigned int>(model.indices.size()));
            }
        }
    }

    // Procesar Materiales (Igual que antes)
    if (!materials.empty()) {
        model.color = glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]);
        model.originalColor = model.color;
    } else {
        model.color = glm::vec3(0.7f, 0.7f, 0.7f); // Gris por defecto
        model.originalColor = model.color;
    }

    // Normalizar si se solicita
    if (normalize) {
        Model::Normalize(model);
    }  

    // Configurar buffers OpenGL
    model.setupModel();

    // Calcular Bounding Box (Caja delimitadora)
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);
    // Nota: Como ahora vertices tiene pos+norm intercalado, saltamos de 6 en 6
    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        glm::vec3 pos(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }
    model.localMinBounds = minBounds;
    model.localMaxBounds = maxBounds;
    
    return model;
}

// ... (resto del código anterior) ...

void Model::drawDebugNormals(GLuint shaderProgram, const glm::vec3& color) const {
    std::vector<float> normalLines;
    for (size_t i = 0; i < vertices.size(); i += 6) {
        glm::vec3 position(vertices[i], vertices[i + 1], vertices[i + 2]);
        glm::vec3 normal(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        
        normalLines.push_back(position.x);
        normalLines.push_back(position.y);
        normalLines.push_back(position.z);
        normalLines.push_back(position.x + normal.x * 0.1f);
        normalLines.push_back(position.y + normal.y * 0.1f);
        normalLines.push_back(position.z + normal.z * 0.1f);
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(float), normalLines.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalsColor"), true);
    glUniform3fv(glGetUniformLocation(shaderProgram, "normalsColor"), 1, glm::value_ptr(color));

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(normalLines.size() / 3));

    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalsColor"), false);
}

void Model::drawDebugBoundingBox(GLuint shaderProgram, const glm::vec3& color) const {
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);

    for (size_t i = 0; i < vertices.size(); i += 6) {
        glm::vec3 pos(vertices[i], vertices[i + 1], vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }

    std::vector<glm::vec3> corners = {
        {minBounds.x, minBounds.y, minBounds.z}, {maxBounds.x, minBounds.y, minBounds.z},
        {maxBounds.x, minBounds.y, minBounds.z}, {maxBounds.x, maxBounds.y, minBounds.z},
        {maxBounds.x, maxBounds.y, minBounds.z}, {minBounds.x, maxBounds.y, minBounds.z},
        {minBounds.x, maxBounds.y, minBounds.z}, {minBounds.x, minBounds.y, minBounds.z},
        {minBounds.x, minBounds.y, maxBounds.z}, {maxBounds.x, minBounds.y, maxBounds.z},
        {maxBounds.x, minBounds.y, maxBounds.z}, {maxBounds.x, maxBounds.y, maxBounds.z},
        {maxBounds.x, maxBounds.y, maxBounds.z}, {minBounds.x, maxBounds.y, maxBounds.z},
        {minBounds.x, maxBounds.y, maxBounds.z}, {minBounds.x, minBounds.y, maxBounds.z},
        {minBounds.x, minBounds.y, minBounds.z}, {minBounds.x, minBounds.y, maxBounds.z},
        {maxBounds.x, minBounds.y, minBounds.z}, {maxBounds.x, minBounds.y, maxBounds.z},
        {maxBounds.x, maxBounds.y, minBounds.z}, {maxBounds.x, maxBounds.y, maxBounds.z},
        {minBounds.x, maxBounds.y, minBounds.z}, {minBounds.x, maxBounds.y, maxBounds.z},
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(glm::vec3), corners.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useBoundingBoxColor"), true);
    glUniform3fv(glGetUniformLocation(shaderProgram, "boundingBoxColor"), 1, glm::value_ptr(color));
   
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(corners.size()));
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glUniform1i(glGetUniformLocation(shaderProgram, "useBoundingBoxColor"), false);
}