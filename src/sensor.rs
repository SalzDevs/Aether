//! A minimal sensor identity plus an open-ended list of metrics.
//! Nothing outside the simulation knows which metrics exist.

use crate::rng;
use std::fmt;

/// Maximum number of metrics a single sensor can carry.
pub const MAX_METRICS: usize = 8;

/// Maximum stored length (excluding the C null terminator) of names/units.
/// The original C code used fixed char buffers, so strings were truncated
/// one byte early; the same limit is applied here for identical behavior.
pub const SENSOR_NAME_MAX: usize = 31;
pub const METRIC_NAME_MAX: usize = 31;
pub const METRIC_UNIT_MAX: usize = 15;

/// Truncates a string to at most `max` bytes without splitting UTF-8.
pub fn truncate_bytes(s: &str, max: usize) -> String {
    if s.len() <= max {
        return s.to_string();
    }
    let mut end = max;
    while !s.is_char_boundary(end) {
        end -= 1;
    }
    s[..end].to_string()
}

/// A single named measurement of a sensor, e.g. ("temperature", "C", 22.5).
#[derive(Clone, Debug, PartialEq)]
pub struct Metric {
    pub name: String,
    pub unit: String,
    pub value: f32,
    /// Value loaded from config; simulation models may reference it.
    pub initial_value: f32,
}

/// A sensor is an identity plus an open-ended list of metrics.
#[derive(Clone, Debug, PartialEq)]
pub struct SensorData {
    pub id: i32,
    pub name: String,
    pub metrics: Vec<Metric>,
    /// Timestamp of the last [`sensor_data_update`] call.
    pub last_update_s: f32,
}

/// A minimal sensor identity (kept for parity with the C `sensor` type).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Sensor {
    pub id: i32,
}

impl Sensor {
    pub fn new(id: i32) -> Self {
        Sensor { id }
    }
}

impl SensorData {
    pub fn new() -> Self {
        SensorData {
            id: 0,
            name: String::new(),
            metrics: Vec::new(),
            last_update_s: 0.0,
        }
    }

    /// Adds a metric with the given name, unit and initial value.
    /// The metric's value starts at `initial_value`.
    /// Returns false if the sensor is full or the name is missing/empty.
    pub fn add_metric(&mut self, name: &str, unit: Option<&str>, value: f32) -> bool {
        if name.is_empty() {
            return false;
        }
        if self.metrics.len() >= MAX_METRICS {
            return false;
        }
        self.metrics.push(Metric {
            name: truncate_bytes(name, METRIC_NAME_MAX),
            unit: truncate_bytes(unit.unwrap_or(""), METRIC_UNIT_MAX),
            value,
            initial_value: value,
        });
        true
    }

    /// Returns the metric with the given name, if present.
    pub fn find_metric(&self, name: &str) -> Option<&Metric> {
        self.metrics.iter().find(|m| m.name == name)
    }

    pub fn find_metric_mut(&mut self, name: &str) -> Option<&mut Metric> {
        self.metrics.iter_mut().find(|m| m.name == name)
    }
}

impl Default for SensorData {
    fn default() -> Self {
        Self::new()
    }
}

impl fmt::Display for SensorData {
    /// Mirrors the C `sensorDataToString` format:
    /// `Sensor 7 | temperature(C):20 humidity(%):50`
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Sensor {} |", self.id)?;
        for m in &self.metrics {
            write!(f, " {}({}):{}", m.name, m.unit, fmt_g(m.value))?;
        }
        Ok(())
    }
}

/// Formats a float the way C's `%g` does for the value ranges used here:
/// shortest representation without trailing zeros.
pub fn fmt_g(v: f32) -> String {
    if v == v.trunc() && v.abs() < 1e9 {
        format!("{}", v.trunc() as i64)
    } else {
        format!("{}", v)
    }
}

/// Each update moves a metric by up to +/- this fraction of its initial
/// value. Metrics with an initial value of 0 stay constant.
const RANDOM_WALK_FRACTION: f32 = 0.05;

fn random_walk(value: f32, max_delta: f32) -> f32 {
    let delta = (rng::rand01() * 2.0 - 1.0) * max_delta;
    let next = value + delta;
    if next > 0.0 {
        next
    } else {
        0.0
    }
}

impl SensorData {
    /// Placeholder simulation: nudges every metric around its initial value.
    /// Replace with real data sources later.
    pub fn update(&mut self, time_s: f32) {
        self.last_update_s = time_s;

        for m in &mut self.metrics {
            let initial_value = m.initial_value.abs();
            let max_delta = initial_value * RANDOM_WALK_FRACTION;
            if max_delta <= 0.0 {
                continue;
            }
            m.value = random_walk(m.value, max_delta);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_default_sensor() -> SensorData {
        let mut d = SensorData::new();
        d.add_metric("temperature", Some("C"), 20.0);
        d.add_metric("humidity", Some("%"), 50.0);
        d
    }

    #[test]
    fn sensor_init() {
        let s = Sensor::new(42);
        assert_eq!(s.id, 42);
    }

    #[test]
    fn sensor_data_init() {
        let d = SensorData::new();
        assert_eq!(d.id, 0);
        assert!(d.metrics.is_empty());
        assert_eq!(d.last_update_s, 0.0);
    }

    #[test]
    fn add_metric_stores_name_unit_and_value() {
        let mut d = SensorData::new();
        assert!(d.add_metric("temperature", Some("C"), 22.5));
        assert_eq!(d.metrics.len(), 1);

        let m = d.find_metric("temperature").unwrap();
        assert_eq!(m.name, "temperature");
        assert_eq!(m.unit, "C");
        assert_eq!(m.value, 22.5);
        assert_eq!(m.initial_value, 22.5);
    }

    #[test]
    fn add_metric_rejects_full_sensor() {
        let mut d = SensorData::new();
        for i in 0..MAX_METRICS {
            assert!(d.add_metric(&format!("m{i}"), Some(""), 1.0));
        }
        assert!(!d.add_metric("overflow", Some(""), 1.0));
        assert_eq!(d.metrics.len(), MAX_METRICS);
    }

    #[test]
    fn add_metric_rejects_empty_name() {
        let mut d = SensorData::new();
        assert!(!d.add_metric("", Some(""), 1.0));
        assert_eq!(d.metrics.len(), 0);
    }

    #[test]
    fn find_metric_missing_returns_none() {
        let mut d = SensorData::new();
        d.add_metric("a", Some(""), 1.0);
        assert!(d.find_metric("a").is_some());
        assert!(d.find_metric("b").is_none());
    }

    #[test]
    fn update_changes_values() {
        let mut d = make_default_sensor();

        rng::srand(42);
        d.update(0.0);
        let temp1 = d.find_metric("temperature").unwrap().value;

        d.update(1.0);
        assert_ne!(d.find_metric("temperature").unwrap().value, temp1);
    }

    #[test]
    fn update_is_deterministic_with_seed() {
        let mut d1 = make_default_sensor();
        let mut d2 = make_default_sensor();

        rng::srand(42);
        d1.update(5.0);

        rng::srand(42);
        d2.update(5.0);

        for i in 0..d1.metrics.len() {
            assert_eq!(d1.metrics[i].value, d2.metrics[i].value);
        }
    }

    #[test]
    fn update_stays_near_initial_value() {
        const FRACTION: f32 = 0.05;
        let mut d = make_default_sensor();

        rng::srand(7);
        d.update(0.0);

        let m = d.find_metric("temperature").unwrap();
        let max_delta = 20.0 * FRACTION + 0.001;
        assert!(m.value > 20.0 - max_delta && m.value < 20.0 + max_delta);
    }

    #[test]
    fn update_never_negative() {
        let mut d = make_default_sensor();

        rng::srand(1234);
        for i in 0..100 {
            d.update(i as f32);
            assert!(d.find_metric("temperature").unwrap().value >= 0.0);
            assert!(d.find_metric("humidity").unwrap().value >= 0.0);
        }
    }

    #[test]
    fn zero_initial_value_metric_stays_constant() {
        let mut d = SensorData::new();
        d.add_metric("offset", Some(""), 0.0);

        rng::srand(99);
        d.update(10.0);
        assert_eq!(d.find_metric("offset").unwrap().value, 0.0);
    }

    #[test]
    fn to_string_contains_sensor_id_and_metrics() {
        let mut d = make_default_sensor();
        d.id = 7;

        let s = d.to_string();
        assert!(s.contains("Sensor 7"));
        assert!(s.contains("temperature(C):20"));
        assert!(s.contains("humidity(%):50"));
    }

    #[test]
    fn truncate_bytes_never_splits_utf8() {
        assert_eq!(truncate_bytes("hello", 3), "hel");
        assert_eq!(truncate_bytes("héllo", 2), "h"); // é is 2 bytes
        assert_eq!(truncate_bytes("hi", 10), "hi");
    }
}
