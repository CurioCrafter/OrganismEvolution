#include "ActivityAnimations.h"
#include "IKSolver.h"
#include <algorithm>
#include <cmath>

namespace animation {

// =============================================================================
// BONE POSE KEY IMPLEMENTATION
// =============================================================================

BonePoseKey BonePoseKey::identity(float t) {
    BonePoseKey key;
    key.time = t;
    key.position = glm::vec3(0.0f);
    key.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    key.scale = glm::vec3(1.0f);
    return key;
}

BonePoseKey BonePoseKey::lerp(const BonePoseKey& a, const BonePoseKey& b, float t) {
    BonePoseKey result;
    result.time = glm::mix(a.time, b.time, t);
    result.position = glm::mix(a.position, b.position, t);
    result.rotation = glm::slerp(a.rotation, b.rotation, t);
    result.scale = glm::mix(a.scale, b.scale, t);
    return result;
}

// =============================================================================
// BONE CHANNEL IMPLEMENTATION
// =============================================================================

BonePoseKey BoneChannel::sample(float time) const {
    if (keys.empty()) {
        return BonePoseKey::identity(time);
    }

    if (keys.size() == 1) {
        BonePoseKey result = keys[0];
        result.time = time;
        return result;
    }

    // Find surrounding keyframes
    size_t nextIdx = 0;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i].time > time) {
            nextIdx = i;
            break;
        }
        nextIdx = i + 1;
    }

    if (nextIdx == 0) {
        BonePoseKey result = keys[0];
        result.time = time;
        return result;
    }

    if (nextIdx >= keys.size()) {
        BonePoseKey result = keys.back();
        result.time = time;
        return result;
    }

    const BonePoseKey& prev = keys[nextIdx - 1];
    const BonePoseKey& next = keys[nextIdx];

    float t = (time - prev.time) / (next.time - prev.time);
    return BonePoseKey::lerp(prev, next, t);
}

void BoneChannel::addKey(const BonePoseKey& key) {
    // Insert sorted by time
    auto it = std::lower_bound(keys.begin(), keys.end(), key,
        [](const BonePoseKey& a, const BonePoseKey& b) { return a.time < b.time; });
    keys.insert(it, key);
}

float BoneChannel::getDuration() const {
    if (keys.empty()) return 0.0f;
    return keys.back().time;
}

// =============================================================================
// ACTIVITY MOTION CLIP IMPLEMENTATION
// =============================================================================

void ActivityMotionClip::samplePose(float time, SkeletonPose& outPose, const Skeleton& skeleton) const {
    // Handle looping
    float sampleTime = time;
    if (isLooping && duration > 0.0f) {
        sampleTime = std::fmod(time, duration);
    }

    for (const auto& channel : boneChannels) {
        int32_t boneIdx = channel.boneIndex;
        if (boneIdx < 0) {
            // Try to resolve by name
            boneIdx = skeleton.findBoneIndex(channel.boneName);
            if (boneIdx < 0) continue;
        }

        BonePoseKey pose = channel.sample(sampleTime);

        // Get blend weight
        float weight = 1.0f;
        if (!blendMask.empty() && static_cast<size_t>(boneIdx) < blendMask.size()) {
            weight = blendMask[boneIdx];
        }

        if (weight > 0.0f) {
            // Apply to pose (blend with existing)
            BoneTransform& transform = outPose.getLocalTransform(boneIdx);

            if (isAdditive) {
                // Additive blend
                transform.translation += pose.position * weight * additiveWeight;
                transform.rotation = glm::slerp(
                    transform.rotation,
                    transform.rotation * pose.rotation,
                    weight * additiveWeight
                );
            } else {
                // Replace blend
                transform.translation = glm::mix(transform.translation, pose.position, weight);
                transform.rotation = glm::slerp(transform.rotation, pose.rotation, weight);
                transform.scale = glm::mix(transform.scale, pose.scale, weight);
            }
        }
    }
}

BonePoseKey ActivityMotionClip::sampleRootMotion(float time) const {
    if (!hasRootMotion || rootMotionKeys.empty()) {
        return BonePoseKey::identity(time);
    }

    // Sample root motion channel
    float sampleTime = time;
    if (isLooping && duration > 0.0f) {
        sampleTime = std::fmod(time, duration);
    }

    // Find surrounding keys
    size_t nextIdx = 0;
    for (size_t i = 0; i < rootMotionKeys.size(); ++i) {
        if (rootMotionKeys[i].time > sampleTime) {
            nextIdx = i;
            break;
        }
        nextIdx = i + 1;
    }

    if (nextIdx == 0) return rootMotionKeys[0];
    if (nextIdx >= rootMotionKeys.size()) return rootMotionKeys.back();

    const BonePoseKey& prev = rootMotionKeys[nextIdx - 1];
    const BonePoseKey& next = rootMotionKeys[nextIdx];

    float t = (sampleTime - prev.time) / (next.time - prev.time);
    return BonePoseKey::lerp(prev, next, t);
}

// =============================================================================
// SECONDARY MOTION LAYER IMPLEMENTATION
// =============================================================================

void SecondaryMotionLayer::setMorphology(const MorphologyGenes& genes) {
    m_hasTail = genes.hasTail;
    m_tailSegments = genes.tailSegments;
    m_hasEars = true; // Assume ears unless specified

    // Initialize tail rotations
    m_tailRotations.resize(m_tailSegments);
    for (auto& rot : m_tailRotations) {
        rot = glm::quat(1, 0, 0, 0);
    }
}

void SecondaryMotionLayer::update(float deltaTime) {
    m_time += deltaTime;

    // Activity-based modifiers
    float breathingMod = 1.0f;
    float tailMod = 1.0f;
    float swayMod = 1.0f;

    switch (m_currentActivity) {
        case ActivityType::SLEEPING:
            breathingMod = 0.5f;    // Slower breathing
            tailMod = 0.1f;         // Minimal tail movement
            swayMod = 0.0f;         // No sway
            break;
        case ActivityType::THREAT_DISPLAY:
            breathingMod = 1.5f;    // Heavier breathing
            tailMod = 0.3f;         // Stiff tail
            swayMod = 0.2f;
            break;
        case ActivityType::PLAYING:
            breathingMod = 1.3f;
            tailMod = 1.5f;         // Excited tail wagging
            swayMod = 0.5f;
            break;
        default:
            break;
    }

    // Movement speed affects secondary motion
    float speedMod = 1.0f - std::min(1.0f, m_movementSpeed * 0.5f);

    // Breathing
    if (m_config.enableBreathing) {
        m_breathPhase += deltaTime * m_config.breathingRate * 2.0f * 3.14159f * breathingMod;
        float breathAmount = std::sin(m_breathPhase) * 0.5f + 0.5f;
        m_breathingOffset = glm::vec3(0.0f, breathAmount * m_config.breathingAmplitude, 0.0f);
    }

    // Tail motion
    if (m_config.enableTailMotion && m_hasTail) {
        m_tailPhase += deltaTime * m_config.tailWagSpeed * tailMod;
        float wagBase = std::sin(m_tailPhase * 2.0f * 3.14159f);

        for (int i = 0; i < m_tailSegments; ++i) {
            float segmentPhase = m_tailPhase - i * 0.2f; // Phase delay along tail
            float wagAmount = std::sin(segmentPhase * 2.0f * 3.14159f);
            wagAmount *= m_config.tailWagAmplitude * (1.0f - i * 0.1f); // Decreasing amplitude

            // Add gravity droop
            float droop = m_config.tailDroop * (i + 1) * 0.1f;

            m_tailRotations[i] = glm::angleAxis(wagAmount, glm::vec3(0, 1, 0)) *
                                glm::angleAxis(droop, glm::vec3(1, 0, 0));
        }
    }

    // Head bob (when idle/slow)
    if (m_config.enableHeadBob) {
        float bobPhase = m_time * m_config.headBobSpeed * 2.0f * 3.14159f;
        float bobAmount = std::sin(bobPhase) * m_config.headBobAmplitude * speedMod;
        m_headBobOffset = glm::vec3(0.0f, bobAmount, 0.0f);
    }

    // Blinking
    if (m_config.enableBlinking) {
        if (!m_isBlinking && m_time >= m_nextBlinkTime) {
            m_isBlinking = true;
            m_blinkAmount = 0.0f;
        }

        if (m_isBlinking) {
            m_blinkAmount += deltaTime / (m_config.blinkDuration * 0.5f);
            if (m_blinkAmount >= 2.0f) {
                m_isBlinking = false;
                m_blinkAmount = 0.0f;
                // Schedule next blink
                float variance = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
                m_nextBlinkTime = m_time + (1.0f / m_config.blinkRate) * (1.0f + variance);
            }
        }
    }

    // Body sway
    if (m_config.enableBodySway) {
        float swayPhaseX = m_time * m_config.swaySpeed * 2.0f * 3.14159f;
        float swayPhaseZ = m_time * m_config.swaySpeed * 0.7f * 2.0f * 3.14159f;
        m_swayOffset = glm::vec3(
            std::sin(swayPhaseX) * m_config.swayAmplitude * swayMod * speedMod,
            0.0f,
            std::sin(swayPhaseZ) * m_config.swayAmplitude * 0.5f * swayMod * speedMod
        );
    }

    // Ear/antenna twitch
    if (m_config.enableEarTwitch && m_hasEars) {
        // Random twitch based on arousal
        if (static_cast<float>(rand()) / RAND_MAX < deltaTime * m_config.twitchRate * m_arousalLevel) {
            m_earTwitchAmount = m_config.twitchAmplitude;
        }
        m_earTwitchAmount *= 0.9f; // Decay
    }
}

void SecondaryMotionLayer::applyToPose(SkeletonPose& pose, const Skeleton& skeleton) {
    // Apply breathing to chest/spine
    int32_t chestBone = skeleton.findBoneIndex("chest");
    if (chestBone < 0) chestBone = skeleton.findBoneIndex("spine_2");
    if (chestBone >= 0) {
        BoneTransform& transform = pose.getLocalTransform(chestBone);
        transform.translation += m_breathingOffset;
    }

    // Apply tail rotations
    if (m_hasTail) {
        for (int i = 0; i < m_tailSegments; ++i) {
            std::string boneName = "tail_" + std::to_string(i);
            int32_t tailBone = skeleton.findBoneIndex(boneName);
            if (tailBone >= 0) {
                BoneTransform& transform = pose.getLocalTransform(tailBone);
                transform.rotation = transform.rotation * m_tailRotations[i];
            }
        }
    }

    // Apply head bob
    int32_t headBone = skeleton.findBoneIndex("head");
    if (headBone >= 0) {
        BoneTransform& transform = pose.getLocalTransform(headBone);
        transform.translation += m_headBobOffset;
    }

    // Apply body sway to root/pelvis
    int32_t pelvisBone = skeleton.findBoneIndex("pelvis");
    if (pelvisBone >= 0) {
        BoneTransform& transform = pose.getLocalTransform(pelvisBone);
        transform.translation += m_swayOffset;
    }
}

glm::quat SecondaryMotionLayer::getTailRotation(int segment) const {
    if (segment >= 0 && segment < static_cast<int>(m_tailRotations.size())) {
        return m_tailRotations[segment];
    }
    return glm::quat(1, 0, 0, 0);
}

void SecondaryMotionLayer::setActivityState(ActivityType activity) {
    m_currentActivity = activity;
}

void SecondaryMotionLayer::setMovementSpeed(float speed) {
    m_movementSpeed = speed;
}

void SecondaryMotionLayer::setArousalLevel(float arousal) {
    m_arousalLevel = glm::clamp(arousal, 0.0f, 1.0f);
}

// =============================================================================
// ACTIVITY MOTION GENERATOR IMPLEMENTATION
// =============================================================================

void ActivityMotionGenerator::setMorphology(const MorphologyGenes& genes) {
    m_genes = genes;
}

void ActivityMotionGenerator::setRigDefinition(const RigDefinition& rig) {
    m_rig = rig;
}

void ActivityMotionGenerator::generateAllClips() {
    m_clips.clear();
    m_clips.resize(static_cast<size_t>(ActivityType::COUNT));

    m_clips[static_cast<size_t>(ActivityType::IDLE)] = generateIdleClip();
    m_clips[static_cast<size_t>(ActivityType::EATING)] = generateEatingClip();
    m_clips[static_cast<size_t>(ActivityType::DRINKING)] = generateDrinkingClip();
    m_clips[static_cast<size_t>(ActivityType::MATING)] = generateMatingClip();
    m_clips[static_cast<size_t>(ActivityType::SLEEPING)] = generateSleepingClip();
    m_clips[static_cast<size_t>(ActivityType::EXCRETING)] = generateExcretingClip(ExcretionType::URINATE);
    m_clips[static_cast<size_t>(ActivityType::GROOMING)] = generateGroomingClip(GroomingType::STRETCH);
    m_clips[static_cast<size_t>(ActivityType::THREAT_DISPLAY)] = generateThreatDisplayClip();
    m_clips[static_cast<size_t>(ActivityType::SUBMISSIVE_DISPLAY)] = generateSubmissiveClip();
    m_clips[static_cast<size_t>(ActivityType::MATING_DISPLAY)] = generateMatingDisplayClip();
    m_clips[static_cast<size_t>(ActivityType::PLAYING)] = generatePlayingClip();
    m_clips[static_cast<size_t>(ActivityType::INVESTIGATING)] = generateInvestigatingClip();
    m_clips[static_cast<size_t>(ActivityType::CALLING)] = generateCallingClip();
}

ActivityMotionClip ActivityMotionGenerator::generateClip(ActivityType type) const {
    switch (type) {
        case ActivityType::IDLE: return generateIdleClip();
        case ActivityType::EATING: return generateEatingClip();
        case ActivityType::DRINKING: return generateDrinkingClip();
        case ActivityType::MATING: return generateMatingClip();
        case ActivityType::SLEEPING: return generateSleepingClip();
        case ActivityType::EXCRETING: return generateExcretingClip(ExcretionType::URINATE);
        case ActivityType::GROOMING: return generateGroomingClip(GroomingType::STRETCH);
        case ActivityType::THREAT_DISPLAY: return generateThreatDisplayClip();
        case ActivityType::SUBMISSIVE_DISPLAY: return generateSubmissiveClip();
        case ActivityType::MATING_DISPLAY: return generateMatingDisplayClip();
        case ActivityType::PLAYING: return generatePlayingClip();
        case ActivityType::INVESTIGATING: return generateInvestigatingClip();
        case ActivityType::CALLING: return generateCallingClip();
        default: return generateIdleClip();
    }
}

const ActivityMotionClip* ActivityMotionGenerator::getClip(ActivityType type) const {
    size_t idx = static_cast<size_t>(type);
    if (idx < m_clips.size()) {
        return &m_clips[idx];
    }
    return nullptr;
}

ActivityMotionClip ActivityMotionGenerator::generateIdleClip() const {
    ActivityMotionClip clip;
    clip.name = "idle";
    clip.activityType = ActivityType::IDLE;
    clip.duration = 2.0f;
    clip.isLooping = true;

    // Subtle spine movement
    BoneChannel spineChannel;
    spineChannel.boneName = "spine_1";
    addSpineWave(spineChannel, 0.02f, 0.5f, clip.duration);
    clip.boneChannels.push_back(spineChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateEatingClip() const {
    ActivityMotionClip clip;
    clip.name = "eating";
    clip.activityType = ActivityType::EATING;
    clip.duration = 3.0f;
    clip.isLooping = true;

    // Head bobbing for eating
    BoneChannel headChannel;
    headChannel.boneName = "head";
    addHeadBob(headChannel, 0.1f, 2.0f, clip.duration);
    clip.boneChannels.push_back(headChannel);

    // Body lowered
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";
    addBodySquat(pelvisChannel, 0.05f, clip.duration * 0.8f, clip.duration);
    clip.boneChannels.push_back(pelvisChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateDrinkingClip() const {
    ActivityMotionClip clip;
    clip.name = "drinking";
    clip.activityType = ActivityType::DRINKING;
    clip.duration = 2.5f;
    clip.isLooping = true;

    // Head down for drinking
    BoneChannel headChannel;
    headChannel.boneName = "head";

    BonePoseKey startKey = BonePoseKey::identity(0.0f);
    BonePoseKey downKey = BonePoseKey::identity(0.3f);
    downKey.rotation = glm::angleAxis(0.4f, glm::vec3(1, 0, 0));
    BonePoseKey lapKey = BonePoseKey::identity(0.5f);
    lapKey.rotation = glm::angleAxis(0.5f, glm::vec3(1, 0, 0));
    BonePoseKey upKey = BonePoseKey::identity(0.8f);
    upKey.rotation = glm::angleAxis(0.3f, glm::vec3(1, 0, 0));

    headChannel.addKey(startKey);
    headChannel.addKey(downKey);
    headChannel.addKey(lapKey);
    headChannel.addKey(upKey);
    clip.boneChannels.push_back(headChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateMatingClip() const {
    ActivityMotionClip clip;
    clip.name = "mating";
    clip.activityType = ActivityType::MATING;
    clip.duration = 5.0f;
    clip.isLooping = false;

    // Body motion
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";

    for (float t = 0.0f; t < clip.duration; t += 0.2f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float phase = t / clip.duration;

        if (phase > 0.2f && phase < 0.8f) {
            float rhythm = std::sin((phase - 0.2f) / 0.6f * 8.0f * 3.14159f);
            key.position.z = rhythm * 0.03f;
        }
        pelvisChannel.addKey(key);
    }
    clip.boneChannels.push_back(pelvisChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateSleepingClip() const {
    ActivityMotionClip clip;
    clip.name = "sleeping";
    clip.activityType = ActivityType::SLEEPING;
    clip.duration = 4.0f;
    clip.isLooping = true;

    // Slow breathing motion
    BoneChannel chestChannel;
    chestChannel.boneName = "chest";

    for (float t = 0.0f; t <= clip.duration; t += 0.5f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float breathPhase = std::sin(t / clip.duration * 2.0f * 3.14159f);
        key.scale = glm::vec3(1.0f + breathPhase * 0.02f);
        chestChannel.addKey(key);
    }
    clip.boneChannels.push_back(chestChannel);

    // Body lowered
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";
    BonePoseKey lowKey = BonePoseKey::identity(0.0f);
    lowKey.position.y = -0.1f;
    pelvisChannel.addKey(lowKey);
    clip.boneChannels.push_back(pelvisChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateExcretingClip(ExcretionType type) const {
    ActivityMotionClip clip;
    clip.name = (type == ExcretionType::URINATE) ? "urinating" : "defecating";
    clip.activityType = ActivityType::EXCRETING;
    clip.duration = 2.0f;
    clip.isLooping = false;

    // Squat motion
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";

    BonePoseKey standKey = BonePoseKey::identity(0.0f);
    BonePoseKey squatKey = BonePoseKey::identity(0.3f);
    squatKey.position.y = -0.1f;
    BonePoseKey holdKey = BonePoseKey::identity(1.5f);
    holdKey.position.y = -0.1f;
    BonePoseKey riseKey = BonePoseKey::identity(2.0f);

    pelvisChannel.addKey(standKey);
    pelvisChannel.addKey(squatKey);
    pelvisChannel.addKey(holdKey);
    pelvisChannel.addKey(riseKey);
    clip.boneChannels.push_back(pelvisChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateGroomingClip(GroomingType type) const {
    ActivityMotionClip clip;
    clip.name = "grooming_" + std::to_string(static_cast<int>(type));
    clip.activityType = ActivityType::GROOMING;
    clip.duration = 3.0f;
    clip.isLooping = false;

    switch (type) {
        case GroomingType::STRETCH:
            {
                // Full body stretch
                BoneChannel spineChannel;
                spineChannel.boneName = "spine_1";

                BonePoseKey relaxKey = BonePoseKey::identity(0.0f);
                BonePoseKey stretchKey = BonePoseKey::identity(1.0f);
                stretchKey.position.z = 0.1f;
                stretchKey.rotation = glm::angleAxis(-0.2f, glm::vec3(1, 0, 0));
                BonePoseKey holdKey = BonePoseKey::identity(2.0f);
                holdKey.position.z = 0.1f;
                holdKey.rotation = glm::angleAxis(-0.2f, glm::vec3(1, 0, 0));
                BonePoseKey returnKey = BonePoseKey::identity(3.0f);

                spineChannel.addKey(relaxKey);
                spineChannel.addKey(stretchKey);
                spineChannel.addKey(holdKey);
                spineChannel.addKey(returnKey);
                clip.boneChannels.push_back(spineChannel);
            }
            break;

        case GroomingType::SHAKE:
            {
                // Rapid shake
                BoneChannel pelvisChannel;
                pelvisChannel.boneName = "pelvis";

                for (float t = 0.0f; t <= clip.duration; t += 0.05f) {
                    BonePoseKey key = BonePoseKey::identity(t);
                    float envelope = std::sin(t / clip.duration * 3.14159f);
                    float shake = std::sin(t * 40.0f) * envelope;
                    key.rotation = glm::angleAxis(shake * 0.1f, glm::vec3(0, 0, 1));
                    pelvisChannel.addKey(key);
                }
                clip.boneChannels.push_back(pelvisChannel);
            }
            break;

        default:
            // Default grooming motion
            break;
    }

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateThreatDisplayClip() const {
    ActivityMotionClip clip;
    clip.name = "threat_display";
    clip.activityType = ActivityType::THREAT_DISPLAY;
    clip.duration = 2.0f;
    clip.isLooping = false;

    // Rise up
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";

    BonePoseKey normalKey = BonePoseKey::identity(0.0f);
    BonePoseKey riseKey = BonePoseKey::identity(0.3f);
    riseKey.position.y = 0.1f;
    BonePoseKey holdKey = BonePoseKey::identity(1.5f);
    holdKey.position.y = 0.1f;
    BonePoseKey returnKey = BonePoseKey::identity(2.0f);

    pelvisChannel.addKey(normalKey);
    pelvisChannel.addKey(riseKey);
    pelvisChannel.addKey(holdKey);
    pelvisChannel.addKey(returnKey);
    clip.boneChannels.push_back(pelvisChannel);

    // Head forward (aggressive)
    BoneChannel headChannel;
    headChannel.boneName = "head";

    BonePoseKey headNormalKey = BonePoseKey::identity(0.0f);
    BonePoseKey headForwardKey = BonePoseKey::identity(0.3f);
    headForwardKey.rotation = glm::angleAxis(-0.15f, glm::vec3(1, 0, 0));

    headChannel.addKey(headNormalKey);
    headChannel.addKey(headForwardKey);
    clip.boneChannels.push_back(headChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateSubmissiveClip() const {
    ActivityMotionClip clip;
    clip.name = "submissive";
    clip.activityType = ActivityType::SUBMISSIVE_DISPLAY;
    clip.duration = 2.0f;
    clip.isLooping = false;

    // Crouch down
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";

    BonePoseKey standKey = BonePoseKey::identity(0.0f);
    BonePoseKey crouchKey = BonePoseKey::identity(0.5f);
    crouchKey.position.y = -0.15f;
    BonePoseKey holdKey = BonePoseKey::identity(1.5f);
    holdKey.position.y = -0.15f;

    pelvisChannel.addKey(standKey);
    pelvisChannel.addKey(crouchKey);
    pelvisChannel.addKey(holdKey);
    clip.boneChannels.push_back(pelvisChannel);

    // Head down
    BoneChannel headChannel;
    headChannel.boneName = "head";

    BonePoseKey headUpKey = BonePoseKey::identity(0.0f);
    BonePoseKey headDownKey = BonePoseKey::identity(0.5f);
    headDownKey.rotation = glm::angleAxis(0.3f, glm::vec3(1, 0, 0));

    headChannel.addKey(headUpKey);
    headChannel.addKey(headDownKey);
    clip.boneChannels.push_back(headChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateMatingDisplayClip() const {
    ActivityMotionClip clip;
    clip.name = "mating_display";
    clip.activityType = ActivityType::MATING_DISPLAY;
    clip.duration = 4.0f;
    clip.isLooping = true;

    // Body bob/dance
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";

    for (float t = 0.0f; t <= clip.duration; t += 0.25f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float phase = t / clip.duration;
        float bob = std::sin(phase * 4.0f * 2.0f * 3.14159f);
        key.position.y = bob * 0.05f + 0.03f;
        pelvisChannel.addKey(key);
    }
    clip.boneChannels.push_back(pelvisChannel);

    // Tail fan (if has tail)
    if (m_genes.hasTail) {
        BoneChannel tailChannel;
        tailChannel.boneName = "tail_0";
        addTailWag(tailChannel, 0.4f, 2.0f, clip.duration);
        clip.boneChannels.push_back(tailChannel);
    }

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generatePlayingClip() const {
    ActivityMotionClip clip;
    clip.name = "playing";
    clip.activityType = ActivityType::PLAYING;
    clip.duration = 3.0f;
    clip.isLooping = true;

    // Bouncy motion
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";

    for (float t = 0.0f; t <= clip.duration; t += 0.15f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float bounce = std::abs(std::sin(t * 4.0f));
        key.position.y = bounce * 0.08f;
        pelvisChannel.addKey(key);
    }
    clip.boneChannels.push_back(pelvisChannel);

    // Play bow moment
    BoneChannel spineChannel;
    spineChannel.boneName = "spine_2";

    BonePoseKey normalSpine = BonePoseKey::identity(0.0f);
    BonePoseKey bowSpine = BonePoseKey::identity(1.5f);
    bowSpine.rotation = glm::angleAxis(0.2f, glm::vec3(1, 0, 0));
    BonePoseKey returnSpine = BonePoseKey::identity(2.0f);

    spineChannel.addKey(normalSpine);
    spineChannel.addKey(bowSpine);
    spineChannel.addKey(returnSpine);
    clip.boneChannels.push_back(spineChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateInvestigatingClip() const {
    ActivityMotionClip clip;
    clip.name = "investigating";
    clip.activityType = ActivityType::INVESTIGATING;
    clip.duration = 2.5f;
    clip.isLooping = true;

    // Head weave
    BoneChannel headChannel;
    headChannel.boneName = "head";

    for (float t = 0.0f; t <= clip.duration; t += 0.2f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float weave = std::sin(t * 2.0f);
        key.rotation = glm::angleAxis(weave * 0.15f, glm::vec3(0, 1, 0));
        headChannel.addKey(key);
    }
    clip.boneChannels.push_back(headChannel);

    // Cautious crouch
    BoneChannel pelvisChannel;
    pelvisChannel.boneName = "pelvis";
    BonePoseKey crouchKey = BonePoseKey::identity(0.0f);
    crouchKey.position.y = -0.04f;
    pelvisChannel.addKey(crouchKey);
    clip.boneChannels.push_back(pelvisChannel);

    return clip;
}

ActivityMotionClip ActivityMotionGenerator::generateCallingClip() const {
    ActivityMotionClip clip;
    clip.name = "calling";
    clip.activityType = ActivityType::CALLING;
    clip.duration = 2.0f;
    clip.isLooping = false;

    // Head up for calling
    BoneChannel headChannel;
    headChannel.boneName = "head";

    BonePoseKey restKey = BonePoseKey::identity(0.0f);
    BonePoseKey upKey = BonePoseKey::identity(0.3f);
    upKey.rotation = glm::angleAxis(-0.2f, glm::vec3(1, 0, 0));
    BonePoseKey callKey = BonePoseKey::identity(0.5f);
    callKey.rotation = glm::angleAxis(-0.25f, glm::vec3(1, 0, 0));

    headChannel.addKey(restKey);
    headChannel.addKey(upKey);
    headChannel.addKey(callKey);
    clip.boneChannels.push_back(headChannel);

    // Chest expansion
    BoneChannel chestChannel;
    chestChannel.boneName = "chest";

    BonePoseKey chestNormal = BonePoseKey::identity(0.0f);
    BonePoseKey chestExpand = BonePoseKey::identity(0.4f);
    chestExpand.scale = glm::vec3(1.05f);
    BonePoseKey chestHold = BonePoseKey::identity(1.5f);
    chestHold.scale = glm::vec3(1.03f);
    BonePoseKey chestReturn = BonePoseKey::identity(2.0f);

    chestChannel.addKey(chestNormal);
    chestChannel.addKey(chestExpand);
    chestChannel.addKey(chestHold);
    chestChannel.addKey(chestReturn);
    clip.boneChannels.push_back(chestChannel);

    return clip;
}

void ActivityMotionGenerator::addSpineWave(BoneChannel& channel, float amplitude, float frequency, float duration) const {
    for (float t = 0.0f; t <= duration; t += 0.1f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float wave = std::sin(t * frequency * 2.0f * 3.14159f);
        key.rotation = glm::angleAxis(wave * amplitude, glm::vec3(1, 0, 0));
        channel.addKey(key);
    }
}

void ActivityMotionGenerator::addHeadBob(BoneChannel& channel, float amplitude, float frequency, float duration) const {
    for (float t = 0.0f; t <= duration; t += 0.1f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float bob = std::sin(t * frequency * 2.0f * 3.14159f);
        key.rotation = glm::angleAxis(bob * amplitude, glm::vec3(1, 0, 0));
        channel.addKey(key);
    }
}

void ActivityMotionGenerator::addTailWag(BoneChannel& channel, float amplitude, float frequency, float duration) const {
    for (float t = 0.0f; t <= duration; t += 0.1f) {
        BonePoseKey key = BonePoseKey::identity(t);
        float wag = std::sin(t * frequency * 2.0f * 3.14159f);
        key.rotation = glm::angleAxis(wag * amplitude, glm::vec3(0, 1, 0));
        channel.addKey(key);
    }
}

void ActivityMotionGenerator::addBodySquat(BoneChannel& channel, float depth, float holdTime, float duration) const {
    BonePoseKey standKey = BonePoseKey::identity(0.0f);
    BonePoseKey squatKey = BonePoseKey::identity(duration * 0.2f);
    squatKey.position.y = -depth;
    BonePoseKey holdKey = BonePoseKey::identity(holdTime);
    holdKey.position.y = -depth;
    BonePoseKey returnKey = BonePoseKey::identity(duration);

    channel.addKey(standKey);
    channel.addKey(squatKey);
    channel.addKey(holdKey);
    channel.addKey(returnKey);
}

void ActivityMotionGenerator::addLimbRaise(BoneChannel& channel, float angle, float holdTime, float duration) const {
    BonePoseKey downKey = BonePoseKey::identity(0.0f);
    BonePoseKey upKey = BonePoseKey::identity(duration * 0.2f);
    upKey.rotation = glm::angleAxis(angle, glm::vec3(1, 0, 0));
    BonePoseKey holdKey = BonePoseKey::identity(holdTime);
    holdKey.rotation = glm::angleAxis(angle, glm::vec3(1, 0, 0));
    BonePoseKey returnKey = BonePoseKey::identity(duration);

    channel.addKey(downKey);
    channel.addKey(upKey);
    channel.addKey(holdKey);
    channel.addKey(returnKey);
}

// =============================================================================
// ACTIVITY ANIMATION BLENDER IMPLEMENTATION
// =============================================================================

void ActivityAnimationBlender::addMotion(const ActivityMotionClip* clip, float weight, float time, float speed) {
    BlendedMotion motion;
    motion.clip = clip;
    motion.weight = weight;
    motion.time = time;
    motion.playbackSpeed = speed;
    m_motions.push_back(motion);
}

void ActivityAnimationBlender::clearMotions() {
    m_motions.clear();
}

void ActivityAnimationBlender::setBasePose(const SkeletonPose& basePose) {
    m_basePose = basePose;
}

void ActivityAnimationBlender::update(float deltaTime) {
    // Update motion times
    for (auto& motion : m_motions) {
        motion.time += deltaTime * motion.playbackSpeed;
    }

    // Update transition
    if (m_isTransitioning) {
        m_transitionProgress += deltaTime / m_transitionDuration;
        if (m_transitionProgress >= 1.0f) {
            m_transitionProgress = 1.0f;
            m_isTransitioning = false;

            // Replace old motions with transition target
            clearMotions();
            if (m_transitionTarget) {
                addMotion(m_transitionTarget, 1.0f);
            }
            m_transitionTarget = nullptr;
        }
    }
}

void ActivityAnimationBlender::getBlendedPose(SkeletonPose& outPose) const {
    if (!m_skeleton) return;

    // Start with base pose
    outPose = m_basePose;

    // Apply all motions
    float totalWeight = 0.0f;
    for (const auto& motion : m_motions) {
        if (motion.clip && motion.weight > 0.0f) {
            totalWeight += motion.weight;
        }
    }

    if (totalWeight > 0.0f) {
        for (const auto& motion : m_motions) {
            if (motion.clip && motion.weight > 0.0f) {
                float normalizedWeight = motion.weight / totalWeight;
                motion.clip->samplePose(motion.time, outPose, *m_skeleton);
            }
        }
    }
}

void ActivityAnimationBlender::transitionTo(const ActivityMotionClip* newClip, float transitionTime) {
    m_transitionTarget = newClip;
    m_transitionDuration = transitionTime;
    m_transitionProgress = 0.0f;
    m_isTransitioning = true;

    // Add new clip with zero weight (will be blended in)
    addMotion(newClip, 0.0f);
}

// =============================================================================
// ACTIVITY IK CONTROLLER IMPLEMENTATION
// =============================================================================

void ActivityIKController::cacheBoneIndices() {
    if (!m_skeleton) return;

    m_headBone = m_skeleton->findBoneIndex("head");
    m_neckBone = m_skeleton->findBoneIndex("neck_0");

    // Find foot bones
    m_footBones.clear();
    for (int i = 0; i < 4; ++i) {
        std::string leftFoot = "leg_" + std::to_string(i) + "_foot_L";
        std::string rightFoot = "leg_" + std::to_string(i) + "_foot_R";
        int32_t leftIdx = m_skeleton->findBoneIndex(leftFoot);
        int32_t rightIdx = m_skeleton->findBoneIndex(rightFoot);
        if (leftIdx >= 0) m_footBones.push_back(leftIdx);
        if (rightIdx >= 0) m_footBones.push_back(rightIdx);
    }

    m_leftHandBone = m_skeleton->findBoneIndex("arm_0_hand_L");
    m_rightHandBone = m_skeleton->findBoneIndex("arm_0_hand_R");
}

void ActivityIKController::updateForActivity(ActivityType activity, float progress,
                                             const glm::vec3& creaturePosition,
                                             const glm::vec3& targetPosition) {
    m_targets.hasLookTarget = false;
    m_targets.hasLeftHandTarget = false;
    m_targets.hasRightHandTarget = false;
    m_targets.hasTailTarget = false;

    switch (activity) {
        case ActivityType::EATING:
        case ActivityType::DRINKING:
            m_targets.hasLookTarget = true;
            m_targets.lookTarget = targetPosition;
            break;

        case ActivityType::MATING:
        case ActivityType::MATING_DISPLAY:
        case ActivityType::THREAT_DISPLAY:
            m_targets.hasLookTarget = true;
            m_targets.lookTarget = targetPosition;
            break;

        case ActivityType::INVESTIGATING:
            m_targets.hasLookTarget = true;
            m_targets.lookTarget = targetPosition;
            break;

        default:
            break;
    }
}

void ActivityIKController::applyIK(SkeletonPose& pose) {
    if (!m_ikSystem || !m_skeleton) return;

    // Apply head look-at
    if (m_targets.hasLookTarget && m_headBone >= 0) {
        // IK system would handle look-at constraint
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

namespace ActivityAnimationUtils {

float smoothstep(float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float smootherstep(float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float bounce(float t, int bounces) {
    if (t >= 1.0f) return 1.0f;

    float b = static_cast<float>(bounces);
    float decay = std::exp(-t * 3.0f);
    float oscillation = std::abs(std::sin(t * b * 3.14159f));

    return 1.0f - decay * oscillation;
}

float spring(float t, float stiffness, float damping) {
    if (t >= 1.0f) return 1.0f;

    float omega = std::sqrt(stiffness);
    float decay = std::exp(-damping * t);
    float oscillation = std::cos(omega * t);

    return 1.0f - decay * oscillation;
}

glm::quat slerpPath(const std::vector<glm::quat>& path, float t) {
    if (path.empty()) return glm::quat(1, 0, 0, 0);
    if (path.size() == 1) return path[0];

    float scaledT = t * (path.size() - 1);
    size_t idx = static_cast<size_t>(scaledT);
    float localT = scaledT - idx;

    idx = std::min(idx, path.size() - 2);

    return glm::slerp(path[idx], path[idx + 1], localT);
}

glm::quat waveRotation(float phase, float amplitude, const glm::vec3& axis) {
    float angle = std::sin(phase * 2.0f * 3.14159f) * amplitude;
    return glm::angleAxis(angle, glm::normalize(axis));
}

glm::quat calculateMovementTilt(const glm::vec3& velocity, float maxTilt) {
    float speed = glm::length(velocity);
    if (speed < 0.01f) return glm::quat(1, 0, 0, 0);

    glm::vec3 dir = velocity / speed;

    // Tilt forward based on speed
    float forwardTilt = std::min(speed * 0.1f, maxTilt);

    // Bank into turns (simplified)
    float bankAngle = 0.0f;

    glm::quat forward = glm::angleAxis(-forwardTilt, glm::vec3(1, 0, 0));
    glm::quat bank = glm::angleAxis(bankAngle, glm::vec3(0, 0, 1));

    return forward * bank;
}

} // namespace ActivityAnimationUtils

} // namespace animation
