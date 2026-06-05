---
name: review
description: Review C++ code for correctness, safety, and performance. Use when asked to review code, check for bugs, or audit a module.
---

# Code Review Skill

Review C++ code for the WebServer project.

## Check Categories

### 1. Memory Safety
- [ ] No raw `new`/`delete` — use smart pointers
- [ ] No dangling pointers or use-after-free
- [ ] No buffer overflows (check array bounds, string operations)
- [ ] RAII for all resource types (sockets, file handles, locks)
- [ ] Proper destructor cleanup

### 2. Thread Safety
- [ ] Shared data protected by mutex or is lock-free
- [ ] No data races (check with ThreadSanitizer)
- [ ] Lock ordering consistent to prevent deadlocks
- [ ] Condition variable usage: predicate check in while loop
- [ ] `std::atomic` for simple flags

### 3. Error Handling
- [ ] System calls checked for errors (`socket`, `bind`, `listen`, `accept`, `read`, `write`)
- [ ] `errno` checked immediately after system call
- [ ] Exceptions caught at thread boundaries
- [ ] Error paths release resources properly

### 4. Performance
- [ ] Pass large objects by `const&` or `string_view`
- [ ] Avoid unnecessary copies (`std::move` where appropriate)
- [ ] Pre-allocate containers when size is known
- [ ] Use `reserve()` for strings and vectors
- [ ] Hot path avoids allocations

### 5. API Design
- [ ] Clear ownership semantics (unique_ptr for exclusive, shared_ptr for shared)
- [ ] Consistent naming conventions (PascalCase for methods, snake_case_ for members)
- [ ] Minimal public interface
- [ ] `const` correctness (const methods, const parameters)

### 6. Modern C++
- [ ] `auto` where type is obvious
- [ ] Range-based for loops
- [ ] `std::optional` for nullable values
- [ ] `std::string_view` for read-only strings
- [ ] `enum class` instead of plain `enum`
- [ ] `constexpr` where possible

## Common Bugs to Watch For

```cpp
// BUG: fd leak on error path
int fd = socket(...);
if (bind(fd, ...) < 0) {
    perror("bind");
    return;  // ← fd leaked! Add close(fd) before return
}

// BUG: use-after-move
std::string s = "hello";
func(std::move(s));
std::cout << s;  // ← s is in moved-from state

// BUG: TOCTOU race
if (file_exists(path)) {
    // Another thread could delete the file here
    open(path);  // ← might fail
}

// BUG: wrong lock scope
std::lock_guard<std::mutex> lock(mtx_);
data_.push_back(item);
// ... expensive computation while holding lock ...
process(item);  // ← move this outside the lock
```

## Output Format

```
## Review: <module name>

### Critical Issues
1. **[file:line]** — description of bug

### Warnings
1. **[file:line]** — description of potential issue

### Suggestions
1. **[file:line]** — improvement suggestion

### Summary
- Critical: N
- Warnings: N
- Suggestions: N
```
