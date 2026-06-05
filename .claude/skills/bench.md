---
name: bench
description: Run stress tests and benchmarks against the WebServer. Use when asked to benchmark, stress test, or measure throughput/latency.
---

# Benchmark Skill

Stress test the WebServer to measure performance.

## Tools

### wrk (recommended)

```bash
# Install
sudo apt install wrk

# Basic benchmark — 4 threads, 1000 connections, 30s
wrk -t4 -c1000 -d30s http://localhost:8080/

# With keep-alive
wrk -t4 -c1000 -d30s -H "Connection: Keep-Alive" http://localhost:8080/

# Custom script (POST, etc.)
wrk -t4 -c1000 -d30s -s post.lua http://localhost:8080/api
```

### ab (Apache Bench)

```bash
# 10000 requests, 100 concurrent
ab -n 10000 -c 100 http://localhost:8080/

# Keep-alive
ab -n 10000 -c 100 -k http://localhost:8080/
```

### WebBench (for high concurrency)

```bash
./webbench -c 1000 -t 30 http://localhost:8080/
```

## Key Metrics

| Metric              | Target          | Description                    |
|---------------------|-----------------|--------------------------------|
| Requests/sec        | > 50,000        | Throughput                     |
| Avg latency         | < 5ms           | Mean response time             |
| P99 latency         | < 50ms          | 99th percentile                |
| Error rate          | 0%              | No failed requests             |
| Max connections     | > 10,000        | Concurrent connections handled |

## Test Scenarios

1. **Static file** — `GET /index.html` (small file)
2. **Hello World** — `GET /` (no disk I/O)
3. **Keep-alive** — persistent connections
4. **Short-lived** — `Connection: close`
5. **Large response** — 1MB+ body
6. **POST** — with request body
7. **Mixed workload** — random paths

## Before Running

```bash
# Increase file descriptor limit
ulimit -n 65535

# Check current limit
ulimit -n

# Optional: tune kernel
sudo sysctl -w net.core.somaxconn=65535
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=65535
```

## Profiling During Benchmark

```bash
# CPU profiling with perf
perf record -g -p $(pgrep server) -- sleep 30
perf report

# Or with gprof (compile with -pg)
# Run benchmark, then:
gprof build/server gmon.out > analysis.txt
```

## Output Format

Report results as:
```
===== Benchmark Results =====
Tool:    wrk
Threads: 4
Conns:   1000
Duration: 30s

Requests/sec:  52,341
Avg Latency:   3.2ms
P99 Latency:   12ms
Transfer/sec:  8.5MB
Errors:        0
============================
```

## Important

- Run server and benchmark on different machines for accurate results (or at least different processes)
- Warm up the server for 5s before measuring
- Run each test 3 times and report the median
- Monitor memory usage during the test (leak detection)
