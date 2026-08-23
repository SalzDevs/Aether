# Aether
<p align="center">
  <img src="logo.png" width="180" alt="logo">
</p>

Aether is a small C application that simulates sensor readings and displays them in a real-time Raylib dashboard.

## Features

- Load sensor data from YAML configuration
- Simulate changing sensor values
- Schedule periodic sensor updates
- Store readings in a FIFO queue
- Display sensor values with Raylib
- Run unit tests for configuration, queue, scheduler, and sensor modules

## Requirements

- C compiler with C99 support
- `make`
- `pkg-config`
- Raylib

### macOS

```sh
brew install raylib libyaml pkg-config
```

### Linux

Install your distribution's development packages for:

- GCC or Clang
- Make
- `pkg-config`
- Raylib

For example, package names may include `build-essential`, `pkg-config`, `libraylib-dev`, and `libyaml-dev`.

## Build and run

Build the application:

```sh
make
```

Run it:

```sh
./main
```

Clean generated files:

```sh
make clean
```

## Run tests

Build and run all unit tests:

```sh
make test
```

The test suite covers:

- sensor initialization and simulated updates;
- queue insertion, removal, and growth;
- task scheduling;
- YAML configuration loading.

## Configuration

Sensor data is loaded from:

```text
config/sensors.yaml
```

Example:

```yaml
sensors:
  - id: 1
    name: temperature_sensor
    metrics:
      - name: temperature
        unit: C
        value: 22.5
      - name: humidity
        unit: "%"
        value: 45
```

Each sensor requires:

| Field | Description |
| --- | --- |
| `id` | Unique numeric sensor identifier |
| `name` | Readable sensor name |
| `metrics` | List of metrics, each with `name`, `unit` and `value` |

Note: a `%` unit must be quoted (`"%"`) because libyaml reserves the percent sign.


## Project structure

```text
.
├── config/       Sensor configuration loading and YAML data
├── Queue/        FIFO sensor-data queue
├── scheduler/    Periodic task scheduling
├── sensor/       Sensor data types and simulation
├── ui/           Dashboard rendering (Raylib)
├── tests/        Unit tests
├── main.c        Application entry point and Raylib dashboard
└── Makefile      Build and test commands
```

## Architecture

```text
sensors.yaml
     │
     ▼
configuration parser
     │
     ▼
sensor data queue ◄── scheduled sensor tasks
     │
     ▼
Raylib sensor dashboard
```

## License

This project is licensed under the [MIT License](LICENSE).
