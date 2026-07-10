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

void UIManager::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // --- ESTILO PRO (Mejorado para Usabilidad) ---
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;     
    style.FrameRounding = 4.0f;      
    style.GrabRounding = 4.0f;       
    style.FramePadding = ImVec2(6, 6); 
    style.ItemSpacing = ImVec2(8, 8);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.94f); // Fondo ligeramente más oscuro para contraste
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

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

    // 1. BARRA DE MENÚ SUPERIOR (Nielsen: Consistencia y Estándares)
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Archivo")) {
            if (ImGui::MenuItem("Importar Modelo", "Ctrl+O")) {
                SceneManager::ImportModel(models);
                if (!models.empty() && models.back().hasTexture) {
                    state.renderMode = 1; 
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Guardar Escena", "Ctrl+S")) {
                SceneManager::Save("scene.txt", models);
            }
            if (ImGui::MenuItem("Cargar Escena", "Ctrl+L")) {
                selectedModelIndex = -1;
                SceneManager::Load("scene.txt", models);
                for (const auto& m : models) {
                    if (m.hasTexture) { 
                        state.renderMode = 1; 
                        break; 
                    }
                }
            }
            if (ImGui::MenuItem("Limpiar Escena", "Ctrl+N")) {
                selectedModelIndex = -1;
                SceneManager::Clear(models);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Salir", "Esc")) {
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
        ImGui::SetNextWindowPos(ImVec2(10, 35), ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspector", &state.showPropertiesPanel, ImGuiWindowFlags_AlwaysAutoResize);

        // Nielsen: Diseño Estético y Minimalista (Uso de Pestañas en lugar de lista larga)
        if (ImGui::BeginTabBar("InspectorTabs")) {
            
            // --- PESTAÑA 1: APARIENCIA ---
            if (ImGui::BeginTabItem("Apariencia")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ESTILO VISUAL");
                ImGui::Separator();
                
                const char* renderModes[] = { 
                    "1. Vista Solida", "2. Vista con Textura", "3. Sin Iluminacion", 
                    "4. Normal (Phong)", "5. Caricatura", "6. Boceto", "7. Holograma" 
                };

                bool hasAnyTexture = false;
                for (const auto& m : models) {
                    if (m.hasTexture) { hasAnyTexture = true; break; }
                }

                if (!hasAnyTexture && state.renderMode == 1) state.renderMode = 0; 

                ImGui::SetNextItemWidth(-1); // Ocupar todo el ancho
                if (ImGui::BeginCombo("##RenderMode", renderModes[state.renderMode])) {
                    for (int n = 0; n < IM_ARRAYSIZE(renderModes); n++) {
                        bool is_selected = (state.renderMode == n);
                        ImGuiSelectableFlags flags = (n == 1 && !hasAnyTexture) ? ImGuiSelectableFlags_Disabled : 0;

                        if (ImGui::Selectable(renderModes[n], is_selected, flags)) state.renderMode = n;
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "COLOR GLOBAL");
                ImGui::Separator();
                if (!models.empty()) {
                    if (ImGui::Checkbox("Sobrescribir Color Base", &state.enableColorChange)) {
                        if (!state.enableColorChange) {
                            for (size_t i = 0; i < models.size(); ++i) {
                                if (static_cast<int>(i) != selectedModelIndex && !models[i].isLight) {
                                    models[i].color = models[i].originalColor;
                                }
                            }
                        }
                    }
                    if (state.enableColorChange) {
                        ImGui::SetNextItemWidth(-1);
                        ImGui::ColorEdit3("##NewColor", (float*)&state.newColor);
                        
                        for (size_t i = 0; i < models.size(); ++i) {
                            if (static_cast<int>(i) != selectedModelIndex && !models[i].isLight) {
                                models[i].color = state.newColor;
                            }
                        }
                    }
                } else {
                    ImGui::TextDisabled("No hay modelos en la escena.");
                }
                ImGui::EndTabItem();
            }

            // --- PESTAÑA 2: MALLA Y DEBUG ---
            if (ImGui::BeginTabItem("Malla & Debug")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GEOMETRÍA");
                ImGui::Separator();
                
                ImGui::Checkbox("Vértices", &state.showVertices);
                if (state.showVertices) {
                    ImGui::Indent();
                    ImGui::ColorEdit3("Color", (float*)&state.vertexColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::SliderFloat("Tamaño", &state.vertexSize, 1.0f, 10.0f);
                    ImGui::Unindent();
                }

                ImGui::Checkbox("Alambrado (Wireframe)", &state.showWireframe);
                if (state.showWireframe) {
                    ImGui::Indent();
                    ImGui::ColorEdit3("Color Líneas", (float*)&state.wireframeColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::Unindent();
                }

                ImGui::Checkbox("Normales", &state.showNormals);
                if (state.showNormals) {
                    ImGui::Indent();
                    ImGui::ColorEdit3("Color Normales", (float*)&state.normalsColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::Unindent();
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "HERRAMIENTAS");
                ImGui::Separator();
                
                ImGui::Checkbox("Caja delimitadora (Bounding Box)", &state.showBoundingBox);
                if (state.showBoundingBox) {
                    ImGui::Indent();
                    ImGui::ColorEdit3("Color Caja", (float*)&state.boundingBoxColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::Unindent();
                }
                ImGui::EndTabItem();
            }

            // --- PESTAÑA 3: MOTOR ---
            if (ImGui::BeginTabItem("Motor")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GRÁFICOS");
                ImGui::Separator();

                ImGui::Checkbox("Z-buffer (Depth Test)", &state.enableDepthTest);
                ImGui::SameLine(); HelpMarker("Evita que objetos lejanos se dibujen sobre los cercanos.");

                ImGui::Checkbox("Culling (Caras traseras)", &state.enableBackFaceCulling);
                ImGui::SameLine(); HelpMarker("No dibuja las caras traseras para ganar rendimiento.");

                ImGui::Checkbox("Antialiasing (MSAA)", &state.enableAntialiasing);
                ImGui::SameLine(); HelpMarker("Suaviza los bordes dentados.");
                
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "INTERFAZ");
                ImGui::Separator();
                ImGui::Checkbox("Mostrar FPS", &state.showFPS);

                ImGui::EndTabItem();
            }

            if (selectedModelIndex != -1 && selectedModelIndex < (int)models.size()) {
                Model& currentModel = models[selectedModelIndex];
                
                if (ImGui::BeginTabItem("Transformación")) {
                    ImGui::Spacing();
                    if (currentModel.isLight) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FUENTE DE LUZ (ID: %d)", selectedModelIndex);
                        ImGui::Separator();
                        
                        ImGui::Text("Posición (X, Y, Z)");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30); // Deja espacio para el botón R
                        if (ImGui::DragFloat3("##PosLight", (float*)&currentModel.position, 0.05f)) currentModel.updateTransformMatrix();
                        ImGui::SameLine();
                        if (ImGui::Button("R##RPosL", ImVec2(22, 0))) { currentModel.position = glm::vec3(0.0f); currentModel.updateTransformMatrix(); }
                        
                        ImGui::Spacing();
                        ImGui::Text("Color / Intensidad");
                        ImGui::SetNextItemWidth(-1);
                        ImGui::ColorEdit3("##LightColor", (float*)&currentModel.color);
                    } else {
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "MODELO SELECCIONADO (ID: %d)", selectedModelIndex);
                        ImGui::Separator();

                        ImGui::Text("Posición (X, Y, Z)");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
                        if (ImGui::DragFloat3("##Pos", (float*)&currentModel.position, 0.05f)) currentModel.updateTransformMatrix();
                        ImGui::SameLine();
                        if (ImGui::Button("R##RPos", ImVec2(22, 0))) { currentModel.position = glm::vec3(0.0f); currentModel.updateTransformMatrix(); }

                        ImGui::Spacing();
                        ImGui::Text("Rotación (X, Y, Z)");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
                        if (ImGui::DragFloat3("##Rot", (float*)&currentModel.rotation, 0.5f)) currentModel.updateTransformMatrix();
                        ImGui::SameLine();
                        if (ImGui::Button("R##RRot", ImVec2(22, 0))) { currentModel.rotation = glm::vec3(0.0f); currentModel.updateTransformMatrix(); }

                        ImGui::Spacing();
                        ImGui::Text("Escala Individual (X, Y, Z)");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
                        if (ImGui::DragFloat3("##Scl", (float*)&currentModel.scale, 0.02f, 0.01f, 100.0f, "%.2f")) {
                            if(currentModel.scale.x < 0.01f) currentModel.scale.x = 0.01f;
                            if(currentModel.scale.y < 0.01f) currentModel.scale.y = 0.01f;
                            if(currentModel.scale.z < 0.01f) currentModel.scale.z = 0.01f;
                            currentModel.updateTransformMatrix();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("R##RScl", ImVec2(22, 0))) { currentModel.scale = glm::vec3(1.0f); currentModel.updateTransformMatrix(); }

                        ImGui::Spacing();
                        ImGui::Text("Escala Uniforme");
                        float uScale = currentModel.scale.x; 
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
                        if (ImGui::DragFloat("##Uniform", &uScale, 0.02f, 0.01f, 100.0f, "%.2f")) {
                            currentModel.scale = glm::vec3(uScale);
                            if (currentModel.scale.x < 0.01f) currentModel.scale = glm::vec3(0.01f);
                            currentModel.updateTransformMatrix();
                        }
                    }

                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                    // Calculamos el ancho de la ventana actual para centrar los botones
                    float windowWidth = ImGui::GetWindowSize().x;
                    
                    // Botón Deseleccionar (Ancho del texto + 30px de padding)
                    float deselWidth = ImGui::CalcTextSize("Deseleccionar").x + 30.0f; 
                    ImGui::SetCursorPosX((windowWidth - deselWidth) * 0.5f);
                    if (ImGui::Button("Deseleccionar", ImVec2(deselWidth, 30))) selectedModelIndex = -1;
                    
                    if (!currentModel.isLight) {
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                        
                        // Botón Eliminar (Ancho del texto + 30px de padding)
                        float elimWidth = ImGui::CalcTextSize("ELIMINAR MODELO").x + 30.0f;
                        ImGui::SetCursorPosX((windowWidth - elimWidth) * 0.5f);
                        if (ImGui::Button("ELIMINAR MODELO", ImVec2(elimWidth, 30))) {
                            SceneManager::DeleteSelectedModel(models, selectedModelIndex);
                        }
                        ImGui::PopStyleColor(2);
                    }
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
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
  

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}