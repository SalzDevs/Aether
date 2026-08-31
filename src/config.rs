//! YAML configuration loading: sensor registry and persisted settings.

use crate::sensor::{truncate_bytes, SensorData, MAX_METRICS, SENSOR_NAME_MAX};
use serde::Deserialize;
use std::fs;

pub const AETHER_SETTINGS_FILE: &str = "config/settings.yaml";

// ---------------------------------------------------------------------------
// Sensor config
// ---------------------------------------------------------------------------

// Intermediate representation of one metric entry in the YAML file.
#[derive(Deserialize)]
struct MetricSpec {
    name: Option<String>,
    unit: Option<String>,
    value: Option<f32>,
}

// Intermediate representation of one sensor entry in the YAML file.
// Unknown keys are ignored (serde's default), mirroring the C parser's
// `skipNode` behavior.
#[derive(Deserialize)]
struct SensorSpec {
    id: Option<i32>,
    name: Option<String>,
    metrics: Option<Vec<MetricSpec>>,
}

#[derive(Deserialize)]
struct SensorRoot {
    sensors: Option<Vec<SensorSpec>>,
}

/// Loads the sensor registry from a YAML file of the form:
///
/// ```yaml
/// sensors:
///   - id: 1
///     name: temperature_sensor
///     metrics:
///       - name: temperature
///         unit: C
///         value: 22.5
/// ```
///
/// Returns an error when the file is missing, malformed, has no `sensors`
/// key, or yields no usable sensors. Sensors without an id are parsed and
/// dropped; metrics beyond [`MAX_METRICS`] are parsed and dropped.
pub fn load_sensor_config(file_name: &str) -> Result<Vec<SensorData>, String> {
    let text = fs::read_to_string(file_name)
        .map_err(|_| format!("cannot open {file_name}"))?;
    file_to_sensors(&text)
}

pub fn file_to_sensors(yaml: &str) -> Result<Vec<SensorData>, String> {
    let root: SensorRoot =
        serde_yaml::from_str(yaml).map_err(|e| format!("invalid yaml: {e}"))?;
    let specs = root.sensors.ok_or("no sensors key")?;
    if specs.is_empty() {
        return Err("no sensors".into());
    }

    let mut sensors = Vec::new();
    for spec in specs {
        // a sensor without an id is not usable
        let Some(id) = spec.id else { continue };
        let mut s = SensorData::new();
        s.id = id;
        s.name = truncate_bytes(spec.name.as_deref().unwrap_or(""), SENSOR_NAME_MAX);
        for metric in spec.metrics.unwrap_or_default().iter().take(MAX_METRICS) {
            // metrics with a missing/empty name are rejected by add_metric
            s.add_metric(
                metric.name.as_deref().unwrap_or(""),
                metric.unit.as_deref(),
                metric.value.unwrap_or(0.0),
            );
        }
        sensors.push(s);
    }

    if sensors.is_empty() {
        return Err("no usable sensors".into());
    }
    Ok(sensors)
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Settings {
    pub theme: i32,
    pub show_sparklines: bool,
    pub animate_values: bool,
    pub show_indicators: bool,
}

impl Default for Settings {
    fn default() -> Self {
        Settings {
            theme: 0,
            show_sparklines: true,
            animate_values: true,
            show_indicators: true,
        }
    }
}

/// Reads a scalar that must be a strict boolean: `true`/`1` or `false`/`0`.
/// YAML 1.1 spellings like `yes`/`no` are rejected, mirroring the C parser.
fn read_strict_bool(value: &serde_yaml::Value) -> Result<bool, String> {
    match value {
        serde_yaml::Value::Bool(b) => Ok(*b),
        serde_yaml::Value::Number(n) if n.as_u64() == Some(1) => Ok(true),
        serde_yaml::Value::Number(n) if n.as_u64() == Some(0) => Ok(false),
        serde_yaml::Value::String(s) => match s.as_str() {
            "true" | "1" => Ok(true),
            "false" | "0" => Ok(false),
            _ => Err(format!("not a recognized boolean: {s}")),
        },
        _ => Err("not a recognized boolean".into()),
    }
}

/// Loads settings from a YAML mapping. Unknown keys are tolerated; missing
/// keys keep their defaults. Returns an error when the file is missing or
/// malformed (the caller then falls back to defaults).
pub fn load_settings(file_name: &str) -> Result<Settings, String> {
    let text = fs::read_to_string(file_name)
        .map_err(|_| format!("cannot open {file_name}"))?;
    settings_from_str(&text)
}

pub fn settings_from_str(yaml: &str) -> Result<Settings, String> {
    // start from defaults so missing keys fall back gracefully
    let mut out = Settings::default();

    let value: serde_yaml::Value =
        serde_yaml::from_str(yaml).map_err(|e| format!("invalid yaml: {e}"))?;
    let mapping = value.as_mapping().ok_or("not a mapping")?;

    for (key, val) in mapping {
        let key = key.as_str().unwrap_or("");
        match key {
            "theme" => {
                out.theme = val
                    .as_i64()
                    .map(|v| v as i32)
                    .or_else(|| val.as_str().and_then(|s| s.parse().ok()))
                    .ok_or("invalid theme value")?;
            }
            "trend_lines" => out.show_sparklines = read_strict_bool(val)?,
            "animated_values" => out.animate_values = read_strict_bool(val)?,
            "change_indicators" => out.show_indicators = read_strict_bool(val)?,
            _ => {} // tolerate unknown keys
        }
    }
    Ok(out)
}

/// Saves settings as a YAML mapping with the same keys the C emitter wrote.
pub fn save_settings(file_name: &str, s: &Settings) -> Result<(), String> {
    let content = format!(
        "theme: {}\ntrend_lines: {}\nanimated_values: {}\nchange_indicators: {}\n",
        s.theme, s.show_sparklines, s.animate_values, s.show_indicators
    );
    fs::write(file_name, content)
        .map_err(|e| format!("cannot write {file_name}: {e}"))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::sensor::SENSOR_NAME_MAX;

    fn value_of(s: &SensorData, name: &str) -> f32 {
        s.find_metric(name).unwrap().value
    }

    const SAMPLE_YAML: &str = r#"
sensors:
  - id: 1
    name: temperature_sensor
    metrics:
      - name: temperature
        unit: C
        value: 22.5
      - name: humidity
        unit: "%"
        value: 45.0
  - id: 2
    name: barometric_sensor
    metrics:
      - name: pressure
        unit: hPa
        value: 1013.25
  - id: 3
    name: power_sensor
    metrics:
      - name: voltage
        unit: V
        value: 12.0
      - name: current
        unit: A
        value: 3.5
  - id: 4
    name: audio_sensor
    metrics:
      - name: loudness
        unit: dB
        value: 65.0
  - id: 5
    name: motion_sensor
    metrics:
      - name: acceleration
        unit: m/s2
        value: 9.81
      - name: frequency
        unit: Hz
        value: 50.0
"#;

    #[test]
    fn file_to_sensors_parses_sample() {
        let sensors = file_to_sensors(SAMPLE_YAML).unwrap();
        assert_eq!(sensors.len(), 5);

        assert_eq!(sensors[0].id, 1);
        assert_eq!(sensors[0].name, "temperature_sensor");
        assert_eq!(value_of(&sensors[0], "temperature"), 22.5);
        assert_eq!(value_of(&sensors[0], "humidity"), 45.0);

        let humidity = sensors[0].find_metric("humidity").unwrap();
        assert_eq!(humidity.unit, "%");
        assert_eq!(humidity.initial_value, 45.0);

        let pressure = sensors[1].find_metric("pressure").unwrap();
        assert_eq!(pressure.value, 1013.25);
        assert_eq!(pressure.unit, "hPa");

        assert_eq!(sensors[2].id, 3);
        assert_eq!(value_of(&sensors[2], "voltage"), 12.0);
        assert_eq!(value_of(&sensors[2], "current"), 3.5);

        assert_eq!(sensors[4].id, 5);
        assert_eq!(value_of(&sensors[4], "acceleration"), 9.81);
        assert_eq!(value_of(&sensors[4], "frequency"), 50.0);
    }

    #[test]
    fn many_sensors_grow_registry() {
        // The old parser silently dropped sensors past a hard cap (32).
        // Generate 40 and make sure every one of them is loaded.
        let mut yaml = String::from("sensors:\n");
        for i in 1..=40 {
            yaml.push_str(&format!(
                "  - id: {i}\n    name: sensor_{i}\n    metrics:\n      - name: value\n        unit: u\n        value: {}\n",
                i * 10
            ));
        }

        let sensors = file_to_sensors(&yaml).unwrap();
        assert_eq!(sensors.len(), 40);
        assert_eq!(sensors[0].id, 1);
        assert_eq!(value_of(&sensors[0], "value"), 10.0);
        assert_eq!(sensors[39].id, 40);
        assert_eq!(value_of(&sensors[39], "value"), 400.0);
    }

    #[test]
    fn sensors_without_id_are_skipped() {
        let yaml = r#"
sensors:
  - name: no_id_here
  - id: 7
    name: valid
"#;
        let sensors = file_to_sensors(yaml).unwrap();
        assert_eq!(sensors.len(), 1);
        assert_eq!(sensors[0].id, 7);
    }

    #[test]
    fn sensor_without_metrics_is_accepted() {
        let yaml = "sensors:\n  - id: 1\n    name: bare_sensor\n";
        let sensors = file_to_sensors(yaml).unwrap();
        assert_eq!(sensors.len(), 1);
        assert!(sensors[0].metrics.is_empty());
    }

    #[test]
    fn long_names_are_truncated_safely() {
        let yaml = "sensors:\n  - id: 1\n    name: a_very_long_sensor_name_well_past_the_limit_for_testing\n";
        let sensors = file_to_sensors(yaml).unwrap();
        assert_eq!(sensors.len(), 1);
        assert_eq!(sensors[0].name.len(), SENSOR_NAME_MAX);
    }

    #[test]
    fn unknown_keys_are_ignored() {
        let yaml = r#"
version: 2
sensors:
  - id: 1
    name: weird
    location: basement
    tags: [a, b]
    metrics:
      - name: v
        unit: u
        value: 1
        calibration: 0.98
"#;
        let sensors = file_to_sensors(yaml).unwrap();
        assert_eq!(sensors.len(), 1);
        assert_eq!(value_of(&sensors[0], "v"), 1.0);
        assert!(sensors[0].find_metric("calibration").is_none());
    }

    #[test]
    fn malformed_yaml_fails_cleanly() {
        let yaml = "sensors:\n  - id: %%%not valid yaml at all[[[\n";
        assert!(file_to_sensors(yaml).is_err());
    }

    #[test]
    fn header_only_yields_no_sensors() {
        assert!(file_to_sensors("sensors:\n").is_err());
    }

    #[test]
    fn load_sensor_config_missing_file() {
        assert!(load_sensor_config("config/does_not_exist.yaml").is_err());
    }

    #[test]
    fn load_sensor_config_from_real_file() {
        let sensors = load_sensor_config("config/sensors.yaml").unwrap();
        assert_eq!(sensors.len(), 5);
        assert_eq!(sensors[0].id, 1);
    }

    #[test]
    fn load_settings_missing_file_returns_err() {
        assert!(load_settings("settings_test_missing.yaml").is_err());
    }

    #[test]
    fn settings_roundtrip_preserves_all_fields() {
        for theme in 0..=4i32 {
            for bits in 0..8u8 {
                let s = Settings {
                    theme,
                    show_sparklines: bits & 1 != 0,
                    animate_values: bits & 2 != 0,
                    show_indicators: bits & 4 != 0,
                };
                let yaml = {
                    let path = std::env::temp_dir().join("aether_settings_roundtrip.yaml");
                    save_settings(path.to_str().unwrap(), &s).unwrap();
                    std::fs::read_to_string(&path).unwrap()
                };
                let out = settings_from_str(&yaml).unwrap();
                assert_eq!(out.theme, s.theme);
                assert_eq!(out.show_sparklines, s.show_sparklines);
                assert_eq!(out.animate_values, s.animate_values);
                assert_eq!(out.show_indicators, s.show_indicators);
            }
        }
    }

    #[test]
    fn backward_compat_theme_only() {
        let out = settings_from_str("theme: 4\n").unwrap();
        assert_eq!(out.theme, 4);
        assert!(out.show_sparklines);
        assert!(out.animate_values);
        assert!(out.show_indicators);
    }

    #[test]
    fn tolerates_unknown_keys() {
        let yaml = "theme: 1\nfoo: bar\nanimated_values: false\nnested:\n  a: 1\n  b: 2\n";
        let out = settings_from_str(yaml).unwrap();
        assert_eq!(out.theme, 1);
        assert!(out.show_sparklines); // unknown keys ignored, defaults kept
        assert!(!out.animate_values);
        assert!(out.show_indicators);
    }

    #[test]
    fn read_bool_accepts_1_and_0() {
        let yaml = "theme: 0\nanimated_values: 0\nchange_indicators: 1\n";
        let out = settings_from_str(yaml).unwrap();
        assert!(!out.animate_values);
        assert!(out.show_indicators);
    }

    #[test]
    fn invalid_bool_value_fails() {
        let yaml = "theme: 0\ntrend_lines: yes\n";
        assert!(settings_from_str(yaml).is_err());
    }

    #[test]
    fn corrupt_yaml_fails() {
        assert!(settings_from_str("just a scalar, not a mapping\n").is_err());
    }

    #[test]
    fn save_unwritable_path_returns_err() {
        let s = Settings::default();
        assert!(save_settings("/no_such_dir_x7y9/settings.yaml", &s).is_err());
    }
}
