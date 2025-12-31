#include "UIManager.h"

// Función auxiliar para mostrar tooltips (ayuda al pasar el mouse)
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
    
    // --- ESTILO PRO ---
    // Configuramos un tema oscuro pero con bordes redondeados y colores más suaves
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;     // Ventanas redondeadas
    style.FrameRounding = 4.0f;      // Botones/Inputs redondeados
    style.GrabRounding = 4.0f;       // Sliders redondeados
    style.FramePadding = ImVec2(5, 5); // Un poco más de espacio interno
    style.ItemSpacing = ImVec2(8, 6);  // Espacio entre elementos

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void UIManager::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::Render(UIState& state, std::vector<Model>& models, int selectedModelIndex, double fps) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. BARRA DE MENÚ SUPERIOR (Tipo aplicación de escritorio)
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Archivo")) {
            if (ImGui::MenuItem("Importar Modelo (O)")) { 
                // Nota: La lógica sigue en el main por ahora, esto es visual
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Guardar Escena (M)")) {}
            if (ImGui::MenuItem("Cargar Escena (N)")) {}
            if (ImGui::MenuItem("Limpiar Escena (C)")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Salir (Esc)")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Vista")) {
            ImGui::MenuItem("Panel de Propiedades", NULL, &state.showBoundingBox); // Ejemplo
            ImGui::EndMenu();
        }
        // Mostrar FPS a la derecha de la barra
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Text("FPS: %.1f", fps);
        ImGui::EndMainMenuBar();
    }

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
            ImGui::Indent(); // Indentar opciones hijas
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

    // 3. Ventana Flotante de FPS (Opcional)
    if (state.showFPS) {
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparente
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 90, 30), ImGuiCond_Always);
        ImGui::Begin("Stats", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text("%.1f FPS", fps);
        ImGui::End();
    }

    // Renderizar al final
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}