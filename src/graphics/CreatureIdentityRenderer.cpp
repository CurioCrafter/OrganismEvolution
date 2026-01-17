#include "CreatureIdentityRenderer.h"
#include "../ui/CreatureNametags.h"
#include "../entities/SpeciesNaming.h"
#include "../entities/Creature.h"
#include "TextureAtlas.h"
#include "procedural/CreatureTextureGenerator.h"
#include "Camera.h"
#include <imgui.h>

CreatureIdentityRenderer::CreatureIdentityRenderer() {
    m_nametags = std::make_unique<ui::CreatureNametags>();
    m_textureAtlas = std::make_unique<graphics::TextureAtlasManager>();
}

CreatureIdentityRenderer::~CreatureIdentityRenderer() {
    shutdown();
}

bool CreatureIdentityRenderer::initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
    m_device = device;

    // Initialize nametag system
    if (!m_nametags->initialize(device, commandList)) {
        return false;
    }

    // Initialize texture atlas with default config
    graphics::TextureAtlasConfig atlasConfig;
    atlasConfig.atlasWidth = 2048;
    atlasConfig.atlasHeight = 2048;
    atlasConfig.maxAtlases = 4;
    atlasConfig.textureWidth = 128;
    atlasConfig.textureHeight = 128;

    if (!m_textureAtlas->initialize(device, atlasConfig)) {
        return false;
    }

    m_initialized = true;
    return true;
}

void CreatureIdentityRenderer::shutdown() {
    if (m_nametags) {
        m_nametags->shutdown();
    }
    if (m_textureAtlas) {
        m_textureAtlas->shutdown();
    }
    m_device = nullptr;
    m_initialized = false;
}

void CreatureIdentityRenderer::update(float deltaTime,
                                       const std::vector<std::unique_ptr<Creature>>& creatures,
                                       const Camera& camera,
                                       int selectedCreatureId) {
    if (!m_initialized) return;

    // Convert unique_ptr vector to raw pointer vector for nametag system
    std::vector<Creature*> creaturePtrs;
    creaturePtrs.reserve(creatures.size());
    for (const auto& creature : creatures) {
        if (creature && creature->isAlive()) {
            creaturePtrs.push_back(creature.get());
        }
    }

    // Update nametags
    m_nametags->updateNametags(creaturePtrs, camera, selectedCreatureId);

    // Update texture atlas
    if (m_currentCommandList) {
        m_textureAtlas->update(deltaTime, m_currentCommandList);
    }
}

void CreatureIdentityRenderer::renderNametags(const Camera& camera, const glm::mat4& viewProjection) {
    if (!m_initialized || !m_nametags) return;
    m_nametags->renderImGui(camera, viewProjection);
}

naming::SpeciesNamingSystem& CreatureIdentityRenderer::getNamingSystem() {
    return naming::getNamingSystem();
}

void CreatureIdentityRenderer::setNametagsVisible(bool visible) {
    if (m_nametags) {
        m_nametags->setVisible(visible);
    }
}

bool CreatureIdentityRenderer::getNametagsVisible() const {
    return m_nametags ? m_nametags->isVisible() : false;
}

void CreatureIdentityRenderer::toggleNametags() {
    if (m_nametags) {
        m_nametags->toggleVisibility();
    }
}

int CreatureIdentityRenderer::getCreatureAtScreenPos(const glm::vec2& screenPos) const {
    return m_nametags ? m_nametags->getCreatureAtScreenPos(screenPos) : -1;
}

void CreatureIdentityRenderer::setSelectCallback(std::function<void(int)> callback) {
    if (m_nametags) {
        m_nametags->setSelectCallback(callback);
    }
}

graphics::ColorGenes CreatureIdentityRenderer::getColorGenesFromCreature(const Creature* creature) {
    if (!creature) {
        return graphics::ColorGenes();
    }

    const Genome& genome = creature->getGenome();
    uint32_t speciesId = creature->getSpeciesId();

    return graphics::CreatureTextureGenerator::genesFromGenome(
        genome.color,
        genome.size,
        speciesId
    );
}

void CreatureIdentityRenderer::regenerateCreatureTexture(const Creature* creature,
                                                          ID3D12GraphicsCommandList* commandList) {
    if (!creature || !m_textureAtlas) return;

    graphics::ColorGenes genes = getColorGenesFromCreature(creature);
    m_textureAtlas->regenerateTexture(creature->getId(), genes, commandList);
}

void CreatureIdentityRenderer::renderSettingsPanel() {
    if (ImGui::CollapsingHeader("Creature Identity")) {
        // Nametag settings
        if (m_nametags) {
            m_nametags->renderSettingsPanel();
        }

        // Texture atlas statistics
        if (m_textureAtlas && ImGui::TreeNode("Texture Atlas")) {
            ImGui::Text("Active Textures: %zu", m_textureAtlas->getActiveTextureCount());
            ImGui::Text("Atlas Pages: %zu", m_textureAtlas->getAtlasPageCount());
            ImGui::Text("Usage: %.1f%%", m_textureAtlas->getTotalUsagePercent());

            uint32_t lod = m_textureAtlas->getLODLevel();
            if (ImGui::SliderInt("LOD Level", reinterpret_cast<int*>(&lod), 0, 3)) {
                m_textureAtlas->setLODLevel(lod);
            }

            if (ImGui::Button("Clear All Textures")) {
                m_textureAtlas->clear();
            }

            ImGui::TreePop();
        }

        // Naming system statistics
        if (ImGui::TreeNode("Species Naming")) {
            auto& namingSystem = naming::getNamingSystem();
            const auto& stats = namingSystem.getStats();

            ImGui::Text("Known Species: %zu", namingSystem.getSpeciesCount());
            ImGui::Text("Unique Names: %zu", stats.uniqueNames);
            ImGui::Text("Collisions: %zu", stats.collisions);
            ImGui::Text("Avg Name Length: %.1f", stats.averageNameLength);

            if (stats.totalNamesGenerated > 0) {
                float collisionRate = static_cast<float>(stats.collisions) /
                    static_cast<float>(stats.totalNamesGenerated) * 100.0f;
                ImGui::Text("Collision Rate: %.2f%%", collisionRate);
            }

            ImGui::Separator();

            // Display mode toggle
            static int displayMode = 0;
            if (ImGui::RadioButton("Common Name", displayMode == 0)) {
                displayMode = 0;
                namingSystem.setDisplayMode(naming::NameDisplayMode::COMMON_NAME);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Binomial", displayMode == 1)) {
                displayMode = 1;
                namingSystem.setDisplayMode(naming::NameDisplayMode::BINOMIAL);
            }

            bool showDescriptor = namingSystem.getShowDescriptor();
            if (ImGui::Checkbox("Show Descriptor", &showDescriptor)) {
                namingSystem.setShowDescriptor(showDescriptor);
            }

            ImGui::Separator();

            if (ImGui::Button("Clear Names")) {
                namingSystem.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Log Stats")) {
                namingSystem.logStats();
            }
            ImGui::SameLine();
            if (ImGui::Button("Validate (200 names)")) {
                namingSystem.validateNameGeneration(200, 12345);
            }

            ImGui::TreePop();
        }
    }
}

size_t CreatureIdentityRenderer::getActiveTextureCount() const {
    return m_textureAtlas ? m_textureAtlas->getActiveTextureCount() : 0;
}

size_t CreatureIdentityRenderer::getVisibleNametagCount() const {
    return m_nametags ? m_nametags->getVisibleNametagCount() : 0;
}

// Integration helpers
void CreatureIdentityIntegration::beginFrame(CreatureIdentityRenderer& renderer,
                                              ID3D12GraphicsCommandList* commandList) {
    renderer.m_currentCommandList = commandList;
}

void CreatureIdentityIntegration::updateCreatures(CreatureIdentityRenderer& renderer,
                                                   float deltaTime,
                                                   const std::vector<std::unique_ptr<Creature>>& creatures,
                                                   const Camera& camera,
                                                   int selectedCreatureId) {
    renderer.update(deltaTime, creatures, camera, selectedCreatureId);
}

void CreatureIdentityIntegration::renderUI(CreatureIdentityRenderer& renderer,
                                            const Camera& camera,
                                            const glm::mat4& viewProjection) {
    renderer.renderNametags(camera, viewProjection);
}

void CreatureIdentityIntegration::endFrame(CreatureIdentityRenderer& renderer) {
    renderer.m_currentCommandList = nullptr;
}
