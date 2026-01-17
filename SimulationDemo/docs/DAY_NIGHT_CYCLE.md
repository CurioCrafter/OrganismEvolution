# Day/Night Cycle System

## Integration Status: ✅ FULLY INTEGRATED

The day/night cycle system is complete and fully integrated with the rendering pipeline. All shader fields are populated each frame, and visual feedback (sky colors, sun position, stars) changes dynamically.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      DayNightCycle Class                        │
│  (SimulationDemo/DayNightCycle.h)                              │
├─────────────────────────────────────────────────────────────────┤
│  dayTime: 0-1 (0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk)    │
│  dayLengthSeconds: 120.0 (2-minute full cycle by default)      │
│  paused: bool (toggle via ImGui)                               │
├─────────────────────────────────────────────────────────────────┤
│  Update(dt)        → Advances dayTime each frame               │
│  GetSkyColors()    → Returns SkyColors struct                  │
│  GetSunPosition()  → Dynamic sun/moon world position           │
│  GetStarVisibility() → 0-1 fade for star rendering             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Constants Struct                            │
│  (main.cpp lines 2433-2459)                                    │
├─────────────────────────────────────────────────────────────────┤
│  dayTime:          f32   (Offset 240)                          │
│  skyTopColor:      float3 (Offset 244)                         │
│  skyHorizonColor:  float3 (Offset 256)                         │
│  sunIntensity:     f32   (Offset 268)                          │
│  ambientColor:     float3 (Offset 272)                         │
│  starVisibility:   f32   (Offset 284)                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Terrain.hlsl                               │
│  (Runtime/Shaders/Terrain.hlsl)                                │
├─────────────────────────────────────────────────────────────────┤
│  cbuffer Constants : register(b0)                              │
│    - dayTime, skyTopColor, skyHorizonColor                     │
│    - sunIntensity, ambientColor, starVisibility                │
├─────────────────────────────────────────────────────────────────┤
│  Functions using day/night data:                               │
│    - AddStars()           → Procedural stars with visibility   │
│    - calculateAmbientIBL() → Uses skyTopColor, skyHorizonColor │
│    - PSMain fog section   → Blends sky colors, adds stars      │
└─────────────────────────────────────────────────────────────────┘
```

## Time Periods

| Time Range | Period Name | Description |
|------------|-------------|-------------|
| 0.00-0.15  | Night       | Deep night, full stars, cool moonlight |
| 0.15-0.20  | Pre-Dawn    | Faint light on horizon, stars fading |
| 0.20-0.30  | Dawn        | Golden hour sunrise, warm colors |
| 0.30-0.45  | Morning     | Transition to full daylight |
| 0.45-0.55  | Midday      | Bright, slightly warm, full sun |
| 0.55-0.70  | Afternoon   | Gradual transition to evening |
| 0.70-0.80  | Dusk        | Golden hour sunset, orange/red tones |
| 0.80-0.85  | Twilight    | After sunset, purple sky, stars appearing |
| 0.85-1.00  | Night       | Full darkness, stars visible |

## Visual Effects

### Sky Gradient
- **Sky Top Color**: Zenith color (directly overhead)
- **Sky Horizon Color**: Color at the horizon line
- Both colors interpolate smoothly between time periods
- Fog uses a blend of these colors based on view direction

### Dynamic Lighting
- **Sun Position**: Rises from East (+X) at dawn, zenith at noon, sets West (-X) at dusk
- **Moon Position**: Opposite to sun, visible during night hours
- **Sun Intensity**: 0.15 (night) → 1.0 (midday) → 0.15 (night)
- **Ambient Color**: Matches time of day for realistic global illumination

### Stars
- **Procedural Generation**: Hash-based noise creates star field
- **Twinkle Effect**: Sin-wave animation for realistic twinkling
- **Visibility Fade**: Smooth fade in/out during twilight (0.15-0.20 and 0.80-0.85)
- Stars added to fog/sky in pixel shader for distant atmosphere

## Code Integration Points

### World::Update() (main.cpp:1912)
```cpp
dayNight.Update(dt);  // Advances time each frame
```

### Render Frame (main.cpp:3196-3209)
```cpp
SkyColors sky = world.dayNight.GetSkyColors();
Vec3 sunPos = world.dayNight.GetSunPosition();

frameConstants.lightPos = sunPos;
frameConstants.lightColor = sky.sunColor;
frameConstants.dayTime = world.dayNight.dayTime;
frameConstants.skyTopColor = sky.skyTop;
frameConstants.skyHorizonColor = sky.skyHorizon;
frameConstants.sunIntensity = sky.sunIntensity;
frameConstants.ambientColor = sky.ambientColor;
frameConstants.starVisibility = world.dayNight.GetStarVisibility();
```

### Clear Color (main.cpp:3164-3165)
```cpp
SkyColors clearSky = world.dayNight.GetSkyColors();
cmdList->ClearRenderTarget(renderTargets[0],
    Vec4(clearSky.skyTop.x, clearSky.skyTop.y, clearSky.skyTop.z, 1.0f));
```

## ImGui Controls

The Day/Night Cycle panel provides:
- **Time Display**: Shows current period name (Dawn, Noon, etc.)
- **Time Slider**: Manual control of dayTime (0.0-1.0)
- **Day Length**: Adjust cycle duration (30-600 seconds)
- **Pause Toggle**: Freeze time for debugging
- **Quick Buttons**: Jump to Dawn, Noon, Dusk, or Night
- **Color Preview**: Live display of sky colors and intensity

## Success Criteria: ✅ All Met

| Criterion | Status | Details |
|-----------|--------|---------|
| Sky color changes | ✅ | skyTopColor/skyHorizonColor interpolate through 8 periods |
| Lighting intensity varies | ✅ | sunIntensity ranges 0.15-1.0 based on time |
| Stars visible at night | ✅ | AddStars() with starVisibility fade |
| Sun position indicator | ✅ | GetSunPosition() drives lightPos in shader |
| Skybox gradient | ✅ | Fog blends skyTopColor/skyHorizonColor |

## Performance Notes

- All calculations are CPU-side in DayNightCycle class
- Single constant buffer update per frame (no per-draw overhead)
- Star rendering uses procedural noise (no texture lookups)
- Smooth interpolation via lerp functions avoids branching
