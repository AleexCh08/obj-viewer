#pragma once

const char* vertexShaderSource = 
R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float pointSize;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoords = aTexCoords;
        gl_Position = projection * view * vec4(FragPos, 1.0);    
        gl_PointSize = pointSize;
    }
)";

const char* fragmentShaderSource = 
R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoords;

    uniform vec3 objectColor;
    uniform vec3 vertexColor;
    uniform vec3 wireframeColor;
    uniform vec3 normalsColor;
    uniform vec3 boundingBoxColor;

    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPos;

    uniform int isLightSource;

    uniform bool useVertexColor;
    uniform bool useWireframeColor; 
    uniform bool useNormalsColor;
    uniform bool useBoundingBoxColor;

    uniform bool isGrid;
    uniform vec3 gridColor;

    uniform sampler2D texture1;
    uniform int hasTexture;
    
    uniform int renderMode; // NUEVA VARIABLE PARA CONTROLAR EL ESTILO

    void main() {
        // Excepciones para depuración y luces (se dibujan sin afectar el estilo)
        if (isLightSource == 1) { FragColor = vec4(objectColor, 1.0); return; }
        if (isGrid) { FragColor = vec4(gridColor, 1.0); return; }
        if (useBoundingBoxColor) { FragColor = vec4(boundingBoxColor, 1.0); return; }
        if (useNormalsColor) { FragColor = vec4(normalsColor, 1.0); return; }
      
        // 1. OBTENER COLOR BASE (Textura o Color Plano)
        vec4 baseColor;
        
        // Determinar si debemos usar textura en este modo
        bool useTex = (hasTexture == 1);
        if (renderMode == 0) {
            useTex = false; // Vista Sólida: IGNORA texturas siempre
        } else if (renderMode == 1) {
            useTex = (hasTexture == 1); // Vista Textura: usa si tiene
        }

        if (useTex) {
            baseColor = texture(texture1, TexCoords);
        } else {
            vec3 currentColor = objectColor;
            if (useVertexColor) currentColor = vertexColor;
            else if (useWireframeColor) currentColor = wireframeColor;
            baseColor = vec4(currentColor, 1.0);
        }

        // 2. APLICAR ESTILO SEGÚN EL RENDER MODE
        if (renderMode == 0 || renderMode == 1) {
            // MODOS 0 y 1: Sólido y Textura (Iluminación mate básica)
            float ambientStrength = 0.4;
            vec3 ambient = ambientStrength * lightColor;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            vec3 lighting = (ambient + diffuse);
            FragColor = vec4(lighting * baseColor.rgb, baseColor.a);
        }
        else if (renderMode == 2) {
            // MODO 2: Sin iluminación (Color o textura pura)
            FragColor = baseColor;
        }
        else if (renderMode == 3) {
            // MODO 3: Normal / Iluminación (Phong completo con brillo especular)
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * lightColor;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 lighting = (ambient + diffuse + specular);
            FragColor = vec4(lighting * baseColor.rgb, baseColor.a);
        } else if (renderMode == 4) {
            // MODO 4: Estilo Caricatura (Cel Shading)
            float ambientStrength = 0.3; // Añadimos luz base para evitar oscuridad total
            vec3 ambient = ambientStrength * lightColor;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            vec3 viewDir = normalize(viewPos - FragPos);

            // 1. Discretizar la luz difusa en "escalones" duros
            float diff = max(dot(norm, lightDir), 0.0);
            float celDiff;
            if (diff > 0.8) celDiff = 1.0;
            else if (diff > 0.5) celDiff = 0.6;
            else if (diff > 0.2) celDiff = 0.3;
            else celDiff = 0.0; 

            vec3 diffuse = celDiff * lightColor;

            // 2. Simular un contorno (Outline) negro en los bordes
            float rim = max(dot(viewDir, norm), 0.0);
            float outline = (rim < 0.25) ? 0.0 : 1.0;

            // Multiplicamos la suma de luz por el color base y aplicamos el contorno
            FragColor = vec4((ambient + diffuse) * baseColor.rgb * outline, baseColor.a);
        }
        else {
            // Fallback para modos no programados aún
            FragColor = baseColor; 
        }
    }
)";