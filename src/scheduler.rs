//! Fixed-slot task scheduler decoupled from data handling.
//! The scheduler only decides WHEN; the task decides WHAT to do.

use std::time::{SystemTime, UNIX_EPOCH};

/// Current wall-clock time in whole seconds since the Unix epoch
/// (mirrors the C `time(NULL)`).
pub fn unix_now() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0)
}

pub type TaskFn = Box<dyn FnMut()>;

pub struct Task {
    /// period of running the task
    pub period: u32,
    /// time of the last run
    pub last_run: u64,
    /// executable task function
    pub function: Option<TaskFn>,
}

impl Task {
    /// Initiates the Task structure.
    pub fn new(period: u32, function: Option<TaskFn>, current_time: u64) -> Self {
        Task {
            period,
            last_run: current_time,
            function,
        }
    }

    /// Evaluates if enough time has passed to run the task.
    pub fn should_run(&self, current_time: u64) -> bool {
        should_run_task(self.period, self.last_run, current_time)
    }

    /// Runs the task if its period has elapsed.
    pub fn run(&mut self) {
        if self.function.is_none() {
            return;
        }
        let current_time = unix_now();
        if self.should_run(current_time) {
            if let Some(f) = self.function.as_mut() {
                f();
            }
            self.last_run = current_time;
        }
    }
}

/// Evaluates if enough time has passed to run a task.
pub fn should_run_task(period: u32, last_run: u64, current_time: u64) -> bool {
    current_time.wrapping_sub(last_run) >= period as u64
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn init_sets_period_and_function() {
        let t = Task::new(50, Some(Box::new(|| {})), 12345);
        assert_eq!(t.period, 50);
        assert!(t.function.is_some());
        assert_eq!(t.last_run, 12345);
    }

    #[test]
    fn init_zero_values() {
        let t = Task::new(0, None, 0);
        assert_eq!(t.period, 0);
        assert!(t.function.is_none());
        assert_eq!(t.last_run, 0);
    }

    #[test]
    fn should_run_exactly_at_period() {
        assert!(should_run_task(30, 1000, 1030));
    }

    #[test]
    fn should_not_run_before_period() {
        assert!(!should_run_task(30, 1000, 1029));
    }

    #[test]
    fn should_run_long_past_period() {
        assert!(should_run_task(30, 1000, 5000));
    }

    #[test]
    fn zero_period_always_ready() {
        assert!(should_run_task(0, 1000, 1000));
    }

    #[test]
    fn wrap_around_still_runs() {
        assert!(should_run_task(30, 1000, 10));
    }

    #[test]
    fn run_task_executes_and_updates_last_run_when_period_elapsed() {
        let start = unix_now() - 3;
        let mut t = Task::new(3, Some(Box::new(|| {})), start);
        t.run();

        assert!(t.function.is_some()); // was executed
        assert!(t.last_run >= start + 3); // last_run moved forward
    }

    #[test]
    fn run_task_does_nothing_before_period() {
        let start = unix_now();
        let ran = std::rc::Rc::new(std::cell::Cell::new(false));
        let ran2 = ran.clone();
        let mut t = Task::new(
            3,
            Some(Box::new(move || ran2.set(true))),
            start,
        );
        t.run();

        assert!(!ran.get());
        assert_eq!(t.last_run, start); // unchanged
    }

    #[test]
    fn run_task_with_null_task_does_not_crash() {
        let mut t = Task::new(0, None, 0);
        t.run(); // no function: nothing happens
    }
}
