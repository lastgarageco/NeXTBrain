# NeXTBrain

NeXTBrain is a modular AI server inspired by the NeXT philosophy: elegant, understandable, extensible, and built to bridge classic computing with modern artificial intelligence.

## Current Status

NeXTBrain is in active development.

Current capabilities:

- Modular AI provider architecture.
- Ollama provider implementation.
- TCP server for lightweight clients.
- Local-first design.
- Successfully tested from:
  - macOS using `nc`
  - NeXTSTEP using `telnet`

Current goals:

- Remain model-agnostic.
- Support modern clients through simple APIs.
- Support classic computers through lightweight, documented protocols.
- Grow one small, verified step at a time.

## First AI Provider

The first AI provider is Ollama running Qwen2.5-Coder:7B.

This is an implementation choice, not a permanent dependency.

## Milestones

### Milestone 1 — NeXTBrain Server ✅

- Provider architecture completed.
- Ollama integration completed.
- TCP protocol implemented.
- First successful communication from macOS.
- First successful communication from a NeXTstation.

## Philosophy

See [MANIFESTO.md](MANIFESTO.md).

## Architecture

See [docs/Architecture.md](docs/Architecture.md).
