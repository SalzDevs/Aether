# Contributing to Aether

Thanks for your interest in contributing! Aether is a small, intentionally
simple C99 project — the goal is a readable, dependency-light codebase.

## Getting started

Requirements: a C99 compiler, `make`, `pkg-config`, [raylib](https://www.raylib.com/)
and libyaml.

```sh
git clone https://github.com/SalzDevs/Aether.git
cd Aether
make        # build the app
make test   # build and run the unit tests
./main      # run the dashboard
```

On macOS: `brew install raylib libyaml pkg-config`
On Linux: install `libraylib-dev` and `libyaml-dev` (names vary by distro).

## Where things live

| Path | Responsibility |
| --- | --- |
| `config/` | YAML config parsing (libyaml event API) |
| `sensor/` | The data model: a sensor is an id, a name, and a list of `{name, unit, value}` metrics |
| `History/` | Bounded per-sensor reading history (ring buffers) |
| `scheduler/` | Periodic task scheduling — decides *when*, never touches data structures |
| `ui/` | Dashboard rendering (Raylib), theme tokens, and Raylib-free layout math |
| `tests/` | One executable per module (`make test` runs them all) |

## Ground rules

- **C99, no warnings.** The build uses `-Wall -Wextra`; PRs with warnings won't pass CI.
- **Keep modules in their lane.** The scheduler must not touch data structures,
  the UI must not mutate sensor state, and nothing outside `sensor/` should
  know which metrics exist. When in doubt, look at how existing modules
  exchange data (registries, ring buffers, action structs).
- **Tests for logic.** Pure functions belong in `ui/layout.c` or the module
  they extend, with tests in `tests/test_*.c`. Rendering code is exempt.
- **Match the existing style**: 2-space indent, `snake_case` functions,
  `camelCase` locals, comments explain *why*.

## Submitting changes

1. Open or comment on an issue first if the change is architectural.
2. Create a feature branch.
3. Make sure `make test` passes and `make` builds without warnings.
4. Open a pull request with a short description of *what* and *why*.

Small, focused PRs get merged fastest.

## Good first issues

Check the [good first issue](https://github.com/SalzDevs/Aether/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
label — those are scoped to be small and self-contained.
