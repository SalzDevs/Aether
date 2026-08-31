//! Bounded per-sensor reading history (ring buffer).
//! Pushing into a full history drops the oldest reading.

use crate::sensor::SensorData;

pub struct SensorHistory {
    /// max readings kept
    capacity: usize,
    /// valid readings (<= capacity)
    size: usize,
    /// next write position
    head: usize,
    data: Vec<SensorData>,
}

impl SensorHistory {
    /// Allocates storage for `capacity` readings. Capacity is clamped to 1.
    pub fn new(capacity: usize) -> Self {
        let capacity = capacity.max(1);
        SensorHistory {
            capacity,
            size: 0,
            head: 0,
            data: vec![SensorData::new(); capacity],
        }
    }

    /// Mirrors the C `destroySensorHistory`: resets all state.
    /// Calling it twice is safe.
    pub fn destroy(&mut self) {
        self.capacity = 0;
        self.size = 0;
        self.head = 0;
        self.data = Vec::new();
    }

    /// Number of valid readings currently stored (<= capacity).
    pub fn size(&self) -> usize {
        self.size
    }

    /// Reading `index` counted from the OLDEST (0) to the newest (size-1).
    /// Returns `None` when the index is out of range.
    pub fn at(&self, index: usize) -> Option<&SensorData> {
        if self.size == 0 || index >= self.size {
            return None;
        }
        // oldest lives at (head - size) mod capacity
        let oldest = (self.head + self.capacity - self.size) % self.capacity;
        let slot = (oldest + index) % self.capacity;
        self.data.get(slot)
    }

    /// Computes min/max/avg of one metric across all stored readings.
    /// Returns `None` when there are no readings or the metric is missing
    /// from any stored reading.
    pub fn metric_stats(&self, metric_idx: usize) -> Option<(f32, f32, f32)> {
        let count = self.size();
        if count == 0 {
            return None;
        }

        let mut min = 0.0f32;
        let mut max = 0.0f32;
        let mut sum = 0.0f32;
        for (i, reading) in (0..count).filter_map(|i| self.at(i)).enumerate() {
            if metric_idx >= reading.metrics.len() {
                return None;
            }
            let v = reading.metrics[metric_idx].value;
            if i == 0 || v < min {
                min = v;
            }
            if i == 0 || v > max {
                max = v;
            }
            sum += v;
        }
        Some((min, max, sum / count as f32))
    }

    /// Adds a reading; when the history is full, the oldest is dropped.
    pub fn push(&mut self, d: SensorData) {
        if self.capacity == 0 {
            return; // destroyed history: nothing to do (C would be UB here)
        }
        self.data[self.head] = d;
        self.head = (self.head + 1) % self.capacity;
        if self.size < self.capacity {
            self.size += 1;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::sensor::SensorData;

    fn make_sensor_data(level: f32) -> SensorData {
        let mut d = SensorData::new();
        d.add_metric("level", Some("%"), level);
        d
    }

    fn level_of(d: &SensorData) -> f32 {
        d.find_metric("level").unwrap().value
    }

    #[test]
    fn init_sets_capacity_and_empty() {
        let h = SensorHistory::new(3);
        assert_eq!(h.size(), 0);
        assert!(h.at(0).is_none());
    }

    #[test]
    fn push_increases_size() {
        let mut h = SensorHistory::new(3);
        h.push(make_sensor_data(1.0));
        assert_eq!(h.size(), 1);
        h.push(make_sensor_data(2.0));
        assert_eq!(h.size(), 2);
    }

    #[test]
    fn readings_ordered_oldest_to_newest() {
        let mut h = SensorHistory::new(8);
        for i in 1..=5 {
            h.push(make_sensor_data(i as f32));
        }

        assert_eq!(h.size(), 5);
        for i in 0..5 {
            assert_eq!(level_of(h.at(i).unwrap()), (i + 1) as f32);
        }
    }

    #[test]
    fn full_history_drops_oldest() {
        let mut h = SensorHistory::new(3);
        for i in 1..=6 {
            h.push(make_sensor_data(i as f32));
        }

        assert_eq!(h.size(), 3);
        assert_eq!(level_of(h.at(0).unwrap()), 4.0);
        assert_eq!(level_of(h.at(1).unwrap()), 5.0);
        assert_eq!(level_of(h.at(2).unwrap()), 6.0);
    }

    #[test]
    fn ring_wraps_many_times() {
        let mut h = SensorHistory::new(4);
        for i in 1..=11 {
            h.push(make_sensor_data(i as f32));
        }

        assert_eq!(h.size(), 4);
        assert_eq!(level_of(h.at(0).unwrap()), 8.0);
        assert_eq!(level_of(h.at(1).unwrap()), 9.0);
        assert_eq!(level_of(h.at(2).unwrap()), 10.0);
        assert_eq!(level_of(h.at(3).unwrap()), 11.0);
    }

    #[test]
    fn at_out_of_range_returns_none() {
        let mut h = SensorHistory::new(2);
        h.push(make_sensor_data(1.0));
        assert!(h.at(1).is_none());
        assert!(h.at(99).is_none());
    }

    #[test]
    fn metric_stats_min_max_avg() {
        let mut h = SensorHistory::new(8);
        for v in [4.0f32, 8.0, 6.0] {
            h.push(make_sensor_data(v));
        }

        let (min, max, avg) = h.metric_stats(0).unwrap();
        assert_eq!(min, 4.0);
        assert_eq!(max, 8.0);
        assert_eq!(avg, 6.0);
    }

    #[test]
    fn metric_stats_empty_history_fails() {
        let h = SensorHistory::new(4);
        assert!(h.metric_stats(0).is_none());
    }

    #[test]
    fn metric_stats_missing_metric_fails() {
        let mut h = SensorHistory::new(4);
        h.push(SensorData::new()); // no metrics at all
        assert!(h.metric_stats(0).is_none());
    }

    #[test]
    fn destroy_resets_state() {
        let mut h = SensorHistory::new(2);
        h.push(make_sensor_data(1.0));
        h.destroy();

        assert_eq!(h.size(), 0);
        assert!(h.at(0).is_none());

        h.destroy(); // double destroy is safe
    }

    #[test]
    fn zero_capacity_is_clamped_to_one() {
        let mut h = SensorHistory::new(0);
        h.push(make_sensor_data(1.0));
        h.push(make_sensor_data(2.0));
        assert_eq!(h.size(), 1);
        assert_eq!(level_of(h.at(0).unwrap()), 2.0);
    }
}
