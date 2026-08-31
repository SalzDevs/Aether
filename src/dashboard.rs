//! The raylib dashboard: sensor cards, sparklines, tab pager, detail and
//! settings modals. A direct port of `ui/dashboard.c`.

use crate::config::{save_settings, Settings, AETHER_SETTINGS_FILE};
use crate::history::SensorHistory;
use crate::layout::{layout_map_range, layout_page_list, layout_round2};
use crate::sensor::{fmt_g, Metric, SensorData, MAX_METRICS};
use crate::theme::{apply_theme_preset, DashboardTheme, THEME_PRESETS};
use raylib::prelude::*;

const FONT_PATH: &str = "assets/fonts/JetBrainsMono-Regular.ttf";
const LOGO_PATH: &str = "logo.png";

const SPARK_MAX_POINTS: usize = 64;

/// Input state polled once per frame, before drawing starts. Mirrors the
/// `IsKeyPressed`/`GetMousePosition` calls the C code makes mid-draw.
pub struct FrameInput {
    pub mouse: Vector2,
    pub mouse_pressed: bool,
    pub dt: f32,
    pub time_s: f64,
    pub key_s: bool,
    pub key_escape: bool,
    pub key_left: bool,
    pub key_right: bool,
    pub modifier_down: bool,
    pub digits: [bool; 9],
}

impl FrameInput {
    pub fn gather(rl: &RaylibHandle) -> FrameInput {
        const DIGIT_KEYS: [KeyboardKey; 9] = [
            KeyboardKey::KEY_ONE,
            KeyboardKey::KEY_TWO,
            KeyboardKey::KEY_THREE,
            KeyboardKey::KEY_FOUR,
            KeyboardKey::KEY_FIVE,
            KeyboardKey::KEY_SIX,
            KeyboardKey::KEY_SEVEN,
            KeyboardKey::KEY_EIGHT,
            KeyboardKey::KEY_NINE,
        ];
        FrameInput {
            mouse: rl.get_mouse_position(),
            mouse_pressed: rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT),
            dt: rl.get_frame_time(),
            time_s: rl.get_time(),
            key_s: rl.is_key_pressed(KeyboardKey::KEY_S),
            key_escape: rl.is_key_pressed(KeyboardKey::KEY_ESCAPE),
            key_left: rl.is_key_pressed(KeyboardKey::KEY_LEFT),
            key_right: rl.is_key_pressed(KeyboardKey::KEY_RIGHT),
            modifier_down: rl.is_key_down(KeyboardKey::KEY_LEFT_ALT)
                || rl.is_key_down(KeyboardKey::KEY_RIGHT_ALT)
                || rl.is_key_down(KeyboardKey::KEY_LEFT_SUPER)
                || rl.is_key_down(KeyboardKey::KEY_RIGHT_SUPER),
            digits: DIGIT_KEYS.map(|k| rl.is_key_pressed(k)),
        }
    }
}

pub struct Dashboard {
    font: WeakFont,
    logo: Option<Texture2D>,

    // --- Animation state (one slot per metric, indexed sensor*MAX_METRICS+metric) ---
    display: Vec<f32>,  // smoothed value currently shown
    prev: Vec<f32>,     // last actual value seen (change detection)
    dir: Vec<i8>,       // direction of last change: -1, 0, +1
    dir_timer: Vec<f32> // seconds left before the arrow fades
    ,
    sensor_count: usize,

    // --- Persisted settings ---
    pub settings: Settings,

    // --- Pure UI state ---
    theme: DashboardTheme,
    settings_open: bool,
    detail_sensor: i32,
    current_page: i32,
}

impl Dashboard {
    pub fn new(thread: &RaylibThread, rl: &mut RaylibHandle, sensors: &[SensorData]) -> Dashboard {
        let mut dash = Dashboard {
            font: rl.get_font_default(),
            logo: None,
            display: vec![0.0; sensors.len() * MAX_METRICS],
            prev: vec![0.0; sensors.len() * MAX_METRICS],
            dir: vec![0; sensors.len() * MAX_METRICS],
            dir_timer: vec![0.0; sensors.len() * MAX_METRICS],
            sensor_count: sensors.len(),
            settings: Settings::default(),
            theme: THEME_PRESETS[0],
            settings_open: false,
            detail_sensor: -1,
            current_page: 0,
        };

        // Load settings from disk; fall back to defaults if missing/invalid.
        if let Ok(s) = crate::config::load_settings(AETHER_SETTINGS_FILE) {
            dash.settings = s;
        }
        dash.apply_theme(dash.settings.theme);

        // Start display values at the actual readings (no animation on launch)
        for (s, sensor) in sensors.iter().enumerate() {
            for (m, metric) in sensor.metrics.iter().enumerate() {
                let idx = s * MAX_METRICS + m;
                dash.display[idx] = metric.value;
                dash.prev[idx] = metric.value;
            }
        }

        if std::path::Path::new(FONT_PATH).exists() {
            // ASCII range plus the symbols units and labels use
            let mut codepoints: String = (32u8..127).map(|b| b as char).collect();
            codepoints.push('\u{B7}'); // middle dot (stats separator)
            codepoints.push('\u{B0}'); // degree sign
            codepoints.push('\u{B5}'); // micro sign
            if let Ok(font) = rl.load_font_ex(
                thread,
                FONT_PATH,
                dash.theme.font_atlas_size as i32,
                Some(&codepoints),
            ) {
                // filter the font atlas without taking ownership of it
                let tex = unsafe { WeakTexture2D::from_raw(font.texture) };
                tex.set_texture_filter(thread, TextureFilter::TEXTURE_FILTER_BILINEAR);
                dash.font = font.make_weak();
            }
        }

        // In-app header logo (22px fits topBarHeight=34)
        let logo_path = if std::path::Path::new(LOGO_PATH).exists() {
            LOGO_PATH.to_string()
        } else {
            format!("{}logo.png", rl.application_directory())
        };
        if let Ok(mut img) = Image::load_image(&logo_path) {
            img.resize(22, 22);
            if let Ok(tex) = rl.load_texture_from_image(thread, &img) {
                dash.logo = Some(tex);
            }
        }

        dash
    }

    fn apply_theme(&mut self, index: i32) {
        apply_theme_preset(&mut self.theme, &mut self.settings.theme, index);
    }

    /// Advances value smoothing and change-indicator timers by dt seconds.
    fn update_animations(&mut self, sensors: &[SensorData], dt: f32) {
        let mut alpha = dt * self.theme.value_smoothing;
        if alpha > 1.0 {
            alpha = 1.0;
        }

        for (s, sensor) in sensors.iter().enumerate().take(self.sensor_count) {
            for m in 0..sensor.metrics.len() {
                let idx = s * MAX_METRICS + m;
                let target = sensor.metrics[m].value;

                // With animations off, displayed values snap to the readings
                self.display[idx] = if self.settings.animate_values {
                    self.display[idx] + (target - self.display[idx]) * alpha
                } else {
                    target
                };

                if target != self.prev[idx] {
                    self.dir[idx] = if target > self.prev[idx] { 1 } else { -1 };
                    self.dir_timer[idx] = if self.settings.show_indicators {
                        self.theme.arrow_hold_seconds
                    } else {
                        0.0
                    };
                    self.prev[idx] = target;
                }
                if self.dir_timer[idx] > 0.0 {
                    self.dir_timer[idx] -= dt;
                }
            }
        }
    }

    // -- Text helpers: all rendering goes through the theme font with
    // raylib's usual size/10 glyph spacing. --
    fn measure(&self, text: &str, size: f32) -> Vector2 {
        self.font.measure_text(text, size, size / 10.0)
    }

    fn draw_text(
        &self,
        d: &mut RaylibDrawHandle,
        text: &str,
        x: i32,
        y: i32,
        size: f32,
        color: Color,
    ) {
        d.draw_text_ex(
            &self.font,
            text,
            Vector2::new(x as f32, y as f32),
            size,
            size / 10.0,
            color,
        );
    }

    /// Shrinks fontSize until text fits maxWidth (or hits the floor).
    fn fit_font_size(&self, text: &str, max_width: f32, font_size: &mut f32, floor_size: f32) {
        let mut bounds = self.measure(text, *font_size).x;
        while bounds > max_width && *font_size > floor_size {
            *font_size -= 1.0;
            bounds = self.measure(text, *font_size).x;
        }
    }

    /// Draws a single line of text centered inside the given rect.
    #[allow(clippy::too_many_arguments)]
    fn draw_centered_in_row(
        &self,
        d: &mut RaylibDrawHandle,
        text: &str,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        font_size: f32,
        color: Color,
    ) {
        let bounds = self.measure(text, font_size).x;
        self.draw_text(
            d,
            text,
            x + (w - bounds as i32) / 2,
            y + (h - font_size as i32) / 2,
            font_size,
            color,
        );
    }

    /// Draws the metric's reading history as a small trend line between
    /// the label and the value. Returns the width consumed (0 if skipped).
    #[allow(clippy::too_many_arguments)]
    fn draw_sparkline(
        &self,
        d: &mut RaylibDrawHandle,
        hist: Option<&SensorHistory>,
        metric_idx: usize,
        x: i32,
        y: i32,
        max_w: i32,
        h: i32,
    ) -> i32 {
        let Some(hist) = hist else { return 0 };
        if max_w < 24 {
            return 0;
        }

        let mut count = hist.size();
        if count < 2 {
            return 0; // a trend needs at least two readings
        }
        if count > SPARK_MAX_POINTS {
            count = SPARK_MAX_POINTS;
        }

        // Gather the series and its range
        let mut values = Vec::with_capacity(count);
        for i in 0..count {
            let Some(reading) = hist.at(i) else { return 0 };
            if metric_idx >= reading.metrics.len() {
                return 0;
            }
            values.push(reading.metrics[metric_idx].value);
        }
        let (min, max) = crate::layout::layout_min_max(&values);

        // Vertical layout: small margins inside the row
        let top = y as f32 + 4.0;
        let bottom = (y + h) as f32 - 4.0;
        if bottom - top < 4.0 {
            return 0;
        }

        let step = max_w as f32 / (count - 1) as f32;
        let points: Vec<Vector2> = values
            .iter()
            .enumerate()
            .map(|(i, &v)| {
                Vector2::new(
                    x as f32 + step * i as f32,
                    layout_map_range(v, min, max, bottom, top),
                )
            })
            .collect();

        let line_color = Color::new(
            self.theme.value_text.r,
            self.theme.value_text.g,
            self.theme.value_text.b,
            110,
        );
        d.draw_line_strip(&points, line_color);

        // Bright dot on the newest reading
        let last = points[count - 1];
        d.draw_circle_v(last, 2.0, self.theme.value_text);

        max_w
    }

    /// Draws one label/value line: label on the left, value + unit (+ change
    /// arrow) on the right. Sizes are computed by the caller.
    #[allow(clippy::too_many_arguments)]
    fn draw_label_value_line(
        &self,
        d: &mut RaylibDrawHandle,
        label: &str,
        value_buf: &str,
        unit: &str,
        x: i32,
        y: i32,
        w: i32,
        line_h: i32,
        pad: i32,
        value_size: f32,
        label_size: f32,
        unit_size: f32,
        dir: i8,
        dir_alpha: f32,
    ) {
        let group_width = self.measure(value_buf, value_size).x + 4.0 + self.measure(unit, unit_size).x;
        let group_x = x + w - pad - group_width as i32;

        let label_y = y + (line_h - label_size as i32) / 2;
        let value_y = y + (line_h - value_size as i32) / 2;
        self.draw_text(d, label, x + pad, label_y, label_size, self.theme.label_text);

        let arrow_size = value_size / 3.0;
        if dir != 0 && dir_alpha > 0.0 && self.settings.show_indicators {
            let base = if dir > 0 {
                self.theme.up_color
            } else {
                self.theme.down_color
            };
            let arrow_color = Color::new(base.r, base.g, base.b, (255.0 * dir_alpha) as u8);
            let ax = (group_x - arrow_size as i32 - 6) as f32;
            let ay = (value_y + value_size as i32 / 2) as f32;
            let half = arrow_size / 2.0;
            if dir > 0 {
                d.draw_triangle(
                    Vector2::new(ax, ay + half),
                    Vector2::new(ax + arrow_size, ay + half),
                    Vector2::new(ax + half, ay - half),
                    arrow_color,
                );
            } else {
                d.draw_triangle(
                    Vector2::new(ax + arrow_size, ay - half),
                    Vector2::new(ax, ay - half),
                    Vector2::new(ax + half, ay + half),
                    arrow_color,
                );
            }
        }

        self.draw_text(d, value_buf, group_x, value_y, value_size, self.theme.value_text);
        let unit_x =
            group_x + self.measure(value_buf, value_size).x as i32 + 4;
        let unit_y = value_y + (value_size as i32 - unit_size as i32) / 3;
        self.draw_text(d, unit, unit_x, unit_y, unit_size, self.theme.label_text);
    }

    #[allow(clippy::too_many_arguments)]
    fn draw_metric_row(
        &self,
        d: &mut RaylibDrawHandle,
        m: &Metric,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        index: i32,
        display_value: f32,
        dir: i8,
        dir_alpha: f32,
        hist: Option<&SensorHistory>,
        metric_idx: usize,
        scale: f32,
    ) {
        // Each metric is a two-line block: text line on top, sparkline
        // underneath. The sparkline is earned from leftover block height:
        // if the card is too dense, the row degrades to text only.
        let row_y = y + index * h;

        let value_buf = fmt_g(layout_round2(display_value));
        let unit = &m.unit;

        // Text sizes: grow with the card (scale), capped for readability
        let mut value_size = ((20.0 * scale) as i32).clamp(11, (h * 2) / 3) as f32;
        let mut label_size = (value_size / 2.0).max(9.0);
        let mut unit_size = (value_size / 2.0 + 1.0).max(8.0);

        // Label and value group get separate width budgets so they never collide
        let content_width = w - 32; // chip insets (2*6) + chip padding (2*10)
        let label_max_width = (content_width * 55 / 100) as f32;
        let group_max_width = (content_width * 42 / 100) as f32;

        self.fit_font_size(&m.name, label_max_width, &mut label_size, 8.0);

        let compute_group_width = |vs: f32, us: f32| {
            self.measure(&value_buf, vs).x + 4.0 + self.measure(unit, us).x
        };
        let mut group_width = compute_group_width(value_size, unit_size);
        while group_width > group_max_width && value_size > 8.0 {
            value_size -= 1.0;
            unit_size = (value_size / 2.0 + 1.0).max(8.0);
            group_width = compute_group_width(value_size, unit_size);
        }

        // Metric chip: an inset sub-card that groups this metric's data
        let chip_x = x + 6;
        let chip_y = row_y + 3;
        let chip_w = w - 12;
        let chip_h = h - 6;
        d.draw_rectangle_rounded(
            Rectangle::new(chip_x as f32, chip_y as f32, chip_w as f32, chip_h as f32),
            0.15,
            6,
            self.theme.chip,
        );
        let cx = chip_x + 10;
        let cw = chip_w - 20; // content area inside the chip

        // --- Vertical layout: text line first, sparkline gets the leftover ---
        let text_line_h = value_size as i32 + 6;
        let spark_wanted = self.settings.show_sparklines && hist.is_some();
        let spark_h = if spark_wanted {
            (h - text_line_h - 6).min((26.0 * scale) as i32)
        } else {
            0
        };
        let show_spark = spark_h >= 10;

        let group_h = text_line_h + if show_spark { spark_h + 6 } else { 0 };
        let group_y = row_y + (h - group_h) / 2;

        self.draw_label_value_line(
            d,
            &m.name,
            &value_buf,
            unit,
            cx,
            group_y,
            cw,
            text_line_h,
            10,
            value_size,
            label_size,
            unit_size,
            dir,
            dir_alpha,
        );

        // Sparkline: full chip width, directly under its metric
        if show_spark {
            self.draw_sparkline(d, hist, metric_idx, cx, group_y + text_line_h + 6, cw, spark_h);
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn draw_sensor_card(
        &self,
        d: &mut RaylibDrawHandle,
        sensor: &SensorData,
        sensor_idx: usize,
        hist: Option<&SensorHistory>,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        scale: f32,
    ) {
        d.draw_rectangle_rounded(
            Rectangle::new(x as f32, y as f32, w as f32, h as f32),
            0.12,
            8,
            self.theme.panel,
        );

        // --- Header: sensor name, id on the right ---
        let mut header_size = ((16.0 * scale) as i32).min(h / 5).max(10) as f32;
        let header_buf = if sensor.name.is_empty() { "sensor" } else { &sensor.name };
        let id_buf = format!("#{}", sensor.id);

        let header_max_width = (w - 2 * self.theme.padding) as f32
            - self.measure(&id_buf, header_size).x
            - 8.0;
        self.fit_font_size(header_buf, header_max_width, &mut header_size, 9.0);

        let header_y = y + self.theme.padding / 2 + 4;
        self.draw_text(d, header_buf, x + self.theme.padding, header_y, header_size, self.theme.title_text);

        let id_size = (header_size * 3.0 / 4.0).max(9.0);
        let id_x = x + w - self.theme.padding - self.measure(&id_buf, id_size).x as i32;
        self.draw_text(
            d,
            &id_buf,
            id_x,
            header_y + (header_size - id_size) as i32,
            id_size,
            self.theme.label_text,
        );

        let header_bottom = header_y + header_size as i32 + 6;
        d.draw_line(
            x + self.theme.padding,
            header_bottom,
            x + w - self.theme.padding,
            header_bottom,
            self.theme.divider,
        );

        // --- Metrics ---
        let metrics_top = header_bottom + 6;
        let metrics_height = (y + h) - metrics_top - self.theme.padding / 2;
        if sensor.metrics.is_empty() || metrics_height <= 0 {
            return;
        }

        let row_height = metrics_height / sensor.metrics.len() as i32;
        if row_height <= 0 {
            return;
        }

        for (i, metric) in sensor.metrics.iter().enumerate() {
            let idx = sensor_idx * MAX_METRICS + i;
            let dir_alpha = if self.dir_timer[idx] > 0.0 && self.theme.arrow_hold_seconds > 0.0 {
                self.dir_timer[idx] / self.theme.arrow_hold_seconds
            } else {
                0.0
            };
            self.draw_metric_row(
                d,
                metric,
                x,
                metrics_top,
                w,
                row_height,
                i as i32,
                self.display[idx],
                self.dir[idx],
                dir_alpha,
                hist,
                i,
                scale,
            );
        }
    }

    /// Draws a gear silhouette (triangle fan) with a punched center hole.
    fn draw_cogwheel(&self, d: &mut RaylibDrawHandle, center: Vector2, radius: f32, color: Color) {
        const TEETH: usize = 8;
        const ROOT_RATIO: f32 = 0.72; // valley radius as fraction of outer
        const DUTY: f32 = 0.55; // fraction of each cycle spent at outer radius

        const MAX_OUTLINE: usize = 256;
        let mut points: Vec<Vector2> = Vec::with_capacity(MAX_OUTLINE + 2);
        points.push(center); // triangle fan center

        let step_degrees = 360.0 / TEETH as f32;
        let mut a = 0.0f32;
        while a < 360.0 {
            if points.len() >= MAX_OUTLINE {
                break;
            }
            let phase = (a % step_degrees) / step_degrees;
            let r = if phase < DUTY { radius } else { radius * ROOT_RATIO };
            let rad = a * (DEG2RAD as f32);
            // y negated: screen y grows downward, raylib wants counter-clockwise
            points.push(Vector2::new(
                center.x + rad.cos() * r,
                center.y - rad.sin() * r,
            ));
            a += 1.5;
        }
        if let Some(&first) = points.get(1) {
            points.push(first); // close the outline
        }

        d.draw_triangle_fan(&points, color);
        d.draw_circle_v(center, radius * 0.35, self.theme.background); // center hole
    }

    /// Draws an on/off pill switch; returns true when clicked.
    fn draw_toggle_pill(&self, d: &mut RaylibDrawHandle, value: bool, x: i32, y: i32, input: &FrameInput) -> bool {
        let (pill_w, pill_h) = (34.0f32, 16.0f32);
        let pill = Rectangle::new(x as f32, y as f32, pill_w, pill_h);

        let track = if value { self.theme.value_text } else { self.theme.divider };
        d.draw_rectangle_rounded(pill, 0.6, 6, track);

        let knob_r = pill_h / 2.0 - 2.0;
        let knob_x = if value {
            x as f32 + pill_w - knob_r - 2.0
        } else {
            x as f32 + knob_r + 2.0
        };
        d.draw_circle_v(Vector2::new(knob_x, y as f32 + pill_h / 2.0), knob_r, self.theme.title_text);

        input.mouse_pressed && pill.check_collision_point_rec(input.mouse)
    }

    /// Draws one settings row (label + pill); returns true when the pill
    /// was clicked (the caller flips the value).
    fn draw_settings_row(
        &self,
        d: &mut RaylibDrawHandle,
        label: &str,
        value: bool,
        row: Rectangle,
        input: &FrameInput,
    ) -> bool {
        let hover = row.check_collision_point_rec(input.mouse);
        if hover {
            d.draw_rectangle_rec(row, self.theme.panel_hover_color);
        }

        let label_size = 13.0f32;
        let label_y = row.y as i32 + (row.height as i32 - label_size as i32) / 2;
        self.draw_text(d, label, row.x as i32 + self.theme.padding, label_y, label_size, self.theme.title_text);

        let pill_h = 16.0f32;
        let pill_y = row.y + (row.height - pill_h) / 2.0;
        self.draw_toggle_pill(
            d,
            value,
            (row.x + row.width - self.theme.padding as f32 - 34.0) as i32,
            pill_y as i32,
            input,
        )
    }

    fn draw_theme_swatcher(&mut self, d: &mut RaylibDrawHandle, area: Rectangle, input: &FrameInput) {
        let n = THEME_PRESETS.len();
        let size = 30.0f32;
        let gap = 14.0f32;

        let total_w = n as f32 * size + (n - 1) as f32 * gap;
        let x0 = area.x + (area.width - total_w) / 2.0;
        let y = area.y + (area.height - size) / 2.0;

        let pressed = input.mouse_pressed;
        let mut picked: Option<i32> = None;

        for (i, preset) in THEME_PRESETS.iter().enumerate() {
            let sw = Rectangle::new(x0 + (size + gap) * i as f32, y, size, size);
            d.draw_rectangle_rounded(sw, 0.25, 6, preset.value_text);

            if i as i32 == self.settings.theme {
                let ring = Rectangle::new(sw.x - 3.0, sw.y - 3.0, size + 6.0, size + 6.0);
                d.draw_rectangle_rounded_lines(ring, 0.25, 6, self.theme.title_text);
            }

            if pressed && sw.check_collision_point_rec(input.mouse) {
                picked = Some(i as i32);
            }
        }

        if let Some(i) = picked {
            self.apply_theme(i);
            let _ = save_settings(AETHER_SETTINGS_FILE, &self.settings);
        }
    }

    fn draw_settings_panel(&mut self, d: &mut RaylibDrawHandle, screen_w: i32, screen_h: i32, input: &FrameInput) {
        let panel_w = 320.0f32;
        let (row_h, header_h) = (36.0f32, 44.0f32);
        let panel_h = header_h + 3.0 * row_h + 26.0 + 40.0 + 12.0;

        // Dim overlay: pushes the dashboard back, modal-style
        d.draw_rectangle(0, 0, screen_w, screen_h, Color::new(0, 0, 0, 150));

        let panel = Rectangle::new(
            (screen_w as f32 - panel_w) / 2.0,
            (screen_h as f32 - panel_h) / 2.0,
            panel_w,
            panel_h,
        );
        d.draw_rectangle_rounded(panel, 0.06, 8, self.theme.panel);

        self.draw_text(d, "SETTINGS", panel.x as i32 + self.theme.padding, panel.y as i32 + 14, 15.0, self.theme.title_text);
        d.draw_line(
            panel.x as i32 + self.theme.padding,
            (panel.y + header_h) as i32 - 6,
            (panel.x + panel_w - self.theme.padding as f32) as i32,
            (panel.y + header_h) as i32 - 6,
            self.theme.divider,
        );

        let mut row = Rectangle::new(panel.x + 4.0, panel.y + header_h, panel_w - 8.0, row_h);
        let mut changed = false;
        if self.draw_settings_row(d, "Trend lines", self.settings.show_sparklines, row, input) {
            self.settings.show_sparklines = !self.settings.show_sparklines;
            changed = true;
        }
        row.y += row_h;
        if self.draw_settings_row(d, "Animated values", self.settings.animate_values, row, input) {
            self.settings.animate_values = !self.settings.animate_values;
            changed = true;
        }
        row.y += row_h;
        if self.draw_settings_row(d, "Change indicators", self.settings.show_indicators, row, input) {
            self.settings.show_indicators = !self.settings.show_indicators;
            changed = true;
        }
        row.y += row_h;
        if changed {
            let _ = save_settings(AETHER_SETTINGS_FILE, &self.settings);
        }
        d.draw_line(
            panel.x as i32 + self.theme.padding,
            row.y as i32,
            (panel.x + panel_w - self.theme.padding as f32) as i32,
            row.y as i32,
            self.theme.divider,
        );

        self.draw_text(d, "Color theme", row.x as i32 + self.theme.padding, row.y as i32 + 6, 13.0, self.theme.title_text);
        self.draw_theme_swatcher(
            d,
            Rectangle::new(panel.x, row.y + 26.0, panel_w, 40.0),
            input,
        );
    }

    /// Draws the bottom tab pager; returns true when a tab was clicked.
    fn draw_tab_pager(&mut self, d: &mut RaylibDrawHandle, page_count: i32, screen_w: i32, screen_h: i32, input: &FrameInput) -> bool {
        let (circle_d, gap, dots_w) = (24.0f32, 10.0f32, 20.0f32);

        let seq = layout_page_list(page_count, self.current_page + 1, 9);

        // total width to center the strip
        let mut total = 0.0f32;
        for &s in &seq {
            total += if s == -1 { dots_w } else { circle_d };
        }
        total += gap * (seq.len() as f32 - 1.0);

        let mut x = (screen_w as f32 - total) / 2.0;
        let cy = screen_h as f32 - self.theme.bottom_bar_height as f32 / 2.0;
        let mut clicked = false;

        for &page in &seq {
            if page == -1 {
                self.draw_text(d, "...", (x + dots_w / 2.0 - 6.0) as i32, cy as i32 - 7, 13.0, self.theme.label_text);
                x += dots_w + gap;
                continue;
            }

            // page is 1-based
            let active = page == self.current_page + 1;
            let center = Vector2::new(x + circle_d / 2.0, cy);
            let hover = !active && check_collision_point_circle(input.mouse, center, circle_d / 2.0 + 3.0);

            let fill = if active {
                self.theme.value_text
            } else if hover {
                self.theme.panel_hover_color
            } else {
                self.theme.panel
            };
            let number = if active {
                self.theme.background
            } else if hover {
                self.theme.title_text
            } else {
                self.theme.label_text
            };
            d.draw_circle_v(center, circle_d / 2.0, fill);

            let num = format!("{}", page);
            let bounds = self.measure(&num, 12.0);
            self.draw_text(
                d,
                &num,
                (center.x - bounds.x / 2.0) as i32,
                (center.y - bounds.y / 2.0) as i32,
                12.0,
                number,
            );

            if !active && hover && input.mouse_pressed {
                self.current_page = page - 1;
                clicked = true;
            }

            x += circle_d + gap;
        }
        clicked
    }

    /// Draws one metric block inside the detail panel: label/value line,
    /// a large sparkline, and a min/avg/max stats line.
    #[allow(clippy::too_many_arguments)]
    fn draw_detail_metric_block(
        &self,
        d: &mut RaylibDrawHandle,
        m: &Metric,
        hist: Option<&SensorHistory>,
        metric_idx: usize,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        display_value: f32,
        dir: i8,
        dir_alpha: f32,
    ) {
        let value_buf = fmt_g(layout_round2(display_value));

        let (value_size, label_size, unit_size) = (24.0f32, 14.0f32, 15.0f32);
        let text_line_h = value_size as i32 + 8;
        let stats_h = 14.0f32;

        // Metric chip: same inset sub-card grouping as in the cards
        let (chip_x, chip_y, chip_w, chip_h) = (x + 6, y + 3, w - 12, h - 6);
        d.draw_rectangle_rounded(
            Rectangle::new(chip_x as f32, chip_y as f32, chip_w as f32, chip_h as f32),
            0.08,
            6,
            self.theme.chip,
        );
        let (cx, cw) = (chip_x + 14, chip_w - 28);

        // Text line at the top, sparkline in the middle, stats always pinned
        // to the bottom of the chip and centered horizontally.
        let text_y = chip_y + 10;
        self.draw_label_value_line(
            d,
            &m.name,
            &value_buf,
            &m.unit,
            cx,
            text_y,
            cw,
            text_line_h,
            10,
            value_size,
            label_size,
            unit_size,
            dir,
            dir_alpha,
        );

        let spark_top = text_y + text_line_h + 4;
        let stats_y = chip_y + chip_h - stats_h as i32 - 10;
        let mut spark_h = (stats_y - 6 - spark_top) as f32;
        let show_spark = spark_h >= 24.0;
        if spark_h > 90.0 {
            spark_h = 90.0;
        }
        if show_spark {
            self.draw_sparkline(d, hist, metric_idx, cx, spark_top, cw, spark_h as i32);
        }

        if let Some((min, max, avg)) = hist.and_then(|h| h.metric_stats(metric_idx)) {
            let stats_buf = format!(
                "min {}  \u{B7}  avg {}  \u{B7}  max {}",
                fmt_g(layout_round2(min)),
                fmt_g(layout_round2(avg)),
                fmt_g(layout_round2(max))
            );
            let bounds = self.measure(&stats_buf, 12.0);
            self.draw_text(
                d,
                &stats_buf,
                (chip_x as f32 + (chip_w as f32 - bounds.x) / 2.0) as i32,
                stats_y,
                12.0,
                self.theme.label_text,
            );
        }
    }

    fn draw_detail_panel(
        &mut self,
        d: &mut RaylibDrawHandle,
        sensors: &[SensorData],
        histories: &[SensorHistory],
        screen_w: i32,
        screen_h: i32,
    ) {
        let detail = self.detail_sensor as usize;
        let sensor = &sensors[detail];
        let hist = &histories[detail];

        // Dim overlay, then the centered panel
        d.draw_rectangle(0, 0, screen_w, screen_h, Color::new(0, 0, 0, 150));

        let panel_w = (640.0f32).min((screen_w - 2 * self.theme.padding) as f32);
        let panel_h = (460.0f32).min((screen_h - 2 * self.theme.padding) as f32);
        let panel = Rectangle::new(
            (screen_w as f32 - panel_w) / 2.0,
            (screen_h as f32 - panel_h) / 2.0,
            panel_w,
            panel_h,
        );
        d.draw_rectangle_rounded(panel, 0.05, 8, self.theme.panel);

        // Header: sensor name + id
        let name = if sensor.name.is_empty() { "sensor" } else { &sensor.name };
        self.draw_text(d, name, panel.x as i32 + self.theme.padding, panel.y as i32 + 14, 18.0, self.theme.title_text);
        let id_buf = format!("#{}", sensor.id);
        let id_bounds = self.measure(&id_buf, 14.0);
        self.draw_text(
            d,
            &id_buf,
            (panel.x + panel_w - self.theme.padding as f32 - id_bounds.x) as i32,
            panel.y as i32 + 17,
            14.0,
            self.theme.label_text,
        );

        let header_bottom = panel.y as i32 + 48;
        d.draw_line(
            panel.x as i32 + self.theme.padding,
            header_bottom,
            (panel.x + panel_w - self.theme.padding as f32) as i32,
            header_bottom,
            self.theme.divider,
        );

        // Metric blocks share the remaining space
        let metrics_top = header_bottom + 8;
        let metrics_h = (panel.y + panel_h) as i32 - metrics_top - self.theme.padding / 2;
        if sensor.metrics.is_empty() || metrics_h <= 0 {
            return;
        }
        let block_h = metrics_h / sensor.metrics.len() as i32;
        if block_h <= 0 {
            return;
        }

        let block_x = panel.x as i32 + self.theme.padding;
        let block_w = panel_w as i32 - 2 * self.theme.padding;

        for (i, metric) in sensor.metrics.iter().enumerate() {
            let idx = detail * MAX_METRICS + i;
            let dir_alpha = if self.dir_timer[idx] > 0.0 && self.theme.arrow_hold_seconds > 0.0 {
                self.dir_timer[idx] / self.theme.arrow_hold_seconds
            } else {
                0.0
            };
            let by = metrics_top + i as i32 * block_h;
            self.draw_detail_metric_block(
                d,
                metric,
                Some(hist),
                i,
                block_x,
                by,
                block_w,
                block_h,
                self.display[idx],
                self.dir[idx],
                dir_alpha,
            );
        }
    }

    pub fn draw(
        &mut self,
        d: &mut RaylibDrawHandle,
        sensors: &[SensorData],
        histories: &[SensorHistory],
        screen_w: i32,
        screen_h: i32,
        input: &FrameInput,
    ) {
        self.update_animations(sensors, input.dt);
        d.clear_background(self.theme.background);

        // Top bar — in-app logo + title
        let mut title_x = self.theme.padding;
        if let Some(logo) = &self.logo {
            d.draw_texture_ex(
                logo,
                Vector2::new(title_x as f32, (self.theme.top_bar_height - 22) as f32 / 2.0),
                0.0,
                1.0,
                Color::WHITE,
            );
            title_x += 28;
        }
        self.draw_text(d, "AETHER", title_x, 10, 16.0, self.theme.title_text);

        // Settings cogwheel: far top-right, timer sits left of it
        let icon_radius = 9.0f32;
        let icon_center = Vector2::new(
            (screen_w - self.theme.padding) as f32 - icon_radius,
            self.theme.top_bar_height as f32 / 2.0,
        );
        let icon_hit = Rectangle::new(
            icon_center.x - icon_radius - 4.0,
            icon_center.y - icon_radius - 4.0,
            (icon_radius + 4.0) * 2.0,
            (icon_radius + 4.0) * 2.0,
        );
        let icon_hovered = icon_hit.check_collision_point_rec(input.mouse);

        // --- Settings panel input: gear click or S key toggles it ---
        let toggle_settings = (input.mouse_pressed && icon_hovered) || input.key_s;
        if toggle_settings {
            self.settings_open = !self.settings_open;
            self.detail_sensor = -1; // the two modals are mutually exclusive
        }

        // Modals close only via ESC
        if input.key_escape {
            if self.detail_sensor >= 0 {
                self.detail_sensor = -1;
            } else if self.settings_open {
                self.settings_open = false;
            }
        }

        let time_text = format!("{:.1} s", input.time_s);
        let time_bounds = self.measure(&time_text, 14.0);
        self.draw_text(
            d,
            &time_text,
            (icon_center.x - icon_radius - 12.0 - time_bounds.x) as i32,
            12,
            14.0,
            self.theme.label_text,
        );

        self.draw_cogwheel(
            d,
            icon_center,
            icon_radius,
            if icon_hovered {
                self.theme.icon_hover_color
            } else {
                self.theme.icon_color
            },
        );

        if sensors.is_empty() {
            self.draw_centered_in_row(
                d,
                "No sensors loaded",
                0,
                self.theme.top_bar_height,
                screen_w,
                screen_h - self.theme.top_bar_height,
                20.0,
                self.theme.label_text,
            );
            if self.settings_open {
                self.draw_settings_panel(d, screen_w, screen_h, input);
            }
            return;
        }

        // Tab layout: fit as many minimum-size cards as possible; overflow
        // goes to the next tab (page).
        let avail_w = screen_w - 2 * self.theme.padding;
        let mut avail_h = screen_h - self.theme.top_bar_height - self.theme.padding;

        let tabs = crate::layout::layout_tabs(
            avail_w,
            avail_h,
            self.theme.min_card_width,
            self.theme.min_card_height,
            sensors.len(),
        );
        let (cols, rows) = (tabs.cols, tabs.rows);
        let (per_tab, page_count) = (tabs.per_tab, tabs.page_count);

        if self.current_page >= page_count {
            self.current_page = page_count - 1;
        }
        if self.current_page < 0 {
            self.current_page = 0;
        }

        // Keyboard: Alt/Cmd + 1..9 jumps to a tab
        if input.modifier_down {
            for (k, &pressed) in input.digits.iter().enumerate() {
                if pressed && k < page_count as usize {
                    self.current_page = k as i32;
                }
            }
        }
        // Arrow keys flip pages
        if input.key_right {
            self.current_page += 1;
        }
        if input.key_left {
            self.current_page -= 1;
        }
        self.current_page = self.current_page.clamp(0, page_count - 1);

        avail_h -= tabs.bottom_reserved;
        let card_width = avail_w / cols;
        let card_height = avail_h / rows;
        if card_width <= 0 || card_height <= 0 {
            return;
        }

        let start = self.current_page * per_tab;
        let end = (sensors.len() as i32).min(start + per_tab);
        // Typography grows with card size (reference: ~220px tall cards)
        let scale = (card_height * 100 / 220).clamp(100, 200) as f32 / 100.0;

        for i in start..end {
            let col = (i - start) % cols;
            let row = (i - start) / cols;
            let x = self.theme.padding + col * (card_width + self.theme.card_gap);
            let y = self.theme.top_bar_height + row * (card_height + self.theme.card_gap);
            self.draw_sensor_card(
                d,
                &sensors[i as usize],
                i as usize,
                histories.get(i as usize),
                x,
                y,
                card_width,
                card_height,
                scale,
            );

            // Click a card (when no modal is open) to open its detail view
            let card_rect = Rectangle::new(x as f32, y as f32, card_width as f32, card_height as f32);
            if !self.settings_open
                && self.detail_sensor < 0
                && input.mouse_pressed
                && card_rect.check_collision_point_rec(input.mouse)
            {
                self.detail_sensor = i;
            }
        }

        // Tab pager (only when there is more than one tab)
        if tabs.pager_visible {
            self.draw_tab_pager(d, page_count, screen_w, screen_h, input);
        }

        // Detail modal, then settings modal on top of everything
        if self.detail_sensor >= 0 {
            self.draw_detail_panel(d, sensors, histories, screen_w, screen_h);
        }
        if self.settings_open {
            self.draw_settings_panel(d, screen_w, screen_h, input);
        }
    }
}
