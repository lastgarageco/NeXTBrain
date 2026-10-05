# NeXTBrain

NeXTBrain is a modular AI server inspired by the NeXT philosophy: elegant, understandable, extensible, and built to bridge classic computing with modern artificial intelligence.

## Current Status

NeXTBrain version **0.0.1** is in active development.

Current capabilities:

- Modular, provider-oriented server architecture.
- Ollama provider implementation.
- TCP server for lightweight clients.
- Portable C client running on NeXTSTEP and modern macOS.
- Simple client shell with `HELP`, `ASK`, `STATUS`, and `QUIT` / `EXIT` / `BYE`.
- `EOT` / `EOL` to leave the client's conversation mode.
- End-to-end `STATUS` reporting live server version, status, provider, model, and the actual server IP address and port for the current connection.
- Local-first design.

The client's conversation mode supports repeated requests; the server does not yet retain conversation history or state.

Current goals:

- Remain model-agnostic.
- Support modern clients through simple APIs.
- Support classic computers through lightweight, documented protocols.
- Grow one small, verified step at a time.

## Configuration

- Client: `nbclient.conf` with `HOST` and `PORT`.
- Server: `nbserver.conf` with `HOST`, `PORT`, `PROVIDER`, and `MODEL`.

Both `nbclient.conf.example` and `nbserver.conf.example` are committed to the repository as starting points for local configuration.

## First AI Provider

The current AI provider is Ollama, with `qwen3:8b` as the configured model.

This is an implementation choice, not a permanent dependency.

## Milestones

### Milestone 1 — NeXTBrain Server ✅

- Provider architecture completed.
- Ollama integration completed.
- TCP protocol implemented.
- First successful communication from macOS.
- First successful communication from a NeXTstation.

### Portable Client and Server Status ✅

- C client running on NeXTSTEP and modern macOS.
- Client shell and configuration files implemented.
- Live server status available through the client.

## Philosophy

See [MANIFESTO.md](MANIFESTO.md).

## Architecture

See [docs/Architecture.md](docs/Architecture.md).