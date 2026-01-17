# Building on Linux/macOS

> NOTE: The active build is Windows-only (DirectX 12). These Linux/macOS notes
> are legacy and may not work with the current codebase.

The project can also be built on Linux or macOS with minor adjustments.

## Ubuntu/Debian Linux

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libglfw3-dev \
    libglew-dev \
    libglm-dev \
    mesa-common-dev

# Build
cd OrganismEvolution
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./OrganismEvolution
```

## Fedora/RHEL Linux

```bash
# Install dependencies
sudo dnf install -y \
    gcc-c++ \
    cmake \
    glfw-devel \
    glew-devel \
    glm-devel \
    mesa-libGL-devel

# Build
cd OrganismEvolution
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./OrganismEvolution
```

## Arch Linux

```bash
# Install dependencies
sudo pacman -S \
    base-devel \
    cmake \
    glfw-x11 \
    glew \
    glm \
    mesa

# Build
cd OrganismEvolution
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./OrganismEvolution
```

## macOS

```bash
# Install Homebrew if needed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake glfw glew glm

# Build
cd OrganismEvolution
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Run
./OrganismEvolution
```

## Notes

- The code is cross-platform C++17
- OpenGL 3.3+ is required
- On headless servers, you may need software rendering (Mesa)
- macOS may require additional permissions for OpenGL access
