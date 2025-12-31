#pragma once

const char* pickingVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* pickingFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec3 pickingColor; // El color único del objeto (su ID codificado)

    void main() {
        FragColor = vec4(pickingColor, 1.0);
    }
)";