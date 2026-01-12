# Setup Guide for OrganismEvolution

This guide walks you through setting up the development environment using MSYS2 on Windows.

## Step 1: Install MSYS2

1. Download MSYS2 installer from [https://www.msys2.org/](https://www.msys2.org/)
2. Run the installer (e.g., `msys2-x86_64-xxxxxxxx.exe`)
3. Follow the installation wizard (default location: `C:\msys64`)
4. After installation, launch "MSYS2 MinGW 64-bit" from the Start Menu

## Step 2: Update MSYS2

Open the MSYS2 MinGW 64-bit terminal and run:

```bash
pacman -Syu
```

If it prompts to close the terminal, do so and reopen it, then run:

```bash
pacman -Su
```

## Step 3: Install Development Tools

Install the required packages for C++ development and OpenGL:

```bash
pacman -S mingw-w64-x86_64-toolchain
```

When prompted, press Enter to install all packages in the group.

```bash
pacman -S mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-glfw \
          mingw-w64-x86_64-glew \
          mingw-w64-x86_64-glm \
          mingw-w64-x86_64-mesa \
          git \
          make
```

## Step 4: Verify Installation

Check that everything is installed correctly:

```bash
gcc --version
g++ --version
cmake --version
git --version
```

You should see version information for each command.

## Step 5: Clone or Navigate to Project

If you haven't cloned the repository yet:

```bash
git clone https://github.com/CurioCrafter/OrganismEvolution.git
cd OrganismEvolution
```

If you're already in the project directory:

```bash
cd /c/Users/andre/Desktop/OrganismEvolution
```

## Step 6: Build the Project

Create a build directory and compile:

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## Step 7: Run the Simulator

After successful compilation:

```bash
./OrganismEvolution.exe
```

## Common Issues and Solutions

### Issue: "cmake: command not found"
**Solution**: Make sure you're using "MSYS2 MinGW 64-bit" terminal, not "MSYS2 MSYS" or "MSYS2 UCRT64".

### Issue: OpenGL linking errors
**Solution**: Ensure you have the correct OpenGL libraries:
```bash
pacman -S mingw-w64-x86_64-mesa
```

### Issue: GLFW or GLEW not found
**Solution**: Reinstall the packages:
```bash
pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

### Issue: "Permission denied" when running executable
**Solution**: Check if antivirus is blocking the executable. Add an exception for the build directory.

## Development Workflow

1. Make code changes in `src/`
2. Rebuild the project:
   ```bash
   cd build
   cmake --build .
   ```
3. Run and test:
   ```bash
   ./OrganismEvolution.exe
   ```

## IDE Setup (Optional)

### Visual Studio Code
1. Install "C/C++" extension by Microsoft
2. Install "CMake Tools" extension
3. Open the project folder in VS Code
4. Configure CMake to use MinGW:
   - Press `Ctrl+Shift+P`
   - Type "CMake: Select a Kit"
   - Choose "GCC for mingw-w64-x86_64"

### CLion
1. Open CLion
2. Go to Settings → Build, Execution, Deployment → Toolchains
3. Add MinGW toolchain pointing to `C:\msys64\mingw64`
4. Open the project's CMakeLists.txt

## Library Documentation

- **GLFW**: [https://www.glfw.org/documentation.html](https://www.glfw.org/documentation.html)
- **GLEW**: [http://glew.sourceforge.net/](http://glew.sourceforge.net/)
- **GLM**: [https://github.com/g-truc/glm](https://github.com/g-truc/glm)
- **OpenGL**: [https://www.opengl.org/](https://www.opengl.org/)

## Next Steps

1. Review [PROJECT_PLAN.md](PROJECT_PLAN.md) for the development roadmap
2. Check [README.md](README.md) for project overview
3. Start with Phase 1 implementation (basic rendering)

## Getting Help

- **MSYS2 Documentation**: [https://www.msys2.org/docs/what-is-msys2/](https://www.msys2.org/docs/what-is-msys2/)
- **OpenGL Tutorial**: [https://learnopengl.com/](https://learnopengl.com/)
- **CMake Documentation**: [https://cmake.org/documentation/](https://cmake.org/documentation/)
- **Project Issues**: [https://github.com/CurioCrafter/OrganismEvolution/issues](https://github.com/CurioCrafter/OrganismEvolution/issues)
