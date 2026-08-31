//! Pure layout and formatting math used by the dashboard.
//! No rendering dependencies, so it is unit-testable in isolation.

/// Rounds to two decimals so animated values render compactly.
pub fn layout_round2(v: f32) -> f32 {
    let scaled = (v * 100.0 + if v >= 0.0 { 0.5 } else { -0.5 }) as i64;
    scaled as f32 / 100.0
}

/// Maps value from [inMin, inMax] to [outMin, outMax].
/// When inMin == inMax, returns the midpoint of the output range.
pub fn layout_map_range(value: f32, in_min: f32, in_max: f32, out_min: f32, out_max: f32) -> f32 {
    if in_max > in_min {
        let t = (value - in_min) / (in_max - in_min);
        out_min + t * (out_max - out_min)
    } else {
        (out_min + out_max) / 2.0
    }
}

/// Computes min and max of a value series. Empty input yields (0, 0).
pub fn layout_min_max(values: &[f32]) -> (f32, f32) {
    if values.is_empty() {
        return (0.0, 0.0);
    }
    let mut min = values[0];
    let mut max = values[0];
    for &v in &values[1..] {
        if v < min {
            min = v;
        }
        if v > max {
            max = v;
        }
    }
    (min, max)
}

/// Result of laying out the sensor tab grid.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct TabLayout {
    pub cols: i32,
    pub rows: i32,
    /// cards per tab (cols * rows)
    pub per_tab: i32,
    /// total tabs needed
    pub page_count: i32,
    /// true when more than one tab is needed
    pub pager_visible: bool,
    /// pixels reserved at the bottom for the pager
    pub bottom_reserved: i32,
}

/// Fits as many minimum-size cards as possible into the available area.
/// When sensors exceed one tab, space is reserved for the bottom pager.
pub fn layout_tabs(
    avail_width: i32,
    avail_height: i32,
    min_card_width: f32,
    min_card_height: f32,
    sensor_count: usize,
) -> TabLayout {
    let mut cols = (avail_width as f32 / min_card_width) as i32;
    if cols < 1 {
        cols = 1;
    }
    let rows_full = (avail_height as f32 / min_card_height) as i32;
    let rows_full = rows_full.max(1);

    let per_tab_full = cols * rows_full;
    let pager_needed = (sensor_count as i32) > per_tab_full;

    let mut rows = rows_full;
    let mut bottom_reserved = 0;
    if pager_needed {
        bottom_reserved = 34; // bottom strip for the pager
        rows = ((avail_height - bottom_reserved) as f32 / min_card_height) as i32;
        if rows < 1 {
            rows = 1;
        }
    }

    let per_tab = cols * rows;
    let page_count = (sensor_count as i32 + per_tab - 1) / per_tab;

    TabLayout {
        cols,
        rows,
        per_tab,
        page_count,
        pager_visible: pager_needed,
        bottom_reserved,
    }
}

/// Builds the visible tab sequence with collapsing: first and last pages
/// are always shown, the current page +-1, and -1 markers in larger gaps.
/// Returns at most `out_max` slots.
pub fn layout_page_list(page_count: i32, current_page_1based: i32, out_max: usize) -> Vec<i32> {
    let mut out: Vec<i32> = Vec::new();

    if page_count <= out_max as i32 {
        for p in 1..=page_count {
            out.push(p);
        }
        return out;
    }

    let candidates = [
        1,
        current_page_1based - 1,
        current_page_1based,
        current_page_1based + 1,
        page_count,
    ];
    let mut prev = 0;
    for &p in &candidates {
        if p < 1 || p > page_count {
            continue;
        }
        if !out.is_empty() && p == prev {
            continue; // dedupe (candidates are sorted)
        }
        if !out.is_empty() && p - prev > 1 && out.len() < out_max - 1 {
            out.push(-1); // gap marker
        }
        if out.len() >= out_max {
            break;
        }
        out.push(p);
        prev = p;
    }

    // guarantee the last page made it (drop a gap marker if needed)
    while !out.is_empty() && *out.last().unwrap() != page_count {
        if *out.last().unwrap() == -1 {
            out.pop();
            continue;
        }
        if out.len() < out_max {
            out.push(page_count);
            break;
        }
        out.pop(); // drop marker + neighbor to make room
        out.pop();
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round2_basic() {
        assert_eq!(layout_round2(22.567), 22.57);
        assert_eq!(layout_round2(45.0), 45.0);
        assert_eq!(layout_round2(0.004), 0.0);
        assert_eq!(layout_round2(1013.256), 1013.26);
    }

    #[test]
    fn round2_negative() {
        assert_eq!(layout_round2(-1.004), -1.0);
        assert_eq!(layout_round2(-2.345), -2.35); // rounds away from zero at .5
    }

    #[test]
    fn map_range_endpoints() {
        // higher value maps to the top (smaller y)
        assert_eq!(layout_map_range(0.0, 0.0, 10.0, 100.0, 0.0), 100.0);
        assert_eq!(layout_map_range(10.0, 0.0, 10.0, 100.0, 0.0), 0.0);
        assert_eq!(layout_map_range(5.0, 0.0, 10.0, 100.0, 0.0), 50.0);
    }

    #[test]
    fn map_range_flat_series_hits_midpoint() {
        assert_eq!(layout_map_range(7.0, 7.0, 7.0, 10.0, 30.0), 20.0);
    }

    #[test]
    fn map_range_identity() {
        assert_eq!(layout_map_range(3.0, 0.0, 10.0, 0.0, 10.0), 3.0);
    }

    #[test]
    fn min_max_basic() {
        let (min, max) = layout_min_max(&[4.0, -2.0, 9.5, 3.0]);
        assert_eq!(min, -2.0);
        assert_eq!(max, 9.5);
    }

    #[test]
    fn min_max_single_and_empty() {
        let (min, max) = layout_min_max(&[5.0]);
        assert_eq!(min, 5.0);
        assert_eq!(max, 5.0);

        let (min, max) = layout_min_max(&[]);
        assert_eq!(min, 0.0);
        assert_eq!(max, 0.0);
    }

    #[test]
    fn tabs_small_window_six_per_tab() {
        // 800x450 window: 768x400 available -> 3 cols x 2 rows = 6 per tab
        let tl = layout_tabs(768, 400, 230.0, 160.0, 30);
        assert_eq!(tl.cols, 3);
        assert_eq!(tl.rows, 2);
        assert_eq!(tl.per_tab, 6);
        assert_eq!(tl.page_count, 5);
        assert!(tl.pager_visible);
        assert!(tl.bottom_reserved > 0);
    }

    #[test]
    fn tabs_single_page_hides_pager() {
        let tl = layout_tabs(768, 400, 230.0, 160.0, 5);
        assert_eq!(tl.page_count, 1);
        assert!(!tl.pager_visible);
        assert_eq!(tl.bottom_reserved, 0);
    }

    #[test]
    fn tabs_large_window_more_per_tab() {
        // 1280x800 window: 1248x750 available -> 5 cols x 4 rows = 20 per tab
        let tl = layout_tabs(1248, 750, 230.0, 160.0, 30);
        assert_eq!(tl.cols, 5);
        assert_eq!(tl.per_tab, 20);
        assert_eq!(tl.page_count, 2);
    }

    #[test]
    fn tabs_narrow_window_single_column() {
        let tl = layout_tabs(200, 400, 230.0, 160.0, 10);
        assert_eq!(tl.cols, 1);
        assert_eq!(tl.page_count, (10 + tl.per_tab - 1) / tl.per_tab);
    }

    #[test]
    fn tabs_empty_registry() {
        let tl = layout_tabs(768, 400, 230.0, 160.0, 0);
        assert_eq!(tl.page_count, 0);
        assert!(!tl.pager_visible);
    }

    #[test]
    fn page_list_all_pages_when_few() {
        let out = layout_page_list(5, 1, 9);
        assert_eq!(out, vec![1, 2, 3, 4, 5]);
    }

    #[test]
    fn page_list_collapses_first_page() {
        let out = layout_page_list(10, 1, 9);
        assert_eq!(out, vec![1, 2, -1, 10]);
    }

    #[test]
    fn page_list_collapses_middle_page() {
        let out = layout_page_list(10, 5, 9);
        assert_eq!(out, vec![1, -1, 4, 5, 6, -1, 10]);
    }

    #[test]
    fn page_list_collapses_last_page() {
        let out = layout_page_list(10, 10, 9);
        assert_eq!(out, vec![1, -1, 9, 10]);
    }

    #[test]
    fn page_list_never_exceeds_out_max() {
        for pages in 1..=60 {
            for cur in 1..=pages {
                let out = layout_page_list(pages, cur, 9);
                assert!(!out.is_empty() && out.len() <= 9);
                assert_eq!(out[0], 1); // first page always present
                assert_eq!(*out.last().unwrap(), pages); // last page always present
                assert!(out.contains(&cur)); // current page always visible
            }
        }
    }
}
