# Dockerfile for building OrganismEvolution with MinGW
FROM archlinux:latest

# Install MSYS2-like environment with MinGW
RUN pacman -Sy --noconfirm \
    mingw-w64-gcc \
    cmake \
    make \
    git

# Install dependencies (we'd need to adapt this for cross-compilation)
# Note: This is simplified - actual cross-compilation is more complex
RUN pacman -Sy --noconfirm \
    mesa \
    glew \
    glfw-x11 \
    glm

WORKDIR /app

# Copy project files
COPY . .

# Build
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

CMD ["./build/OrganismEvolution"]
