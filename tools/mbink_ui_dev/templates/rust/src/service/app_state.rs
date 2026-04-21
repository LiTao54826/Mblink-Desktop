#[derive(Default)]
pub struct AppState {
    pub counter: u32,
}

impl AppState {
    pub fn increment(&mut self) -> u32 {
        self.counter += 1;
        self.counter
    }
}
