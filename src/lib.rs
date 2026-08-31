//! Aether — a real-time sensor dashboard.
//!
//! Ported from the original C/Raylib implementation: the data modules
//! (sensor simulation, history, scheduler, config, layout math) are pure
//! and unit-tested; the dashboard is rendered with raylib.

pub mod config;
pub mod dashboard;
pub mod history;
pub mod layout;
pub mod rng;
pub mod scheduler;
pub mod sensor;
pub mod theme;
