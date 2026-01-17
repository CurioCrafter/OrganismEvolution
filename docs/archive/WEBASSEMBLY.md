# Building for Web (WebAssembly)

You can compile OrganismEvolution to run directly in a web browser using Emscripten!

## Why WebAssembly?

- **No installation needed** - Runs in any modern browser
- **Cross-platform** - Works on Windows, Mac, Linux, even mobile
- **Easy sharing** - Just send someone a URL
- **Claude-friendly** - No special build environment needed

## Setup Emscripten

```bash
# Clone Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest version
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # On Windows: emsdk_env.bat
```

## Modify CMakeLists.txt for Emscripten

Add this section to CMakeLists.txt:

```cmake
# Emscripten-specific settings
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --preload-file ../shaders@/shaders")
endif()
```

## Build for Web

```bash
cd OrganismEvolution
mkdir build-web && cd build-web

# Configure with Emscripten
emcmake cmake ..

# Build
emmake make

# Serve locally
python3 -m http.server 8000
```

Then open http://localhost:8000/OrganismEvolution.html in your browser!

## Benefits for Future Claude Sessions

1. **No MinGW needed** - Builds anywhere Emscripten runs
2. **Instant testing** - See results in browser immediately
3. **Easy deployment** - Host on GitHub Pages, Netlify, etc.
4. **Mobile compatible** - Touch controls possible
5. **Shareable** - Anyone can try it instantly

## GitHub Pages Deployment

```yaml
# Add to .github/workflows/deploy-web.yml
name: Deploy to GitHub Pages

on:
  push:
    branches: [ main ]

jobs:
  build-deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Setup Emscripten
        uses: mymindstorm/setup-emsdk@v12

      - name: Build for Web
        run: |
          mkdir build-web
          cd build-web
          emcmake cmake ..
          emmake make

      - name: Deploy to GitHub Pages
        uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./build-web
```

Then your simulator would be live at:
**https://curiocrafter.github.io/OrganismEvolution/**

## Code Changes Needed

Minimal changes required:
1. File I/O must use Emscripten's virtual filesystem
2. Shader loading needs to use preloaded files
3. Mouse capture works differently in browser
4. Performance may vary (but usually 60 FPS is achievable)

This would make the project **universally accessible** without any build tools!
