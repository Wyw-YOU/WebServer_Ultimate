---
name: run
description: Run the WebServer on a given port. Use when asked to start, run, or launch the server.
---

# Run Skill

Launch the compiled WebServer binary.

## Usage

```bash
cd build && ./server <port>
```

Default port if not specified: **8080**.

## Quick Test

After starting the server, test with curl:

```bash
curl -v http://localhost:8080/
```

## Common Ports

- `8080` — standard HTTP dev port
- `80` — standard HTTP (may need root)
- `443` — HTTPS (not yet implemented)

## Notes

- The server currently runs in the foreground. Use `Ctrl+C` to stop.
- If port is in use: `lsof -i :<port>` or `ss -tlnp | grep <port>` to find the process.
- For background mode: `./server 8080 &`
