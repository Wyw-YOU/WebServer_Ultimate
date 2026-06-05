---
name: new-module
description: Scaffold a new C++ module (header + source) following the project's conventions. Use when creating a new component like ThreadPool, Timer, HttpParser, etc.
---

# New Module Skill

Create a new header/source pair following project conventions.

## File Layout

```
include/<ModuleName>.hpp   — class declaration
src/<ModuleName>.cpp       — implementation
```

## Header Template (`include/<ModuleName>.hpp`)

```cpp
#pragma once

// Standard includes
#include <string>
#include <memory>

class <ModuleName> {
public:
    <ModuleName>();
    ~<ModuleName>();

    // Public interface

private:
    // Internal state
};
```

## Source Template (`src/<ModuleName>.cpp`)

```cpp
#include "<ModuleName>.hpp"
#include "Log.hpp"

<ModuleName>::<ModuleName>() {
    LOG_DEBUG("<ModuleName> created");
}

<ModuleName>::~<ModuleName>() {
    LOG_DEBUG("<ModuleName> destroyed");
}
```

## Conventions

- Use `#pragma once` for include guards
- Include `"Log.hpp"` in source files for logging
- Use C++20 features where appropriate (concepts, ranges, coroutines, etc.)
- Place all declarations in `include/`, all implementations in `src/`
- Class names: PascalCase. Methods: PascalCase. Members: snake_case with trailing underscore
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers
- Use `std::string_view` for read-only string parameters

## After Creating

Add the new `.cpp` file to CMakeLists.txt if not using `aux_source_directory` (already auto-collects from `src/`).
