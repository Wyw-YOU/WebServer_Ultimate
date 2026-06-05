---
name: build
description: Build the WebServer project with CMake. Use when asked to compile, build, or make the project.
---

# Build Skill

Build the WebServer project using CMake.

## Steps

1. Ensure the build directory exists:
   ```bash
   mkdir -p build && cd build
   ```

2. Run CMake configure:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. Build with all cores:
   ```bash
   cmake --build . -j$(nproc)
   ```

## Build Types

- **Debug**: `cmake .. -DCMAKE_BUILD_TYPE=Debug` — includes debug symbols, no optimization
- **Release**: `cmake .. -DCMAKE_BUILD_TYPE=Release` — full optimization (`-O2`)

## Troubleshooting

- If CMake version error: check `cmake_minimum_required` in CMakeLists.txt
- If missing headers: ensure `include/` path is correct in CMakeLists.txt
- If linker errors: check that all `.cpp` files are included in the build

## Output

The compiled binary is `build/server`.
