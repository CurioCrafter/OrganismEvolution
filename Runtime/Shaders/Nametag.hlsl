// Nametag shader - Billboard quads with procedural digit rendering
// Enhanced with species names, health bars, and status icons
// Ported from OrganismEvolution GLSL

// ============================================================================
// Constant Buffers
// ============================================================================

cbuffer Constants : register(b0)
{
    // Per-frame constants (must match C++ Constants struct exactly)
    // HLSL packing: float4x4 MUST be 16-byte aligned
    float4x4 view;             // Offset 0, 64 bytes
    float4x4 projection;       // Offset 64, 64 bytes
    float4x4 viewProjection;   // Offset 128, 64 bytes
    float3 viewPos;            // Offset 192, 12 bytes
    float time;                // Offset 204, 4 bytes (packs after viewPos)
    float3 lightPos;           // Offset 208, 12 bytes
    float padding1;            // Offset 220, 4 bytes (packs after lightPos)
    float3 lightColor;         // Offset 224, 12 bytes
    float padding2;            // Offset 236, 4 bytes (packs after lightColor)
    // Day/Night cycle data (must match C++ Constants struct order)
    float dayTime;             // Offset 240, 4 bytes
    float3 skyTopColor;        // Offset 244, 12 bytes (packs after dayTime)
    float3 skyHorizonColor;    // Offset 256, 12 bytes
    float sunIntensity;        // Offset 268, 4 bytes (packs after skyHorizonColor)
    float3 ambientColor;       // Offset 272, 12 bytes
    float starVisibility;      // Offset 284, 4 bytes (packs after ambientColor)
    float2 dayNightPadding;    // Offset 288, 8 bytes
    float2 modelAlignPadding;  // Offset 296, 8 bytes - CRITICAL: align model to 16-byte boundary
    // Per-object constants (model starts at offset 304, which is 16-byte aligned)
    float4x4 model;            // Offset 304, 64 bytes (304 % 16 == 0, correctly aligned)
    float3 textColor;          // Offset 368, 12 bytes (maps to objectColor)
    int creatureID;            // Offset 380, 4 bytes (packs after textColor)
    int creatureType;          // Offset 384, 4 bytes
    float3 endPadding;         // Offset 388, 12 bytes (pad to 400)
};

// Extended nametag constant buffer for enhanced features
cbuffer NametagExtended : register(b1)
{
    float2 screenSize;         // Screen dimensions for screen-space calculations
    float fadeStartDistance;   // Distance at which fading begins
    float fadeEndDistance;     // Distance at which fully faded
    float healthPercent;       // Creature health [0-1]
    float energyPercent;       // Creature energy [0-1]
    uint statusFlags;          // Bit flags for status icons
    float isSelected;          // 1.0 if selected, 0.0 otherwise
    float4 healthBarColor;     // Health bar foreground color
    float4 energyBarColor;     // Energy bar foreground color
    float4 selectionColor;     // Selection highlight color
};

// ============================================================================
// Input/Output Structures
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// ============================================================================
// Digit Patterns (Seven-segment display)
// ============================================================================

// Segment layout:
//   AAA
//  F   B
//   GGG
//  E   C
//   DDD

static const int digitPatterns[10] = {
    0x3F,  // 0: A B C D E F
    0x06,  // 1: B C
    0x5B,  // 2: A B D E G
    0x4F,  // 3: A B C D G
    0x66,  // 4: B C F G
    0x6D,  // 5: A C D F G
    0x7D,  // 6: A C D E F G
    0x07,  // 7: A B C
    0x7F,  // 8: A B C D E F G
    0x6F   // 9: A B C D F G
};

bool drawSegment(float2 uv, int segment, float digitX, float digitWidth)
{
    float localX = (uv.x - digitX) / digitWidth;
    float localY = uv.y;

    if (segment == 0) // A - top horizontal
        return localY > 0.85 && localY < 1.0 && localX > 0.1 && localX < 0.9;
    if (segment == 1) // B - top-right vertical
        return localX > 0.75 && localX < 0.9 && localY > 0.5 && localY < 0.95;
    if (segment == 2) // C - bottom-right vertical
        return localX > 0.75 && localX < 0.9 && localY > 0.05 && localY < 0.5;
    if (segment == 3) // D - bottom horizontal
        return localY > 0.0 && localY < 0.15 && localX > 0.1 && localX < 0.9;
    if (segment == 4) // E - bottom-left vertical
        return localX > 0.1 && localX < 0.25 && localY > 0.05 && localY < 0.5;
    if (segment == 5) // F - top-left vertical
        return localX > 0.1 && localX < 0.25 && localY > 0.5 && localY < 0.95;
    if (segment == 6) // G - middle horizontal
        return localY > 0.45 && localY < 0.55 && localX > 0.1 && localX < 0.9;

    return false;
}

bool drawDigit(float2 uv, int digit, float digitX, float digitWidth)
{
    if (digit < 0 || digit > 9) return false;
    if (uv.x < digitX || uv.x > digitX + digitWidth) return false;

    int pattern = digitPatterns[digit];

    [unroll]
    for (int s = 0; s < 7; s++)
    {
        if ((pattern & (1 << s)) != 0)
        {
            if (drawSegment(uv, s, digitX, digitWidth))
                return true;
        }
    }
    return false;
}

// ============================================================================
// Vertex Shader
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.texCoord = input.texCoord;
    output.position = mul(viewProjection, mul(model, float4(input.position, 1.0)));  // Column-major: matrix-on-left

    return output;
}

// ============================================================================
// Pixel Shader
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    // Flip Y coordinate
    float2 uv = float2(input.texCoord.x, 1.0 - input.texCoord.y);

    // Calculate number of digits
    int id = creatureID;
    int numDigits = 1;
    int temp = id;
    while (temp >= 10)
    {
        temp /= 10;
        numDigits++;
    }

    // Calculate digit width
    float spacing = 0.05;
    float totalDigitWidth = (1.0 - spacing * (float(numDigits) + 1.0)) / float(numDigits);
    float digitWidth = min(totalDigitWidth, 0.25);

    // Center the digits
    float totalWidth = float(numDigits) * digitWidth + float(numDigits - 1) * spacing;
    float startX = (1.0 - totalWidth) / 2.0;

    bool inText = false;

    // Extract digits and draw
    int divisor = 1;
    for (int i = 0; i < numDigits - 1; i++)
        divisor *= 10;

    float currentX = startX;
    temp = id;

    [loop]
    for (int j = 0; j < numDigits && j < 6; j++)
    {
        int digit = temp / divisor;
        temp = temp % divisor;
        divisor /= 10;

        if (drawDigit(uv, digit, currentX, digitWidth))
        {
            inText = true;
            break;
        }
        currentX += digitWidth + spacing;
    }

    // Draw type indicator
    float typeX = 0.02;
    float typeWidth = 0.15;
    bool inType = false;

    if (uv.x < typeX + typeWidth && uv.x > typeX)
    {
        if (creatureType == 0)
        {
            // H for Herbivore
            float localX = (uv.x - typeX) / typeWidth;
            if ((localX < 0.3 || localX > 0.7) && uv.y > 0.1 && uv.y < 0.9)
                inType = true;
            if (localX > 0.2 && localX < 0.8 && uv.y > 0.45 && uv.y < 0.55)
                inType = true;
        }
        else
        {
            // C for Carnivore
            float localX = (uv.x - typeX) / typeWidth;
            if (localX > 0.2 && localX < 0.9 && (uv.y > 0.85 || uv.y < 0.15))
                inType = true;
            if (localX < 0.3 && uv.y > 0.1 && uv.y < 0.9)
                inType = true;
        }
    }

    if (inText || inType)
    {
        return float4(textColor, 1.0);
    }
    else
    {
        // Semi-transparent dark background
        return float4(0.1, 0.1, 0.1, 0.6);
    }
}

// ============================================================================
// Enhanced Pixel Shaders for Health Bars and Selection
// ============================================================================

// Health/Energy bar pixel shader
float4 PSMainHealthBar(PSInput input) : SV_TARGET
{
    float2 uv = float2(input.texCoord.x, 1.0 - input.texCoord.y);

    // Determine if this is health bar (top) or energy bar (bottom) based on Y
    bool isHealthBar = uv.y > 0.5;
    float fillPercent = isHealthBar ? healthPercent : energyPercent;
    float4 barColor = isHealthBar ? healthBarColor : energyBarColor;

    // Bar dimensions
    float barMargin = 0.05;
    float barHeight = 0.35;

    // Normalize Y to bar space
    float localY = isHealthBar ? (uv.y - 0.55) / barHeight : (uv.y - 0.1) / barHeight;

    // Check if in bar region
    if (uv.x > barMargin && uv.x < (1.0 - barMargin) && localY > 0.0 && localY < 1.0)
    {
        // Filled region
        float fillWidth = barMargin + (1.0 - 2.0 * barMargin) * fillPercent;
        if (uv.x < fillWidth)
        {
            // Gradient effect on fill
            float intensity = 0.8 + 0.2 * localY;
            return float4(barColor.rgb * intensity, barColor.a);
        }
        else
        {
            // Empty region - dark background
            return float4(0.15, 0.15, 0.15, 0.8);
        }
    }

    // Border
    float borderWidth = 0.02;
    if (uv.x > barMargin - borderWidth && uv.x < (1.0 - barMargin + borderWidth) &&
        localY > -borderWidth && localY < (1.0 + borderWidth))
    {
        return float4(0.3, 0.3, 0.3, 0.9);
    }

    discard;
    return float4(0, 0, 0, 0);
}

// Selection highlight with glow effect
float4 PSMainSelection(PSInput input) : SV_TARGET
{
    float2 uv = input.texCoord;

    // Calculate distance from center
    float2 center = float2(0.5, 0.5);
    float dist = length(uv - center) * 2.0; // Normalize to [0, 1] at edges

    // Pulsing glow effect
    float pulse = 0.8 + 0.2 * sin(time * 3.0);

    // Soft circular gradient with pulse
    float glow = saturate(1.0 - dist) * pulse;
    glow = pow(glow, 1.5); // Soften falloff

    // Only show when selected
    float4 finalColor = selectionColor;
    finalColor.a = glow * isSelected * 0.5;

    if (finalColor.a < 0.01) discard;

    return finalColor;
}

// Status icon rendering (uses texture atlas coordinates)
float4 PSMainStatusIcon(PSInput input) : SV_TARGET
{
    float2 uv = input.texCoord;

    // Status icon layout - 8 icons in a row
    // Bit 0: Hungry, Bit 1: Scared, Bit 2: Mating, etc.

    float iconWidth = 0.125; // 1/8 of width per icon
    int iconIndex = int(uv.x / iconWidth);
    float localX = fmod(uv.x, iconWidth) / iconWidth;

    // Check if this icon should be shown
    if ((statusFlags & (1u << iconIndex)) == 0)
    {
        discard;
        return float4(0, 0, 0, 0);
    }

    // Simple procedural icons
    float4 iconColor = float4(1, 1, 1, 1);
    float inIcon = 0.0;

    // Hungry icon (exclamation mark)
    if (iconIndex == 0)
    {
        iconColor = float4(1.0, 0.8, 0.2, 1.0); // Yellow
        if (localX > 0.4 && localX < 0.6)
        {
            if (uv.y > 0.3 && uv.y < 0.8) inIcon = 1.0;
            if (uv.y > 0.1 && uv.y < 0.2) inIcon = 1.0;
        }
    }
    // Scared icon (question mark)
    else if (iconIndex == 1)
    {
        iconColor = float4(0.8, 0.8, 1.0, 1.0); // Light blue
        float2 localUV = float2(localX, uv.y);
        float dist = length(localUV - float2(0.5, 0.65));
        if (dist > 0.15 && dist < 0.25 && localUV.y > 0.5) inIcon = 1.0;
        if (localX > 0.4 && localX < 0.6 && uv.y > 0.3 && uv.y < 0.5) inIcon = 1.0;
        if (localX > 0.4 && localX < 0.6 && uv.y > 0.1 && uv.y < 0.2) inIcon = 1.0;
    }
    // Mating icon (heart)
    else if (iconIndex == 2)
    {
        iconColor = float4(1.0, 0.3, 0.5, 1.0); // Pink
        float2 localUV = float2(localX - 0.5, uv.y - 0.5) * 2.0;
        float heart = pow(localUV.x * localUV.x + localUV.y * localUV.y - 1.0, 3.0)
                    - localUV.x * localUV.x * localUV.y * localUV.y * localUV.y;
        inIcon = heart < 0.0 ? 1.0 : 0.0;
    }
    // Attacking icon (X)
    else if (iconIndex == 3)
    {
        iconColor = float4(1.0, 0.2, 0.2, 1.0); // Red
        float2 localUV = float2(localX, uv.y);
        float d1 = abs(localUV.x - localUV.y);
        float d2 = abs(localUV.x - (1.0 - localUV.y));
        if (d1 < 0.15 || d2 < 0.15) inIcon = 1.0;
    }
    // Fleeing icon (arrow left)
    else if (iconIndex == 4)
    {
        iconColor = float4(0.5, 0.8, 1.0, 1.0); // Light blue
        float2 localUV = float2(localX, uv.y);
        if (localUV.y > 0.4 && localUV.y < 0.6 && localUV.x > 0.3) inIcon = 1.0;
        if (abs(localUV.y - 0.5) < (0.5 - localUV.x) * 0.8 && localUV.x < 0.5) inIcon = 1.0;
    }
    // Sleeping icon (Z)
    else if (iconIndex == 5)
    {
        iconColor = float4(0.7, 0.7, 0.9, 1.0); // Gray-blue
        float2 localUV = float2(localX, uv.y);
        if (localUV.y > 0.75 && localX > 0.2 && localX < 0.8) inIcon = 1.0;
        if (localUV.y < 0.25 && localX > 0.2 && localX < 0.8) inIcon = 1.0;
        float diag = 1.0 - localUV.y;
        if (abs(localX - 0.2 - diag * 0.6) < 0.1 && localUV.y > 0.2 && localUV.y < 0.8) inIcon = 1.0;
    }

    if (inIcon < 0.5)
    {
        discard;
        return float4(0, 0, 0, 0);
    }

    return iconColor;
}

// ============================================================================
// Billboard Vertex Shader (camera-facing quads)
// ============================================================================

struct BillboardVSInput
{
    float3 worldPosition : POSITION;
    float2 quadOffset : TEXCOORD0;  // Offset from center (-0.5 to 0.5)
    float2 texCoord : TEXCOORD1;
    float scale : TEXCOORD2;
};

struct BillboardPSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float alpha : TEXCOORD1;
};

BillboardPSInput VSMainBillboard(BillboardVSInput input)
{
    BillboardPSInput output;

    // Calculate camera right and up vectors from view matrix
    float3 cameraRight = float3(view[0][0], view[1][0], view[2][0]);
    float3 cameraUp = float3(view[0][1], view[1][1], view[2][1]);

    // Calculate distance for fading and scaling
    float3 toCamera = viewPos - input.worldPosition;
    float distance = length(toCamera);

    // Scale based on distance (perspective correction)
    float distanceScale = saturate(1.0 / (distance * 0.05 + 1.0)) * 2.0;
    float finalScale = input.scale * distanceScale;

    // Create billboard quad
    float3 worldPos = input.worldPosition;
    worldPos += cameraRight * input.quadOffset.x * finalScale;
    worldPos += cameraUp * input.quadOffset.y * finalScale;

    output.position = mul(viewProjection, float4(worldPos, 1.0));
    output.texCoord = input.texCoord;

    // Calculate alpha fade
    output.alpha = 1.0 - saturate((distance - fadeStartDistance) / (fadeEndDistance - fadeStartDistance));

    return output;
}

float4 PSMainBillboard(BillboardPSInput input) : SV_TARGET
{
    float4 color = float4(textColor, 1.0);
    color.a *= input.alpha;

    if (color.a < 0.01) discard;

    return color;
}
