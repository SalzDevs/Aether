//! Minimal deterministic random generator used by the sensor simulation.
//! Replaces the C `srand`/`rand` pair so results stay reproducible under a
//! seed. Seed state is per-thread, so parallel tests never interfere.

use std::cell::Cell;

thread_local! {
    static STATE: Cell<u64> = const { Cell::new(0) };
}

const MUL: u64 = 6364136223846793005;
const ADD: u64 = 1442695040888963407;

/// Seeds the generator (mirrors `srand`).
pub fn srand(seed: u64) {
    STATE.with(|s| s.set(seed));
}

/// Returns a pseudo-random float in [0, 1) (mirrors `rand() / RAND_MAX`).
pub fn rand01() -> f32 {
    STATE.with(|s| {
        let next = s.get().wrapping_mul(MUL).wrapping_add(ADD);
        s.set(next);
        ((next >> 33) as f64 / (1u64 << 31) as f64) as f32
    })
}
