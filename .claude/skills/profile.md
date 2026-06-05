---
name: profile
description: Profile the WebServer for CPU, memory, and I/O bottlenecks. Use when investigating performance issues or optimizing hot paths.
---

# Performance Profiling Skill

Profile and optimize the WebServer.

## CPU Profiling

### perf (Linux)

```bash
# Record CPU profile (30 seconds)
perf record -g -p $(pgrep server) -- sleep 30

# Interactive report
perf report

# Flame graph
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

### gprof (compile with `-pg`)

```bash
# CMakeLists.txt: add -pg to CMAKE_CXX_FLAGS
cmake .. -DCMAKE_CXX_FLAGS="-pg -O2"
make
./server 8080
# After running:
gprof server gmon.out > profile.txt
```

## Memory Profiling

### Valgrind

```bash
# Memory leak check
valgrind --leak-check=full --show-leak-kinds=all ./server 8080

# Memory usage tracking
valgrind --tool=massif ./server 8080
ms_print massif.out.<pid>
```

### AddressSanitizer

```bash
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address -g"
make
./server 8080
```

### ThreadSanitizer

```bash
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
make
./server 8080
```

## I/O Profiling

### strace

```bash
# Trace system calls
strace -c -p $(pgrep server)

# Trace specific calls
strace -e trace=read,write,accept,epoll_wait -p $(pgrep server)
```

### iostat

```bash
iostat -x 1
```

## Hot Path Analysis

Focus profiling on these critical paths:

1. **epoll_wait** → event dispatching
2. **read/recv** → request parsing
3. **HTTP parser** → state machine transitions
4. **write/send** → response sending
5. **Thread pool** → task queue contention

## Common Bottlenecks

| Symptom               | Likely Cause              | Fix                          |
|-----------------------|---------------------------|------------------------------|
| High CPU in epoll     | Too many events           | Use EPOLLONESHOT             |
| High CPU in parser    | String copies             | Use string_view              |
| High latency          | Lock contention           | Lock-free queue              |
| Memory growth         | Connection leak           | Check timer/timeout          |
| Low throughput        | Single-threaded response  | Move to thread pool          |

## Optimization Checklist

- [ ] Compile with `-O2` or `-O3`
- [ ] Use `string_view` instead of `string` for reads
- [ ] Avoid allocations in hot path (pre-allocate buffers)
- [ ] Use `writev` (scatter-gather I/O) for headers + body
- [ ] Minimize lock scope in thread pool
- [ ] Use `EPOLLONESHOT` to avoid redundant wakeups
- [ ] Profile with real workload, not synthetic

## Benchmark Before/After

Always compare before and after optimization:

```
Before: 30,000 req/s, 5ms avg latency
After:  52,000 req/s, 3ms avg latency
Improvement: +73% throughput, -40% latency
```
