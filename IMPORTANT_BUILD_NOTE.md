# IMPORTANT: How to Build This Project

## You MUST Use MSYS2 MinGW 64-bit Terminal

This project **cannot** be built from:
- Windows Command Prompt (cmd.exe)
- Windows PowerShell
- Git Bash
- Regular MSYS2 terminal
- MSYS2 UCRT64 terminal

## Correct Build Environment

**Use ONLY**: **MSYS2 MinGW 64-bit** terminal

### How to Open the Correct Terminal

1. Press Windows key
2. Type "MSYS2 MinGW 64-bit"
3. Click on the application

The terminal title bar should show: **MINGW64**

### Verify You're in the Right Environment

In the terminal, you should see a prompt like:
```
username@computername MINGW64 ~
$
```

The key indicator is **MINGW64** in the prompt.

## Quick Build Instructions

Once you have the correct terminal open:

```bash
# Navigate to project
cd /c/Users/andre/Desktop/OrganismEvolution

# Create build directory
mkdir build
cd build

# Configure
cmake .. -G "MinGW Makefiles"

# Build
cmake --build .

# Run
./OrganismEvolution.exe
```

## Why This Matters

MSYS2 MinGW 64-bit provides:
- GCC C++ compiler (g++)
- MinGW-w64 toolchain
- OpenGL development libraries (GLFW, GLEW, GLM)
- CMake build system
- Unix-like tools (make, etc.)

Regular Windows terminals don't have these tools available.

## If You Haven't Installed MSYS2 Yet

1. Download from https://www.msys2.org/
2. Install it
3. Follow the setup guide in [SETUP_GUIDE.md](SETUP_GUIDE.md)

## For More Information

- **Detailed Setup**: See [SETUP_GUIDE.md](SETUP_GUIDE.md)
- **Build Instructions**: See [BUILD_AND_RUN.md](BUILD_AND_RUN.md)
- **Quick Start**: See [QUICK_START.md](QUICK_START.md)

---

**Remember**: Always use **MSYS2 MinGW 64-bit** terminal for this project!
