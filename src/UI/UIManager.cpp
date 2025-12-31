#include "UIManager.h"

void UIManager::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
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

    // --- Ventana de Opciones ---
    ImGui::Begin("Opciones de Renderizado");    
    
    ImGui::Checkbox("Ver/Ocultar vértices", &state.showVertices);
    if (state.showVertices) {
        ImGui::ColorEdit3("Color de vértices", (float*)&state.vertexColor);
        ImGui::SliderFloat("Tamaño de vértices", &state.vertexSize, 1.0f, 10.0f);
    }

    ImGui::Checkbox("Ver/Ocultar alambrado", &state.showWireframe);
    if (state.showWireframe) {
        ImGui::ColorEdit3("Color de alambrado", (float*)&state.wireframeColor);
    }

    if (!models.empty()) {                       
        ImGui::Checkbox("Ver/Ocultar relleno de triangulos", &state.enableColorChange);
        if (state.enableColorChange) {               
            ImGui::ColorEdit3("Color del modelo", (float*)&state.newColor);               
            for (size_t i = 0; i < models.size(); ++i) {
                if (static_cast<int>(i) != selectedModelIndex) {
                    models[i].color = state.newColor;
                }
            }
        }            
    }

    ImGui::Checkbox("Ver/Ocultar normales", &state.showNormals);
    if (state.showNormals) {
        ImGui::ColorEdit3("Color de las normales", (float*)&state.normalsColor);
    }

    ImGui::Checkbox("Habilitar Z-buffer", &state.enableDepthTest);
    ImGui::Checkbox("Habilitar Back-face Culling", &state.enableBackFaceCulling);
    ImGui::Checkbox("Mostrar FPS", &state.showFPS);
    ImGui::Checkbox("Habilitar Antialiasing", &state.enableAntialiasing);
    ImGui::Checkbox("Mostrar Bounding Box", &state.showBoundingBox);
    
    if (state.showBoundingBox) {
        ImGui::ColorEdit3("Color del Bounding Box", (float*)&state.boundingBoxColor);
    }    
    ImGui::End();

    // --- Ventana de FPS ---
    if (state.showFPS) {
        ImGui::Begin("FPS");
        ImGui::Text("%.2f", fps);
        ImGui::End();
    }

    // Renderizar al final
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}