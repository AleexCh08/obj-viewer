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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Model::updateTransformMatrix() {
    glm::mat4 mat = glm::mat4(1.0f);
    mat = glm::translate(mat, position);
    mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::scale(mat, scale);
    
    this->transformMatrix = mat;
}

void Model::applyTransformations() {    
    for (size_t i = 0; i < originalVertices.size(); i += 6) {
        glm::vec4 position(originalVertices[i], originalVertices[i+1], originalVertices[i+2], 1.0f);
        position = transformMatrix * position;
        vertices[i] = position.x;
        vertices[i + 1] = position.y;
        vertices[i + 2] = position.z;
    }
    position = glm::vec3(0.0f);
    rotation = glm::vec3(0.0f);
    scale = glm::vec3(1.0f);
    updateTransformMatrix();

    if (debugNormalsVAO != 0) { glDeleteVertexArrays(1, &debugNormalsVAO); debugNormalsVAO = 0; }
    if (debugBoxVAO != 0)     { glDeleteVertexArrays(1, &debugBoxVAO);     debugBoxVAO = 0; }
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

void Model::Normalize(Model& model) {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        glm::vec3 pos(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }

    glm::vec3 size = maxBounds - minBounds;
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
    
    for (const auto& shape : shapes) {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            tinyobj::index_t idx0 = shape.mesh.indices[i + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[i + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[i + 2];

            glm::vec3 v0(attrib.vertices[3 * idx0.vertex_index + 0], attrib.vertices[3 * idx0.vertex_index + 1], attrib.vertices[3 * idx0.vertex_index + 2]);
            glm::vec3 v1(attrib.vertices[3 * idx1.vertex_index + 0], attrib.vertices[3 * idx1.vertex_index + 1], attrib.vertices[3 * idx1.vertex_index + 2]);
            glm::vec3 v2(attrib.vertices[3 * idx2.vertex_index + 0], attrib.vertices[3 * idx2.vertex_index + 1], attrib.vertices[3 * idx2.vertex_index + 2]);

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));

            glm::vec3 vertices[] = {v0, v1, v2};
            
            for (int j = 0; j < 3; ++j) {
                model.vertices.push_back(vertices[j].x);
                model.vertices.push_back(vertices[j].y);
                model.vertices.push_back(vertices[j].z);
                model.vertices.push_back(triangleNormal.x);
                model.vertices.push_back(triangleNormal.y);
                model.vertices.push_back(triangleNormal.z);
                model.indices.push_back(static_cast<unsigned int>(model.indices.size()));
            }
        }
    }

    if (!materials.empty()) {
        model.color = glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]);
        model.originalColor = model.color;
    } else {
        model.color = glm::vec3(0.7f, 0.7f, 0.7f);
        model.originalColor = model.color;
    }

    if (normalize) {
        Model::Normalize(model);
    }  

    model.setupModel();

    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);
    for (size_t i = 0; i < model.vertices.size(); i += 6) {
        glm::vec3 pos(model.vertices[i], model.vertices[i + 1], model.vertices[i + 2]);
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }
    model.localMinBounds = minBounds;
    model.localMaxBounds = maxBounds;
    
    return model;
}

void Model::drawDebugNormals(GLuint shaderProgram, const glm::vec3& color) {
    // 1. SI NO EXISTE, LO CREAMOS (Solo ocurre 1 vez)
    if (debugNormalsVAO == 0) {
        std::vector<float> normalLines;
        // Usamos la geometría original para generar las líneas
        for (size_t i = 0; i < vertices.size(); i += 6) {
            glm::vec3 pos(vertices[i], vertices[i + 1], vertices[i + 2]);
            glm::vec3 norm(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
            
            // Línea desde el vértice hacia afuera
            normalLines.push_back(pos.x);
            normalLines.push_back(pos.y);
            normalLines.push_back(pos.z);
            
            glm::vec3 endPos = pos + norm * 0.1f; // Largo de la normal
            normalLines.push_back(endPos.x);
            normalLines.push_back(endPos.y);
            normalLines.push_back(endPos.z);
        }

        glGenVertexArrays(1, &debugNormalsVAO);
        glGenBuffers(1, &debugNormalsVBO);
        glBindVertexArray(debugNormalsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debugNormalsVBO);
        glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(float), normalLines.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // 2. DIBUJAR (Esto es lo único que se ejecuta en cada frame -> Rápido)
    glBindVertexArray(debugNormalsVAO);
    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalsColor"), true);
    glUniform3fv(glGetUniformLocation(shaderProgram, "normalsColor"), 1, glm::value_ptr(color));

    // Calculamos cantidad de vértices (vertices.size() / 6 * 2)
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size() / 3));

    glBindVertexArray(0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalsColor"), false);
}

void Model::drawDebugBoundingBox(GLuint shaderProgram, const glm::vec3& color) {
    // 1. SI NO EXISTE, LO CREAMOS
    if (debugBoxVAO == 0) {
        // Usamos los bounds que YA calculamos al cargar el modelo (O(1))
        // en lugar de recorrer todos los vértices otra vez (O(N))
        glm::vec3 min = localMinBounds;
        glm::vec3 max = localMaxBounds;

        std::vector<glm::vec3> lines = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z}, // Base
            {max.x, min.y, min.z}, {max.x, min.y, max.z},
            {max.x, min.y, max.z}, {min.x, min.y, max.z},
            {min.x, min.y, max.z}, {min.x, min.y, min.z},
            
            {min.x, max.y, min.z}, {max.x, max.y, min.z}, // Tope
            {max.x, max.y, min.z}, {max.x, max.y, max.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z},
            {min.x, max.y, max.z}, {min.x, max.y, min.z},
            
            {min.x, min.y, min.z}, {min.x, max.y, min.z}, // Columnas
            {max.x, min.y, min.z}, {max.x, max.y, min.z},
            {max.x, min.y, max.z}, {max.x, max.y, max.z},
            {min.x, min.y, max.z}, {min.x, max.y, max.z}
        };

        glGenVertexArrays(1, &debugBoxVAO);
        glGenBuffers(1, &debugBoxVBO);
        glBindVertexArray(debugBoxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debugBoxVBO);
        glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // 2. DIBUJAR
    glUseProgram(shaderProgram);
    glBindVertexArray(debugBoxVAO);
    
    glUniform1i(glGetUniformLocation(shaderProgram, "useBoundingBoxColor"), true);
    glUniform3fv(glGetUniformLocation(shaderProgram, "boundingBoxColor"), 1, glm::value_ptr(color));
   
    glDrawArrays(GL_LINES, 0, 24);

    glBindVertexArray(0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useBoundingBoxColor"), false);
}