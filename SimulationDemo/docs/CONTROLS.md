# SimulationDemo Controls

Complete reference for all input controls in the simulation.

## Keyboard Controls

### Camera Movement

| Key | Action | Details |
|-----|--------|---------|
| `W` | Move Forward | Camera moves in look direction |
| `S` | Move Backward | Camera moves opposite to look direction |
| `A` | Strafe Left | Camera moves perpendicular left |
| `D` | Strafe Right | Camera moves perpendicular right |
| `Space` | Move Up | Camera moves along world Y-axis |
| `Ctrl` | Move Down | Camera moves down along world Y-axis |

**Movement Speed**: 25 units/second (modified by `Shift` for sprint)

### Simulation Control

| Key | Action | Details |
|-----|--------|---------|
| `P` | Pause/Resume | Toggles simulation update loop |
| `+` / `=` | Speed Up | Increases simulation speed (max 8.0x) |
| `-` | Slow Down | Decreases simulation speed (min 0.25x) |

**Speed Levels**: 0.25x, 0.5x, 1.0x, 2.0x, 4.0x, 8.0x

### Application Control

| Key | Action | Details |
|-----|--------|---------|
| `Tab` | Toggle Mouse Capture | Locks/unlocks cursor for camera look |
| `Esc` | Exit Application | Closes the window and exits |

## Mouse Controls

### Camera Look

| Input | Action | Condition |
|-------|--------|-----------|
| Mouse Move (X) | Yaw (horizontal look) | Mouse captured (Tab) |
| Mouse Move (Y) | Pitch (vertical look) | Mouse captured (Tab) |

**Mouse Sensitivity**: 0.1 (configurable in Camera settings)

**Pitch Limits**: -89 to +89 degrees (prevents gimbal lock)

### Camera Zoom

| Input | Action | Details |
|-------|--------|---------|
| Scroll Up | Zoom In | Decreases FOV (min 1 degree) |
| Scroll Down | Zoom Out | Increases FOV (max 90 degrees) |

**Default FOV**: 45 degrees

## Camera System Details

### First-Person Camera

The camera uses a first-person perspective with the following characteristics:

```
Position: vec3 (world coordinates)
Front:    vec3 (normalized look direction)
Up:       vec3 (world up, typically 0,1,0)
Right:    vec3 (computed from Front x Up)

Yaw:   Rotation around Y-axis (horizontal)
Pitch: Rotation around X-axis (vertical)
```

### View Matrix Calculation

```cpp
glm::mat4 GetViewMatrix() {
    return glm::lookAt(position, position + front, up);
}
```

### Mouse Look Processing

```cpp
void ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Update front vector
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
}
```

## Mouse Capture States

### Captured Mode (Tab pressed)
- Cursor hidden and locked to window center
- Mouse movement controls camera orientation
- Raw mouse input enabled for precise control
- Press `Tab` again to release

### Released Mode (default)
- Cursor visible and free to move
- Mouse movement ignored for camera
- Can interact with UI elements (future)

## Input Guards

The input system implements several guards to prevent unwanted behavior:

### Focus Guard
Input processing only occurs when the window has focus. This prevents:
- Accidental input when alt-tabbed
- Input spikes when regaining focus

### First Mouse Guard
On initial capture or focus regain, the first mouse movement is ignored to prevent camera jumps.

### Delta Time Capping
Maximum delta time is capped at 100ms (10 FPS minimum) to prevent large movement jumps when the application was unfocused.

## Debug Command Interface

Commands can be sent via `simulator_command.txt` file:

| Command | Action |
|---------|--------|
| `pause` | Pause simulation |
| `resume` | Resume simulation |
| `speed_up` | Increase simulation speed |
| `speed_down` | Decrease simulation speed |
| `screenshot` | Capture PNG screenshot |
| `debug_info` | Write debug stats to file |
| `camera_pos X Y Z` | Set camera position |
| `camera_look PITCH YAW` | Set camera orientation |
| `camera_info` | Output current camera state |

## Default Camera Settings

```cpp
struct CameraDefaults {
    float speed       = 25.0f;   // Movement speed
    float sensitivity = 0.1f;    // Mouse sensitivity
    float fov         = 45.0f;   // Field of view (degrees)
    float nearPlane   = 0.1f;    // Near clip plane
    float farPlane    = 1000.0f; // Far clip plane

    vec3 position     = {150, 50, 150}; // Starting position
    float yaw         = -90.0f;  // Looking along -Z
    float pitch       = -20.0f;  // Slightly looking down
};
```

## Control Flow Diagram

```
+------------------+
|   Window Event   |
+------------------+
         |
         v
+------------------+     No      +------------------+
| Window Focused?  |------------>| Ignore Input     |
+------------------+             +------------------+
         | Yes
         v
+------------------+
|  Input Type?     |
+------------------+
    |         |
    v         v
+--------+ +--------+
|Keyboard| | Mouse  |
+--------+ +--------+
    |         |
    v         v
+--------+ +------------------+     No      +------------------+
|Process | | Mouse Captured?  |------------>| Ignore Movement  |
|Keys    | +------------------+             +------------------+
+--------+        | Yes
    |             v
    |      +------------------+     Yes     +------------------+
    |      | First Mouse?     |------------>| Skip, Clear Flag |
    |      +------------------+             +------------------+
    |             | No
    |             v
    |      +------------------+
    |      | Process Look     |
    |      +------------------+
    |             |
    v             v
+------------------+
| Update Camera    |
+------------------+
         |
         v
+------------------+
| Update Simulation|
+------------------+
```
