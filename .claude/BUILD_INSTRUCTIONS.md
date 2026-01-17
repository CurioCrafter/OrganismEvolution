# Build Instructions for Future Claude Sessions

## CRITICAL: How to Compile This Project

**DO NOT ask the user to compile the project themselves.** You have full access to compile via command line.

### Environment Setup

This project uses **MSVC** (Visual Studio) with DirectX 12. Use a Visual Studio
Developer Prompt or Developer PowerShell.

### Complete Build Command (Single Line)

```batch
cmake --build c:/Users/andre/Desktop/OrganismEvolution/build_dx12 --config Debug
```

### Clean Rebuild (When CMake Configuration Changes)

```batch
cd c:/Users/andre/Desktop/OrganismEvolution
rmdir /s /q build_dx12
cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
cmake --build build_dx12 --config Debug
```

### Key Details

1. **Use MSVC** - This is a Windows DX12 project.
2. **Generator:** Visual Studio (e.g., "Visual Studio 17 2022")
3. **Shaders:** CMake copies `Runtime/Shaders` next to the executable after build.

### Handling Compilation Errors

When you see compilation errors:
1. **Read the error carefully** - Look for the actual error (not warnings)
2. **Fix the code** - Use Edit tool to fix the issue
3. **Rebuild immediately** - Run the build command again
4. **Iterate** - Continue until build succeeds

### Running the Built Executable

After successful build:
```batch
c:/Users/andre/Desktop/OrganismEvolution/build_dx12/Debug/OrganismEvolution.exe
```

## User Preference

**The user does NOT want to manually compile.** They expect Claude to:
1. Write/modify code
2. Compile it automatically using the commands above
3. Report any errors and fix them
4. Continue until the build succeeds

**Quote from user:**
> "you compile it also make a note for future claude to understand you dont need me to compile every time fuck off with that"

## Build Process Summary

The typical workflow is:
1. Make code changes using Edit/Write tools
2. Run: `cmake --build c:/Users/andre/Desktop/OrganismEvolution/build_dx12 --config Debug`
3. If errors occur, fix them and go to step 2
4. When build succeeds, inform the user and optionally run the executable

**Never ask the user to open a terminal or run build commands manually.**
