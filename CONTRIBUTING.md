# Contributing to Aether

Thanks for your interest in contributing! Aether is a small, intentionally
simple Rust project — the goal is a readable, dependency-light codebase.

## Getting started

Requirements: a Rust toolchain (install via [rustup](https://rustup.io)).

```sh
git clone https://github.com/SalzDevs/Aether.git
cd Aether
cargo build    # build the app
cargo test     # run the unit tests
cargo run      # run the dashboard
```

Raylib and serde-yaml are pulled in automatically by Cargo; there is no
system dependency to install.

## Where things live

| Path | Responsibility |
| --- | --- |
| `src/config.rs` | YAML config parsing (sensor registry and persisted settings) |
| `src/sensor.rs` | The data model: a sensor is an id, a name, and a list of `{name, unit, value}` metrics |
| `src/history.rs` | Bounded per-sensor reading history (ring buffers) |
| `src/scheduler.rs` | Periodic task scheduling — decides *when*, never touches data structures |
| `src/dashboard.rs` | Dashboard rendering (Raylib) |
| `src/layout.rs` | Raylib-free layout and formatting math |
| `src/theme.rs` | Theme tokens and presets |

## Ground rules

- **No warnings.** The build is checked with `cargo clippy -- -D warnings` in
  CI; PRs with warnings won't pass.
- **Keep modules in their lane.** The scheduler must not touch data structures,
  the UI must not mutate sensor state, and nothing outside `sensor.rs` should
  know which metrics exist. When in doubt, look at how existing modules
  exchange data (registries, ring buffers, action structs).
- **Tests for logic.** Pure functions live in `src/layout.rs` or the module
  they extend, with tests alongside in `#[cfg(test)]` blocks. Rendering code
  is exempt.
- **Match the existing style**: `rustfmt` formatting, `snake_case` functions,
  comments explain *why*.

## Submitting changes

1. Open or comment on an issue first if the change is architectural.
2. Create a feature branch.
3. Make sure `cargo test` passes and `cargo clippy --all-targets` is clean.
4. Open a pull request with a short description of *what* and *why*.

Small, focused PRs get merged fastest.

## Good first issues

Check the [good first issue](https://github.com/SalzDevs/Aether/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
label — those are scoped to be small and self-contained.
