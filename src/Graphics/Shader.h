#pragma once

#include <glad/glad.h> // Incluir glad para obtener todos los encabezados de OpenGL
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID; // El ID del programa de shader

    // Constructor que lee y construye el shader
    Shader(const char* vertexSource, const char* fragmentSource);

    // Activar el shader
    void use();

    // Funciones útiles para uniformes (esto limpiará mucho tu main después)
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
    // Funciones internas para verificar errores de compilación
    void checkCompileErrors(unsigned int shader, std::string type);
};