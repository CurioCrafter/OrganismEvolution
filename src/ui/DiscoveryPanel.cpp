#include "DiscoveryPanel.h"
#include <imgui.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace ui {

// ============================================================================
// DiscoveryPanel Implementation
// ============================================================================

DiscoveryPanel::DiscoveryPanel() = default;

void DiscoveryPanel::initialize(discovery::SpeciesCatalog* catalog, discovery::ScanningSystem* scanner) {
    m_catalog = catalog;
    m_scanner = scanner;
    m_initialized = true;

    // Set up event callback to receive notifications
    if (m_catalog) {
        m_catalog->setEventCallback([this](const discovery::DiscoveryEvent& event) {
            addNotification(event);
        });
    }
}

void DiscoveryPanel::shutdown() {
    if (m_catalog) {
        m_catalog->setEventCallback(nullptr);
    }
    m_initialized = false;
    m_catalog = nullptr;
    m_scanner = nullptr;
}

void DiscoveryPanel::update(float deltaTime) {
    if (!m_initialized) return;

    // Update animation states
    m_scanPulse += deltaTime * m_scanner->getUIStyle().pulseSpeed;
    if (m_scanPulse > 6.28318f) m_scanPulse -= 6.28318f;

    m_scanRotation += deltaTime * m_scanner->getUIStyle().rotationSpeed;
    if (m_scanRotation > 360.0f) m_scanRotation -= 360.0f;

    // Update notifications
    for (auto& notif : m_notifications) {
        notif.timeRemaining -= deltaTime;
        if (notif.timeRemaining < 1.0f) {
            notif.fadeProgress = 1.0f - notif.timeRemaining;
        }
    }

    // Remove expired notifications
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
            [](const DiscoveryNotification& n) { return n.timeRemaining <= 0.0f || n.dismissed; }),
        m_notifications.end());
}

void DiscoveryPanel::render(float screenWidth, float screenHeight) {
    if (!m_initialized) return;

    // Always render scan HUD overlay
    if (m_config.showScanHUD) {
        renderScanHUD(screenWidth, screenHeight);
    }

    // Always render mini progress in corner
    if (m_config.showMiniProgress && !m_visible) {
        renderMiniProgress(screenWidth, screenHeight);
    }

    // Always render notifications
    if (m_config.showNotifications) {
        renderNotifications(screenWidth, screenHeight);
    }

    // Render main catalog panel if visible
    if (m_visible) {
        renderCatalogPanel(screenWidth, screenHeight);
    }
}

void DiscoveryPanel::renderScanHUD(float screenWidth, float screenHeight) {
    if (!m_scanner || !m_scanner->isScanning()) return;

    // Get all visible targets and render indicators
    const auto& targets = m_scanner->getVisibleTargets();
    const auto* currentTarget = m_scanner->getCurrentTarget();

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Render indicators for all visible targets
    for (const auto& target : targets) {
        if (!target.isInView) continue;

        bool isCurrent = (currentTarget && currentTarget->creatureId == target.creatureId);

        // Get rarity color
        glm::vec3 color = isCurrent
            ? m_scanner->getUIStyle().scanningColor
            : glm::vec3(0.5f, 0.5f, 0.5f);

        if (target.discoveryState != discovery::DiscoveryState::COMPLETE) {
            color = discovery::rarityToColor(target.rarity);
        } else {
            color = glm::vec3(0.3f, 0.3f, 0.3f);  // Dimmed for already discovered
        }

        float size = isCurrent ? 24.0f : 12.0f;
        float alpha = isCurrent ? 1.0f : 0.6f;

        ImU32 imColor = IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(alpha * 255));

        if (isCurrent) {
            // Animated targeting reticle for current target
            float pulse = (std::sin(m_scanPulse) + 1.0f) * 0.5f;
            float outerSize = size + pulse * 8.0f;

            // Outer rotating brackets
            float angle = glm::radians(m_scanRotation);
            for (int i = 0; i < 4; ++i) {
                float a = angle + i * 1.5708f;  // 90 degrees apart
                float x1 = target.screenX + std::cos(a) * outerSize;
                float y1 = target.screenY + std::sin(a) * outerSize;
                float x2 = target.screenX + std::cos(a + 0.3f) * (outerSize - 4.0f);
                float y2 = target.screenY + std::sin(a + 0.3f) * (outerSize - 4.0f);
                drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), imColor, 2.0f);
            }

            // Inner crosshair
            drawList->AddLine(
                ImVec2(target.screenX - size * 0.5f, target.screenY),
                ImVec2(target.screenX + size * 0.5f, target.screenY),
                imColor, 1.5f);
            drawList->AddLine(
                ImVec2(target.screenX, target.screenY - size * 0.5f),
                ImVec2(target.screenX, target.screenY + size * 0.5f),
                imColor, 1.5f);

            // Progress arc
            if (target.scanProgress < 1.0f && target.discoveryState != discovery::DiscoveryState::COMPLETE) {
                float progressAngle = target.scanProgress * 6.28318f;
                int segments = static_cast<int>(progressAngle * 10);
                for (int i = 0; i < segments; ++i) {
                    float a1 = -1.5708f + (i / static_cast<float>(segments)) * progressAngle;
                    float a2 = -1.5708f + ((i + 1) / static_cast<float>(segments)) * progressAngle;
                    drawList->AddLine(
                        ImVec2(target.screenX + std::cos(a1) * (size + 8.0f),
                               target.screenY + std::sin(a1) * (size + 8.0f)),
                        ImVec2(target.screenX + std::cos(a2) * (size + 8.0f),
                               target.screenY + std::sin(a2) * (size + 8.0f)),
                        IM_COL32(100, 255, 100, 200), 3.0f);
                }
            }

            // Species name and distance
            std::string label = target.displayName;
            if (m_scanner->getUIStyle().showDistance) {
                label += " (" + std::to_string(static_cast<int>(target.distance)) + "m)";
            }

            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            float labelX = target.screenX - textSize.x * 0.5f;
            float labelY = target.screenY + outerSize + 8.0f;

            // Background for text
            drawList->AddRectFilled(
                ImVec2(labelX - 4.0f, labelY - 2.0f),
                ImVec2(labelX + textSize.x + 4.0f, labelY + textSize.y + 2.0f),
                IM_COL32(0, 0, 0, 180), 4.0f);

            drawList->AddText(ImVec2(labelX, labelY), imColor, label.c_str());

            // Rarity indicator
            if (m_scanner->getUIStyle().showRarity && target.rarity >= discovery::RarityTier::UNCOMMON) {
                std::string rarityStr = discovery::rarityToString(target.rarity);
                glm::vec3 rarityColor = discovery::rarityToColor(target.rarity);
                ImU32 rarityImColor = IM_COL32(
                    static_cast<int>(rarityColor.r * 255),
                    static_cast<int>(rarityColor.g * 255),
                    static_cast<int>(rarityColor.b * 255), 255);

                ImVec2 raritySize = ImGui::CalcTextSize(rarityStr.c_str());
                float rarityY = labelY + textSize.y + 4.0f;
                drawList->AddText(
                    ImVec2(target.screenX - raritySize.x * 0.5f, rarityY),
                    rarityImColor, rarityStr.c_str());
            }

        } else {
            // Simple diamond marker for non-current targets
            drawList->AddQuadFilled(
                ImVec2(target.screenX, target.screenY - size),
                ImVec2(target.screenX + size, target.screenY),
                ImVec2(target.screenX, target.screenY + size),
                ImVec2(target.screenX - size, target.screenY),
                imColor);
        }
    }
}

void DiscoveryPanel::renderMiniProgress(float screenWidth, float screenHeight) {
    if (!m_scanner->hasLockedTarget()) return;

    const auto* target = m_scanner->getCurrentTarget();
    if (!target) return;

    ImGui::SetNextWindowPos(ImVec2(screenWidth - 200.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(190.0f, 80.0f));
    ImGui::SetNextWindowBgAlpha(0.8f);

    if (ImGui::Begin("##ScanMini", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar)) {

        ImGui::Text("SCANNING");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", target->displayName.c_str());

        float progress = target->scanProgress;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 16.0f), "");

        // Distance
        ImGui::Text("%.0fm", target->distance);
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4(discovery::rarityToColor(target->rarity).r,
                   discovery::rarityToColor(target->rarity).g,
                   discovery::rarityToColor(target->rarity).b, 1.0f),
            "%s", discovery::rarityToString(target->rarity));
    }
    ImGui::End();
}

void DiscoveryPanel::renderNotifications(float screenWidth, float screenHeight) {
    float y = screenHeight * 0.1f;

    for (const auto& notif : m_notifications) {
        float alpha = 1.0f - notif.fadeProgress;

        ImGui::SetNextWindowPos(ImVec2(screenWidth * 0.5f - 150.0f, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.9f * alpha);

        std::string windowId = "##Notif" + std::to_string(reinterpret_cast<uintptr_t>(&notif));
        if (ImGui::Begin(windowId.c_str(), nullptr,
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_AlwaysAutoResize)) {

            // Icon based on event type
            const char* icon = "?";
            ImVec4 iconColor = ImVec4(1.0f, 1.0f, 1.0f, alpha);

            switch (notif.event.type) {
                case discovery::DiscoveryEvent::Type::SPECIES_DISCOVERED:
                    icon = "*";
                    iconColor = ImVec4(0.4f, 1.0f, 0.4f, alpha);
                    break;
                case discovery::DiscoveryEvent::Type::SPECIES_DETECTED:
                    icon = "!";
                    iconColor = ImVec4(0.4f, 0.8f, 1.0f, alpha);
                    break;
                case discovery::DiscoveryEvent::Type::RARITY_FOUND:
                    icon = "***";
                    glm::vec3 rc = discovery::rarityToColor(notif.event.rarity);
                    iconColor = ImVec4(rc.r, rc.g, rc.b, alpha);
                    break;
            }

            ImGui::TextColored(iconColor, "%s", icon);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, alpha), "%s", notif.event.message.c_str());
        }
        ImGui::End();

        y += 45.0f;
    }
}

void DiscoveryPanel::renderCatalogPanel(float screenWidth, float screenHeight) {
    float panelX = screenWidth * m_config.panelX;
    float panelY = screenHeight * m_config.panelY;
    float panelW = screenWidth * m_config.panelWidth;
    float panelH = screenHeight * m_config.panelHeight;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(m_config.opacity);

    if (ImGui::Begin("Species Catalog", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        // Statistics header
        const auto& stats = m_catalog->getStatistics();
        ImGui::Text("Discovered: %u | Sightings: %u", stats.speciesDiscovered, stats.totalSightings);

        ImGui::Separator();

        // View mode tabs
        if (ImGui::BeginTabBar("CatalogTabs")) {
            if (ImGui::BeginTabItem("Recent")) {
                m_viewMode = DiscoveryViewMode::RECENT;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Catalog")) {
                m_viewMode = DiscoveryViewMode::CATALOG;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("By Rarity")) {
                m_viewMode = DiscoveryViewMode::RARITY;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        // Rarity filter for rarity view
        if (m_viewMode == DiscoveryViewMode::RARITY) {
            ImGui::PushItemWidth(150.0f);
            const char* rarities[] = {"Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythical"};
            int currentRarity = static_cast<int>(m_rarityFilter);
            if (ImGui::Combo("Filter", &currentRarity, rarities, 6)) {
                m_rarityFilter = static_cast<discovery::RarityTier>(currentRarity);
            }
            ImGui::PopItemWidth();
        }

        ImGui::Separator();

        // Species list
        auto entries = getFilteredEntries();

        if (entries.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No species discovered yet.");
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Look at creatures to scan them!");
        } else {
            ImGui::BeginChild("SpeciesList", ImVec2(0.0f, panelH * 0.5f), true);

            for (const auto* entry : entries) {
                bool isSelected = (m_selectedSpeciesId == entry->speciesId);
                renderSpeciesListEntry(*entry, isSelected);
            }

            ImGui::EndChild();
        }

        // Selected species details
        if (m_selectedSpeciesId != 0) {
            ImGui::Separator();
            const auto* selected = getSelectedEntry();
            if (selected) {
                renderSpeciesDetails(*selected);
            }
        }
    }
    ImGui::End();
}

void DiscoveryPanel::renderSpeciesListEntry(const discovery::SpeciesDiscoveryEntry& entry, bool isSelected) {
    ImGui::PushID(static_cast<int>(entry.speciesId));

    // Rarity color for selection highlight
    glm::vec3 rarityCol = discovery::rarityToColor(entry.rarity);
    ImVec4 headerColor = isSelected
        ? ImVec4(rarityCol.r * 0.5f, rarityCol.g * 0.5f, rarityCol.b * 0.5f, 0.5f)
        : ImVec4(0.2f, 0.2f, 0.2f, 0.3f);

    ImGui::PushStyleColor(ImGuiCol_Header, headerColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(rarityCol.r * 0.6f, rarityCol.g * 0.6f, rarityCol.b * 0.6f, 0.6f));

    bool clicked = ImGui::Selectable("##Entry", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, 50.0f));

    if (clicked) {
        selectSpecies(entry.speciesId);
    }

    ImGui::PopStyleColor(2);

    // Entry content (overlaid on selectable)
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
    ImGui::BeginGroup();

    // Color swatch
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 swatchMin = ImVec2(windowPos.x + cursorPos.x, windowPos.y + cursorPos.y);
    ImVec2 swatchMax = ImVec2(swatchMin.x + 32.0f, swatchMin.y + 32.0f);

    ImU32 swatchColor = IM_COL32(
        static_cast<int>(entry.primaryColor.r * 255),
        static_cast<int>(entry.primaryColor.g * 255),
        static_cast<int>(entry.primaryColor.b * 255), 255);
    drawList->AddRectFilled(swatchMin, swatchMax, swatchColor, 4.0f);

    // Rarity border
    ImU32 borderColor = IM_COL32(
        static_cast<int>(rarityCol.r * 255),
        static_cast<int>(rarityCol.g * 255),
        static_cast<int>(rarityCol.b * 255), 255);
    drawList->AddRect(swatchMin, swatchMax, borderColor, 4.0f, 0, 2.0f);

    ImGui::SetCursorPosX(cursorPos.x + 40.0f);

    // Name
    std::string displayName = entry.commonName.empty() ? "???" : entry.commonName;
    if (entry.discoveryState != discovery::DiscoveryState::COMPLETE) {
        displayName = "? " + displayName;
    }
    ImGui::Text("%s", displayName.c_str());

    // Rarity badge
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(rarityCol.r, rarityCol.g, rarityCol.b, 1.0f),
                       "[%s]", discovery::rarityToString(entry.rarity));

    // Progress or sample count
    ImGui::SetCursorPosX(cursorPos.x + 40.0f);
    if (entry.discoveryState == discovery::DiscoveryState::COMPLETE) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Samples: %u", entry.sampleCount);
    } else {
        float progress = entry.getDiscoveryProgress();
        ImGui::ProgressBar(progress, ImVec2(100.0f, 12.0f), "");
    }

    ImGui::EndGroup();

    ImGui::PopID();
}

void DiscoveryPanel::renderSpeciesDetails(const discovery::SpeciesDiscoveryEntry& entry) {
    ImGui::BeginChild("SpeciesDetails", ImVec2(0.0f, 0.0f), true);

    // Header
    glm::vec3 rarityCol = discovery::rarityToColor(entry.rarity);
    ImGui::TextColored(ImVec4(rarityCol.r, rarityCol.g, rarityCol.b, 1.0f), "%s", entry.commonName.c_str());

    if (!entry.scientificName.empty() && entry.traitsUnlocked[1]) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(%s)", entry.scientificName.c_str());
    }

    ImGui::Separator();

    // Trait sections (unlocked progressively)
    if (entry.traitsUnlocked[0]) {
        ImGui::Text("Type: %s", getCreatureTypeName(entry.creatureType));

        ImVec2 colorBoxMin = ImGui::GetCursorScreenPos();
        ImVec2 colorBoxMax = ImVec2(colorBoxMin.x + 20.0f, colorBoxMin.y + 20.0f);
        ImGui::GetWindowDrawList()->AddRectFilled(colorBoxMin, colorBoxMax,
            IM_COL32(static_cast<int>(entry.primaryColor.r * 255),
                     static_cast<int>(entry.primaryColor.g * 255),
                     static_cast<int>(entry.primaryColor.b * 255), 255));
        ImGui::Dummy(ImVec2(24.0f, 20.0f));
        ImGui::SameLine();
        ImGui::Text("Primary Color");
    }

    if (entry.traitsUnlocked[1]) {
        ImGui::Text("Avg Size: %.2f", entry.averageSize);
        ImGui::Text("Avg Speed: %.1f", entry.averageSpeed);
    }

    if (entry.traitsUnlocked[2]) {
        if (isFlying(entry.creatureType)) {
            ImGui::Text("Locomotion: Flying");
        } else if (isAquatic(entry.creatureType)) {
            ImGui::Text("Locomotion: Swimming");
        } else {
            ImGui::Text("Locomotion: Walking");
        }
    }

    if (entry.traitsUnlocked[3]) {
        ImGui::Text("Rarity: %s", discovery::rarityToString(entry.rarity));
        ImGui::Text("Habitats: %zu biome(s)", entry.habitatBiomes.size());
    }

    if (entry.traitsUnlocked[4]) {
        ImGui::Text("Generations Observed: %u", entry.generationsObserved);
        ImGui::Text("Total Samples: %u", entry.sampleCount);
    }

    // Locked trait indicators
    int unlockedCount = entry.getUnlockedTraitCount();
    if (unlockedCount < 5) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Scan progress: %d/5 traits unlocked", unlockedCount);
    }

    // User notes
    if (!entry.userNotes.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("Notes: %s", entry.userNotes.c_str());
    }

    // First seen
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
        "First seen: %s", formatTimestamp(entry.firstSeenTimestamp).c_str());

    ImGui::EndChild();
}

std::vector<const discovery::SpeciesDiscoveryEntry*> DiscoveryPanel::getFilteredEntries() const {
    std::vector<const discovery::SpeciesDiscoveryEntry*> result;

    switch (m_viewMode) {
        case DiscoveryViewMode::RECENT:
            result = m_catalog->getRecentDiscoveries(20);
            break;

        case DiscoveryViewMode::CATALOG:
            for (const auto& [id, entry] : m_catalog->getAllEntries()) {
                if (entry.discoveryState >= discovery::DiscoveryState::DETECTED) {
                    result.push_back(&entry);
                }
            }
            // Sort by name
            std::sort(result.begin(), result.end(),
                [](const discovery::SpeciesDiscoveryEntry* a, const discovery::SpeciesDiscoveryEntry* b) {
                    return a->commonName < b->commonName;
                });
            break;

        case DiscoveryViewMode::RARITY:
            result = m_catalog->getEntriesByRarity(m_rarityFilter);
            break;

        case DiscoveryViewMode::BIOME:
            result = m_catalog->getEntriesByBiome(m_biomeFilter);
            break;

        default:
            break;
    }

    // Apply search filter if set
    if (!m_searchQuery.empty()) {
        std::vector<const discovery::SpeciesDiscoveryEntry*> filtered;
        for (const auto* entry : result) {
            if (entry->commonName.find(m_searchQuery) != std::string::npos ||
                entry->scientificName.find(m_searchQuery) != std::string::npos) {
                filtered.push_back(entry);
            }
        }
        result = std::move(filtered);
    }

    return result;
}

void DiscoveryPanel::setViewMode(DiscoveryViewMode mode) {
    m_viewMode = mode;
}

void DiscoveryPanel::clearFilters() {
    m_rarityFilter = discovery::RarityTier::COMMON;
    m_biomeFilter = BiomeType::GRASSLAND;
    m_searchQuery.clear();
}

void DiscoveryPanel::selectSpecies(uint32_t speciesId) {
    m_selectedSpeciesId = speciesId;
    if (m_speciesSelectedCallback) {
        m_speciesSelectedCallback(speciesId);
    }
}

const discovery::SpeciesDiscoveryEntry* DiscoveryPanel::getSelectedEntry() const {
    return m_catalog->getEntry(m_selectedSpeciesId);
}

void DiscoveryPanel::addNotification(const discovery::DiscoveryEvent& event) {
    DiscoveryNotification notif;
    notif.event = event;
    notif.timeRemaining = m_config.notificationDuration;
    notif.fadeProgress = 0.0f;

    m_notifications.push_back(notif);

    // Limit notifications
    while (m_notifications.size() > MAX_NOTIFICATIONS) {
        m_notifications.erase(m_notifications.begin());
    }
}

void DiscoveryPanel::clearNotifications() {
    m_notifications.clear();
}

std::string DiscoveryPanel::formatTimestamp(uint64_t timestamp) const {
    if (timestamp == 0) return "Unknown";

    time_t time = static_cast<time_t>(timestamp);
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M");
    return ss.str();
}

std::string DiscoveryPanel::formatDuration(float seconds) const {
    if (seconds < 60.0f) {
        return std::to_string(static_cast<int>(seconds)) + "s";
    } else if (seconds < 3600.0f) {
        int minutes = static_cast<int>(seconds / 60.0f);
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(minutes) + "m " + std::to_string(secs) + "s";
    } else {
        int hours = static_cast<int>(seconds / 3600.0f);
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }
}

// ============================================================================
// DiscoveryHUD Implementation
// ============================================================================

DiscoveryHUD::DiscoveryHUD() = default;

void DiscoveryHUD::initialize(discovery::ScanningSystem* scanner) {
    m_scanner = scanner;
}

void DiscoveryHUD::render(float screenWidth, float screenHeight) {
    if (!m_visible || !m_scanner || !m_scanner->isScanning()) return;

    m_animTime += 0.016f;  // Approximate 60fps

    // The scan HUD rendering is now in DiscoveryPanel::renderScanHUD
    // This class is kept for potential future standalone HUD needs
}

void DiscoveryHUD::renderScanReticle(const discovery::ScanTargetInfo& target, float screenWidth, float screenHeight) {
    // Implementation moved to DiscoveryPanel::renderScanHUD
}

void DiscoveryHUD::renderTargetIndicator(const discovery::ScanTargetInfo& target, float screenWidth, float screenHeight) {
    // Implementation moved to DiscoveryPanel::renderScanHUD
}

void DiscoveryHUD::renderProgressRing(float x, float y, float radius, float progress, const glm::vec3& color) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    ImU32 imColor = IM_COL32(
        static_cast<int>(color.r * 255),
        static_cast<int>(color.g * 255),
        static_cast<int>(color.b * 255), 200);

    float startAngle = -1.5708f;  // -90 degrees (top)
    float endAngle = startAngle + progress * 6.28318f;

    int segments = static_cast<int>(progress * 32);
    if (segments < 2) segments = 2;

    for (int i = 0; i < segments; ++i) {
        float a1 = startAngle + (i / static_cast<float>(segments)) * (endAngle - startAngle);
        float a2 = startAngle + ((i + 1) / static_cast<float>(segments)) * (endAngle - startAngle);

        drawList->AddLine(
            ImVec2(x + std::cos(a1) * radius, y + std::sin(a1) * radius),
            ImVec2(x + std::cos(a2) * radius, y + std::sin(a2) * radius),
            imColor, 3.0f);
    }
}

} // namespace ui
