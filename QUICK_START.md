# Quick Start Guide

## For First-Time Setup

### 1. Install MSYS2
Download from [https://www.msys2.org/](https://www.msys2.org/) and install.

### 2. Open MSYS2 MinGW 64-bit Terminal
Find it in your Start Menu under "MSYS2" folder.

### 3. Install All Dependencies (Copy-Paste This)
```bash
pacman -Syu && \
pacman -S --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-glfw \
  mingw-w64-x86_64-glew \
  mingw-w64-x86_64-glm \
  mingw-w64-x86_64-mesa \
  git \
  make
```

### 4. Navigate to Project (Adjust Path if Needed)
```bash
cd /c/Users/andre/Desktop/OrganismEvolution
```

### 5. Build and Run
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
./OrganismEvolution.exe
```

## Daily Development Workflow

### Build After Code Changes
```bash
cd build
cmake --build .
./OrganismEvolution.exe
```

### Clean Build (if needed)
```bash
cd build
rm -rf *
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### Git Workflow
```bash
# Check status
git status

# Add changes
git add .

# Commit
git commit -m "Your commit message"

# Push to GitHub
git push
```

## Common Commands

### Verify Installation
```bash
gcc --version      # Should show GCC version
cmake --version    # Should show CMake version
git --version      # Should show Git version
```

### Update MSYS2 Packages
```bash
pacman -Syu
```

### Search for Packages
```bash
pacman -Ss <package-name>
```

## Troubleshooting

### "Command not found" errors
Make sure you're using **MSYS2 MinGW 64-bit** terminal, not regular MSYS2.

### OpenGL errors
```bash
pacman -S mingw-w64-x86_64-mesa
```

### CMake can't find packages
```bash
pacman -S mingw-w64-x86_64-pkg-config
```

### Fresh start
```bash
rm -rf build
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## Project Structure at a Glance

```
OrganismEvolution/
├── src/
│   └── main.cpp              # Entry point (currently basic OpenGL setup)
├── CMakeLists.txt            # Build configuration
├── PROJECT_PLAN.md           # Full development plan and research
├── README.md                 # Project overview
├── SETUP_GUIDE.md            # Detailed setup instructions
└── QUICK_START.md            # This file
```

## Next Development Steps

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for the full roadmap. The immediate next phases are:

1. **Phase 1**: Basic rendering (camera, simple shapes)
2. **Phase 2**: Terrain generation (Perlin noise, island/ocean)
3. **Phase 3**: Creature foundation (genome, physics, movement)
4. **Phase 4**: Neural networks (AI behavior)
5. **Phase 5**: Evolution system (genetic algorithms)
6. **Phase 6**: Balancing and statistics
7. **Phase 7**: Polish and features

## Resources

- **Learn OpenGL**: [https://learnopengl.com/](https://learnopengl.com/)
- **GLFW Docs**: [https://www.glfw.org/docs/latest/](https://www.glfw.org/docs/latest/)
- **CMake Tutorial**: [https://cmake.org/cmake/help/latest/guide/tutorial/](https://cmake.org/cmake/help/latest/guide/tutorial/)
- **Project Repository**: [https://github.com/CurioCrafter/OrganismEvolution](https://github.com/CurioCrafter/OrganismEvolution)

## Getting Help

- Review [SETUP_GUIDE.md](SETUP_GUIDE.md) for detailed instructions
- Check [PROJECT_PLAN.md](PROJECT_PLAN.md) for architecture details
- Open an issue on GitHub if you encounter problems

---

Happy coding! 🧬
