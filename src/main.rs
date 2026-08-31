use aether::config::load_sensor_config;
use aether::dashboard::{Dashboard, FrameInput};
use aether::history::SensorHistory;
use aether::scheduler::{unix_now, Task};
use aether::sensor::SensorData;
use raylib::prelude::*;

/// One reading history per sensor, seeded with the config snapshot.
const HISTORY_CAPACITY: usize = 64;

/// Seconds between simulated sensor updates (the C mock task period).
const SENSOR_PERIOD_S: u32 = 3;

fn main() {
    const INITIAL_WIDTH: i32 = 800;
    const INITIAL_HEIGHT: i32 = 450;

    let (mut rl, thread) = init()
        .width(INITIAL_WIDTH)
        .height(INITIAL_HEIGHT)
        .title("Aether")
        .resizable()
        .build();

    // Window icon: prefer the working directory, fall back to the
    // application directory (mirrors main.c).
    let icon_path = if std::path::Path::new("logo.png").exists() {
        "logo.png".to_string()
    } else {
        format!("{}logo.png", rl.application_directory())
    };
    if let Ok(icon) = Image::load_image(&icon_path) {
        rl.set_window_icon(&icon);
    }

    rl.set_window_min_size(480, 320);
    rl.set_target_fps(60);

    // Load sensors from config; the Vec stays alive and acts as the
    // registry: the single, authoritative copy of each sensor's state.
    let mut sensors: Vec<SensorData> = match load_sensor_config("config/sensors.yaml") {
        Ok(s) => s,
        Err(e) => {
            eprintln!("AETHER: {e}");
            return;
        }
    };

    let mut dashboard = Dashboard::new(&thread, &mut rl, &sensors);

    // One reading history per sensor, seeded with the config snapshot
    let mut histories: Vec<SensorHistory> = sensors
        .iter()
        .map(|s| {
            let mut h = SensorHistory::new(HISTORY_CAPACITY);
            h.push(s.clone());
            h
        })
        .collect();

    // One task per sensor: the scheduler decides WHEN, the loop decides
    // WHAT to do (update the registry entry and push a reading).
    let mut tasks: Vec<Task> = (0..sensors.len())
        .map(|_| Task::new(SENSOR_PERIOD_S, None, unix_now()))
        .collect();

    while !rl.window_should_close() {
        let (screen_w, screen_h) = (rl.get_screen_width(), rl.get_screen_height());
        let input = FrameInput::gather(&rl);

        {
            let mut d = rl.begin_drawing(&thread);
            dashboard.draw(&mut d, &sensors, &histories, screen_w, screen_h, &input);
        } // draw handle dropped before the state updates below

        for (i, task) in tasks.iter_mut().enumerate() {
            if task.should_run(unix_now()) {
                sensors[i].update(rl.get_time() as f32);
                histories[i].push(sensors[i].clone());
                task.last_run = unix_now();
            }
        }
    }
}
