---
name: clean
description: Clean build artifacts and reset the build directory. Use when asked to clean, rebuild from scratch, or remove build files.
---

# Clean Skill

Remove build artifacts and prepare for a fresh build.

## Full Clean

```bash
rm -rf build/
mkdir build
```

## CMake Clean

```bash
cd build
cmake --build . --target clean
```

## Deep Clean (including CMake cache)

```bash
rm -rf build/CMakeCache.txt build/CMakeFiles/ build/Makefile
cd build
cmake ..
```

## Clean Everything (git clean)

```bash
# Remove all untracked files (DANGEROUS — preview first)
git clean -fd --dry-run
git clean -fd
```
