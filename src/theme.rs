//! Central design tokens for the dashboard.
//! All visual identity decisions live here.

use raylib::prelude::Color;

#[derive(Clone, Copy, Debug)]
pub struct DashboardTheme {
    // Palette
    pub background: Color, // window background
    pub panel: Color,      // sensor card fill
    pub chip: Color,       // metric chip fill inside a card (darker than panel)
    pub title_text: Color, // headers, app title, sensor names
    pub label_text: Color, // metric names, units, secondary info
    pub value_text: Color, // metric readings
    pub up_color: Color,   // indicator: reading increased
    pub down_color: Color, // indicator: reading decreased
    pub divider: Color,    // hairlines inside cards

    // Spacing
    pub padding: i32,           // outer margin + inner card padding
    pub card_gap: i32,          // space between cards
    pub top_bar_height: i32,    // reserved strip above the grid
    pub bottom_bar_height: i32, // reserved strip below the grid (tab pager)

    // Tab layout: minimum comfortable card size; a tab holds as many
    // of these as fit in the window
    pub min_card_width: f32,
    pub min_card_height: f32,

    // Typography (base atlas size the font is loaded at)
    pub font_atlas_size: f32,

    // Icons
    pub icon_color: Color,       // idle icon tint (dim)
    pub icon_hover_color: Color, // icon tint while hovered

    // Panels
    pub panel_hover_color: Color, // hovered row inside a panel

    // Motion
    pub arrow_hold_seconds: f32, // how long up/down indicators stay visible
    pub value_smoothing: f32,    // how fast displayed values chase actual ones (per second)
}

const fn rgb(r: u8, g: u8, b: u8) -> Color {
    Color { r, g, b, a: 255 }
}

pub const THEME_PRESETS: [DashboardTheme; 5] = [
    // 0: Dark (same as the original THEME)
    DashboardTheme {
        background: rgb(24, 26, 32),
        panel: rgb(33, 37, 47),
        chip: rgb(26, 29, 38),
        title_text: rgb(235, 240, 250),
        label_text: rgb(145, 152, 168),
        value_text: rgb(110, 190, 255),
        up_color: rgb(130, 205, 140),
        down_color: rgb(235, 135, 120),
        divider: rgb(52, 57, 70),
        padding: 16,
        card_gap: 12,
        top_bar_height: 34,
        bottom_bar_height: 34,
        min_card_width: 230.0,
        min_card_height: 160.0,
        font_atlas_size: 64.0,
        icon_color: rgb(145, 152, 168),
        icon_hover_color: rgb(235, 240, 250),
        panel_hover_color: rgb(44, 49, 62),
        arrow_hold_seconds: 1.5,
        value_smoothing: 10.0,
    },
    // 1: Blue
    DashboardTheme {
        background: rgb(22, 27, 34),
        panel: rgb(30, 38, 48),
        chip: rgb(24, 30, 38),
        title_text: rgb(232, 240, 250),
        label_text: rgb(140, 158, 178),
        value_text: rgb(90, 170, 255),
        up_color: rgb(120, 200, 150),
        down_color: rgb(240, 140, 120),
        divider: rgb(48, 58, 72),
        padding: 16,
        card_gap: 12,
        top_bar_height: 34,
        bottom_bar_height: 34,
        min_card_width: 230.0,
        min_card_height: 160.0,
        font_atlas_size: 64.0,
        icon_color: rgb(140, 158, 178),
        icon_hover_color: rgb(232, 240, 250),
        panel_hover_color: rgb(40, 50, 64),
        arrow_hold_seconds: 1.5,
        value_smoothing: 10.0,
    },
    // 2: Green
    DashboardTheme {
        background: rgb(22, 30, 26),
        panel: rgb(30, 42, 36),
        chip: rgb(24, 34, 28),
        title_text: rgb(232, 248, 238),
        label_text: rgb(140, 170, 155),
        value_text: rgb(90, 215, 150),
        up_color: rgb(120, 225, 160),
        down_color: rgb(240, 160, 120),
        divider: rgb(46, 64, 54),
        padding: 16,
        card_gap: 12,
        top_bar_height: 34,
        bottom_bar_height: 34,
        min_card_width: 230.0,
        min_card_height: 160.0,
        font_atlas_size: 64.0,
        icon_color: rgb(140, 170, 155),
        icon_hover_color: rgb(232, 248, 238),
        panel_hover_color: rgb(40, 56, 46),
        arrow_hold_seconds: 1.5,
        value_smoothing: 10.0,
    },
    // 3: Amber
    DashboardTheme {
        background: rgb(32, 28, 22),
        panel: rgb(44, 38, 30),
        chip: rgb(36, 31, 24),
        title_text: rgb(250, 244, 232),
        label_text: rgb(178, 162, 140),
        value_text: rgb(245, 190, 90),
        up_color: rgb(150, 210, 130),
        down_color: rgb(240, 130, 110),
        divider: rgb(64, 56, 44),
        padding: 16,
        card_gap: 12,
        top_bar_height: 34,
        bottom_bar_height: 34,
        min_card_width: 230.0,
        min_card_height: 160.0,
        font_atlas_size: 64.0,
        icon_color: rgb(178, 162, 140),
        icon_hover_color: rgb(250, 244, 232),
        panel_hover_color: rgb(56, 48, 38),
        arrow_hold_seconds: 1.5,
        value_smoothing: 10.0,
    },
    // 4: Purple
    DashboardTheme {
        background: rgb(28, 24, 34),
        panel: rgb(38, 32, 46),
        chip: rgb(30, 26, 36),
        title_text: rgb(244, 238, 250),
        label_text: rgb(162, 150, 178),
        value_text: rgb(190, 140, 255),
        up_color: rgb(140, 215, 160),
        down_color: rgb(240, 140, 170),
        divider: rgb(58, 50, 70),
        padding: 16,
        card_gap: 12,
        top_bar_height: 34,
        bottom_bar_height: 34,
        min_card_width: 230.0,
        min_card_height: 160.0,
        font_atlas_size: 64.0,
        icon_color: rgb(162, 150, 178),
        icon_hover_color: rgb(244, 238, 250),
        panel_hover_color: rgb(50, 44, 62),
        arrow_hold_seconds: 1.5,
        value_smoothing: 10.0,
    },
];

pub const THEME_PRESET_COUNT: usize = THEME_PRESETS.len();

/// Applies the preset at `index`, mirroring the C `ApplyThemePreset`.
/// Out-of-range indices are ignored.
pub fn apply_theme_preset(theme: &mut DashboardTheme, settings_theme: &mut i32, index: i32) {
    if index < 0 || index as usize >= THEME_PRESET_COUNT {
        return;
    }
    *settings_theme = index;
    *theme = THEME_PRESETS[index as usize];
}
