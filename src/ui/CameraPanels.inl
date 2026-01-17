// CameraPanels.inl - Inline implementation of camera and creature info panels
// Include this file in main.cpp after the help overlay function

// ============================================================================
// Creature Info Panel (Phase 6 - Camera & Selection)
// ============================================================================
void RenderCreatureInfoPanel() {
    if (g_app.selectedCreatureIndex < 0 && g_app.cameraFollowMode == AppState::CameraFollowMode::NONE) return;
    int targetIdx = (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) ? g_app.followCreatureId : g_app.selectedCreatureIndex;
    if (targetIdx < 0) return;

    auto activeCreatures = g_app.world.creaturePool.getActiveCreatures();
    const SimCreature* sel = nullptr;
    if (targetIdx < static_cast<int>(activeCreatures.size())) {
        sel = activeCreatures[targetIdx];
    }
    if (!sel || !sel->alive) {
        if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) {
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            g_app.followCreatureId = -1;
        }
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 310, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(290, 380), ImGuiCond_FirstUseEver);
    bool show = true;
    if (ImGui::Begin("Creature Info", &show)) {
        // Type name and color
        const char* typeNames[] = {"Herbivore","Carnivore","Omnivore","Flying","Aquatic","Unknown"};
        ImVec4 typeColors[] = {
            ImVec4(0.3f,0.9f,0.3f,1), ImVec4(0.9f,0.3f,0.3f,1), ImVec4(0.9f,0.6f,0.2f,1),
            ImVec4(0.6f,0.6f,0.9f,1), ImVec4(0.3f,0.7f,0.9f,1), ImVec4(0.7f,0.7f,0.7f,1)
        };
        int ti = std::min((int)sel->type, 5);
        ImGui::TextColored(typeColors[ti], "%s #%u", typeNames[ti], sel->id);

        // Follow/Unfollow button
        ImGui::SameLine(ImGui::GetWindowWidth() - 75);
        if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE && g_app.followCreatureId == targetIdx) {
            if (ImGui::Button("Unfollow")) {
                g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
                g_app.followCreatureId = -1;
            }
        } else {
            if (ImGui::Button("Follow")) {
                g_app.cameraFollowMode = AppState::CameraFollowMode::FOLLOW;
                g_app.followCreatureId = targetIdx;
                g_app.followOrbitAngle = g_app.cameraYaw;
            }
        }
        ImGui::Separator();

        // Status section
        if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
            float energyPct = sel->energy / 200.0f;
            ImVec4 energyColor = (energyPct > 0.5f) ? ImVec4(0.2f,0.9f,0.2f,1) :
                                  (energyPct > 0.25f) ? ImVec4(0.9f,0.9f,0.2f,1) :
                                  ImVec4(0.9f,0.2f,0.2f,1);
            ImGui::Text("Energy:");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, energyColor);
            ImGui::ProgressBar(energyPct, ImVec2(-1,0), "");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%.1f", sel->energy);

            ImGui::Text("Position: (%.1f, %.1f)", sel->position.x, sel->position.z);
            float speed = glm::length(sel->velocity);
            ImGui::Text("Speed: %.2f", speed);
        }

        // Genome section
        if (ImGui::CollapsingHeader("Genome", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Size: %.2f", sel->genome.size);
            ImGui::Text("Speed: %.2f", sel->genome.speed);
            ImGui::Text("Vision: %.1f", sel->genome.visionRange);
            ImGui::Text("Efficiency: %.2f", sel->genome.efficiency);
            ImGui::Text("Color:");
            ImGui::SameLine();
            ImGui::ColorButton("##creatureColor",
                ImVec4(sel->genome.color.r, sel->genome.color.g, sel->genome.color.b, 1),
                0, ImVec2(40,18));
        }

        // Behavior section
        if (ImGui::CollapsingHeader("Behavior")) {
            float sp = glm::length(sel->velocity);
            const char* behavior = "Wandering";
            if (sp > sel->genome.speed * 0.8f) {
                behavior = (sel->type == CreatureType::CARNIVORE) ? "Hunting" : "Fleeing";
            } else if (sel->energy > 150.0f) {
                behavior = "Well-fed";
            } else if (sel->energy < 30.0f) {
                behavior = "Starving";
            }
            ImGui::Text("State: %s", behavior);
        }

        // Follow camera controls
        if (ImGui::CollapsingHeader("Follow Camera")) {
            ImGui::SliderFloat("Distance", &g_app.followDistance, 5.0f, 100.0f);
            ImGui::SliderFloat("Height", &g_app.followHeight, 2.0f, 50.0f);
            ImGui::SliderFloat("Orbit Angle", &g_app.followOrbitAngle, -180.0f, 180.0f);
        }
    }
    ImGui::End();

    if (!show) {
        g_app.selectedCreatureIndex = -1;
        g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
        g_app.followCreatureId = -1;
    }
}

// ============================================================================
// Camera Settings Panel (Phase 6)
// ============================================================================
void RenderCameraSettingsPanel() {
    if (!g_app.showDebugPanel) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 240), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(270, 220), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Camera Settings")) {
        // Mouse settings
        if (ImGui::CollapsingHeader("Mouse Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Invert Horizontal", &g_app.invertMouseX);
            ImGui::SameLine();
            ImGui::Checkbox("Invert Vertical", &g_app.invertMouseY);
            ImGui::SliderFloat("Sensitivity", &g_app.mouseSensitivity, 0.05f, 0.5f, "%.2f");
        }

        // Zoom settings
        if (ImGui::CollapsingHeader("Zoom Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Zoom Speed", &g_app.zoomSpeed, 5.0f, 50.0f);
            ImGui::DragFloatRange2("Zoom Range", &g_app.minZoom, &g_app.maxZoom, 1.0f, 5.0f, 1000.0f);
        }

        ImGui::Separator();

        // Current mode display
        const char* presetNames[] = {"Free","Overview","Ground","Cinematic"};
        ImGui::Text("Current Mode: %s", presetNames[(int)g_app.currentPreset]);

        if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) {
            ImGui::TextColored(ImVec4(0.3f,0.9f,0.3f,1), "Following Creature #%d", g_app.followCreatureId);
        }

        // Preset buttons
        ImGui::Text("Presets:");
        if (ImGui::Button("Overview [1]", ImVec2(78, 22))) {
            g_app.currentPreset = AppState::CameraPreset::OVERVIEW;
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            StartCameraTransition(glm::vec3(0, 300, 50), glm::vec3(0), 1.5f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Ground [2]", ImVec2(78, 22))) {
            g_app.currentPreset = AppState::CameraPreset::GROUND;
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            StartCameraTransition(
                glm::vec3(g_app.cameraTarget.x, 5, g_app.cameraTarget.z + 30),
                glm::vec3(g_app.cameraTarget.x, 3, g_app.cameraTarget.z), 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cine [3]", ImVec2(78, 22))) {
            g_app.currentPreset = AppState::CameraPreset::CINEMATIC;
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            g_app.cinematicPlaying = true;
            g_app.cinematicTime = 0;
        }
    }
    ImGui::End();
}
