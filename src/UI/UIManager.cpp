#include "UIManager.h"
#include <imgui_internal.h>
#include <string>

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.05f) {
    bool changed = false;
    ImGui::PushID(label.c_str());
    float buttonWidth = 20.0f; 
    float itemWidth = (ImGui::GetContentRegionAvail().x - 40) / 3.0f - buttonWidth; 

    // --- EJE X (ROJO) ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    if (ImGui::Button("X", ImVec2(buttonWidth, 0))) {
        values.x = resetValue; 
        changed = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##X", &values.x, speed, 0.0f, 0.0f, "%.2f")) changed = true;
    
    // --- EJE Y (VERDE) ---
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Y", ImVec2(buttonWidth, 0))) {
        values.y = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##Y", &values.y, speed, 0.0f, 0.0f, "%.2f")) changed = true;

    // --- EJE Z (AZUL) ---
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    if (ImGui::Button("Z", ImVec2(buttonWidth, 0))) {
        values.z = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##Z", &values.z, speed, 0.0f, 0.0f, "%.2f")) changed = true;

    ImGui::SameLine();
    if (ImGui::Button("R")) { 
        values = glm::vec3(resetValue);
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resetear %s", label.c_str());

    ImGui::PopID();
    return changed;
}

void UIManager::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // --- ESTILO PRO ---
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;     
    style.FrameRounding = 4.0f;      
    style.GrabRounding = 4.0f;       
    style.FramePadding = ImVec2(5, 5); 
    style.ItemSpacing = ImVec2(8, 6);  

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void UIManager::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::Render(GLFWwindow* window, UIState& state, std::vector<Model>& models, int& selectedModelIndex, double fps) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. BARRA DE MENÚ SUPERIOR
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Archivo")) {
            if (ImGui::MenuItem("Importar Modelo (Ctrl+O)")) {
                SceneManager::ImportModel(models);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Guardar Escena (Ctrl+S)")) {
                for (auto& model : models) model.applyTransformations();
                SceneManager::Save("scene.txt", models);
            }
            if (ImGui::MenuItem("Cargar Escena (Ctrl+L)")) {
                selectedModelIndex = -1;
                SceneManager::Load("scene.txt", models);
            }
            if (ImGui::MenuItem("Limpiar Escena (Ctrl+N)")) {
                selectedModelIndex = -1;
                SceneManager::Clear(models);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Salir (Esc)")) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Vista")) {
            ImGui::MenuItem("Panel de Propiedades", NULL, &state.showPropertiesPanel); 
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (state.showPropertiesPanel) {
    // 2. PANEL LATERAL (Inspector)
    // Fijamos una posición y tamaño por defecto para la primera vez
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);

        ImGui::Begin("Inspector de Propiedades");

        // SECCIÓN: Geometría y Estilo
        if (ImGui::CollapsingHeader("Visualización del Modelo", ImGuiTreeNodeFlags_DefaultOpen)) {
            
            // Vértices
            ImGui::Checkbox("Mostrar Vértices", &state.showVertices);
            if (state.showVertices) {
                ImGui::Indent();
                ImGui::ColorEdit3("Color Vértices", (float*)&state.vertexColor, ImGuiColorEditFlags_NoInputs);
                ImGui::SliderFloat("Tamaño", &state.vertexSize, 1.0f, 10.0f);
                ImGui::Unindent();
            }

            // Alambrado (Wireframe)
            ImGui::Checkbox("Mostrar Alambrado", &state.showWireframe);
            if (state.showWireframe) {
                ImGui::Indent();
                ImGui::ColorEdit3("Color Líneas", (float*)&state.wireframeColor, ImGuiColorEditFlags_NoInputs);
                ImGui::Unindent();
            }

            // Normales
            ImGui::Checkbox("Mostrar Normales", &state.showNormals);
            if (state.showNormals) {
                ImGui::Indent();
                ImGui::ColorEdit3("Color Normales", (float*)&state.normalsColor, ImGuiColorEditFlags_NoInputs);
                ImGui::Unindent();
            }
            
            ImGui::Separator();
            
            // Color del Relleno (Logic para modelos)
            if (!models.empty()) {
                ImGui::Checkbox("Sobrescribir Color Base", &state.enableColorChange);
                if (state.enableColorChange) {
                    ImGui::Indent();
                    ImGui::ColorEdit3("Color Base", (float*)&state.newColor);
                    ImGui::SameLine(); HelpMarker("Cambia el color de todos los modelos no seleccionados");
                    
                    // Aplicar lógica aquí mismo para simplificar main
                    for (size_t i = 0; i < models.size(); ++i) {
                        if (static_cast<int>(i) != selectedModelIndex) {
                            models[i].color = state.newColor;
                        }
                    }
                    ImGui::Unindent();
                }
            }
        }

        // SECCIÓN: Selección
        if (ImGui::CollapsingHeader("Selección y Debug")) {
            ImGui::Checkbox("Mostrar Bounding Box", &state.showBoundingBox);
            if (state.showBoundingBox) {
                ImGui::SameLine();
                ImGui::ColorEdit3("##BoxColor", (float*)&state.boundingBoxColor, ImGuiColorEditFlags_NoInputs);
            }
            
            if (selectedModelIndex != -1) {
                ImGui::TextColored(ImVec4(0,1,0,1), "Modelo Seleccionado: ID %d", selectedModelIndex);
            } else {
                ImGui::TextDisabled("Ningún modelo seleccionado");
            }
        }

        // SECCIÓN: Configuración Motor
        if (ImGui::CollapsingHeader("Configuración del Motor")) {
            ImGui::Checkbox("Habilitar Z-buffer", &state.enableDepthTest);
            ImGui::SameLine(); HelpMarker("Evita que objetos lejanos se dibujen sobre los cercanos.");

            ImGui::Checkbox("Back-face Culling", &state.enableBackFaceCulling);
            ImGui::SameLine(); HelpMarker("No dibuja las caras traseras para ganar rendimiento.");

            ImGui::Checkbox("Antialiasing (MSAA)", &state.enableAntialiasing);
            ImGui::SameLine(); HelpMarker("Suaviza los bordes dentados (Sierra).");
            
            ImGui::Separator();
            ImGui::Checkbox("Mostrar ventana FPS flotante", &state.showFPS);
        }

        ImGui::End(); // Fin Inspector
    }

    // 3. Ventana Flotante de FPS 
    if (state.showFPS) {
        ImGui::SetNextWindowBgAlpha(0.65f); 
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 90, 30), ImGuiCond_Always);
        ImGui::Begin("Stats", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text("%.1f FPS", fps);
        ImGui::End();
    }

    // 4. BARRA DE HERRAMIENTAS INFERIOR (Transformaciones)
    // Solo mostrar si hay un modelo seleccionado
    if (selectedModelIndex != -1 && selectedModelIndex < (int)models.size()) {
        Model& currentModel = models[selectedModelIndex];

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 120));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 120));
        ImGui::SetNextWindowBgAlpha(0.9f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Transform Bar", NULL, flags);

        if (currentModel.isLight) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "FUENTE DE LUZ SELECCIONADA");
            ImGui::Separator();

            ImGui::Columns(2, "LightCols", false);
            ImGui::Text("Posición de la Luz");
            if (DrawVec3Control("PosLight", currentModel.position, 0.0f, 0.02f)) {
                currentModel.updateTransformMatrix();
            }
            ImGui::NextColumn();
            ImGui::Text("Color de la Luz");
            ImGui::SetNextItemWidth(-1);
            ImGui::ColorEdit3("##LightColor", (float*)&currentModel.color, ImGuiColorEditFlags_NoInputs);
            
        } else {
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "OBJETO SELECCIONADO: ID %d", selectedModelIndex);
            ImGui::Separator();
            ImGui::Columns(4, "TransformCols", false); 

            // Posición
            ImGui::Text("Posición");
            if (DrawVec3Control("Pos", currentModel.position, 0.0f, 0.02f)) {
                currentModel.updateTransformMatrix();
            }
            ImGui::NextColumn();

            // Rotación
            ImGui::Text("Rotación");
            if (DrawVec3Control("Rot", currentModel.rotation, 0.0f, 0.5f)) {
                currentModel.updateTransformMatrix();
            }
            ImGui::NextColumn();

            // Escala
            ImGui::Text("Escala");
            if (DrawVec3Control("Scl", currentModel.scale, 1.0f, 0.02f)) {
                if(currentModel.scale.x < 0.01f) currentModel.scale.x = 0.01f;
                if(currentModel.scale.y < 0.01f) currentModel.scale.y = 0.01f;
                if(currentModel.scale.z < 0.01f) currentModel.scale.z = 0.01f;
                currentModel.updateTransformMatrix();
            }
            ImGui::NextColumn();

            ImGui::Dummy(ImVec2(0, 15)); 
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("ELIMINAR", ImVec2(-1, 40))) {
                SceneManager::DeleteSelectedModel(models, selectedModelIndex);
            }
            ImGui::PopStyleColor(2);
        }   
        ImGui::EndColumns();
        ImGui::End();   
    }

    // Renderizar al final
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}