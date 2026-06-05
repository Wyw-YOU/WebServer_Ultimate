---
name: ci
description: Set up or run CI/CD pipeline (GitHub Actions). Use when asked about CI, GitHub Actions, automated builds, or testing.
---

# CI Skill

Set up GitHub Actions for automated build and test.

## Workflow File

`.github/workflows/build.yml`:

```yaml
name: Build and Test

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake g++ libmysqlclient-dev

    - name: Configure
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release

    - name: Build
      run: cmake --build build -j$(nproc)

    - name: Test
      run: cd build && ctest --output-on-failure

    - name: Sanitizers
      run: |
        cmake -B build-sanitize \
          -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
        cmake --build build-sanitize -j$(nproc)
        cd build-sanitize && ctest --output-on-failure
```

## Checklist

- [ ] Build succeeds on Ubuntu (latest)
- [ ] Build succeeds with `-Wall -Wextra -Werror`
- [ ] Tests pass
- [ ] Sanitizers (ASan, UBSan) pass
- [ ] No memory leaks (Valgrind optional)
