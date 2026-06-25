use mblink_sys::MBlinkConfig;

#[derive(Debug, Clone)]
pub struct AppBuilder {
    title: String,
    width: i32,
    height: i32,
    headless: bool,
    borderless: bool,
    transparent: bool,
    always_on_top: bool,
    resizable: bool,
    gpu: bool,
    fullscreen: bool,
    min_size: Option<(i32, i32)>,
    max_size: Option<(i32, i32)>,
}

impl Default for AppBuilder {
    fn default() -> Self {
        Self {
            title: "MBlink".to_string(),
            width: 800,
            height: 600,
            headless: false,
            borderless: false,
            transparent: false,
            always_on_top: false,
            resizable: true,
            gpu: true,
            fullscreen: false,
            min_size: None,
            max_size: None,
        }
    }
}

impl AppBuilder {
    pub fn build(self) -> crate::Result<crate::App> {
        crate::App::build(self)
    }

    pub fn title(mut self, value: impl Into<String>) -> Self {
        self.title = value.into();
        self
    }

    pub fn size(mut self, width: i32, height: i32) -> Self {
        self.width = width;
        self.height = height;
        self
    }

    pub fn headless(mut self, value: bool) -> Self {
        self.headless = value;
        self
    }

    pub fn borderless(mut self, value: bool) -> Self {
        self.borderless = value;
        self
    }

    pub fn transparent(mut self, value: bool) -> Self {
        self.transparent = value;
        self
    }

    pub fn always_on_top(mut self, value: bool) -> Self {
        self.always_on_top = value;
        self
    }

    pub fn resizable(mut self, value: bool) -> Self {
        self.resizable = value;
        self
    }

    pub fn gpu(mut self, value: bool) -> Self {
        self.gpu = value;
        self
    }

    pub fn fullscreen(mut self, value: bool) -> Self {
        self.fullscreen = value;
        self
    }

    pub fn min_size(mut self, width: i32, height: i32) -> Self {
        self.min_size = Some((width, height));
        self
    }

    pub fn max_size(mut self, width: i32, height: i32) -> Self {
        self.max_size = Some((width, height));
        self
    }

    pub(crate) fn apply_to_raw(&self, cfg: &mut MBlinkConfig) {
        cfg.width = self.width;
        cfg.height = self.height;
        cfg.headless = self.headless;
        cfg.borderless = self.borderless;
        cfg.transparent = self.transparent;
        cfg.always_on_top = self.always_on_top;
        cfg.resizable = self.resizable;
        cfg.gpu = self.gpu;
        cfg.fullscreen = self.fullscreen;
        if let Some((w, h)) = self.min_size {
            cfg.min_width = w;
            cfg.min_height = h;
        }
        if let Some((w, h)) = self.max_size {
            cfg.max_width = w;
            cfg.max_height = h;
        }
    }

    pub(crate) fn title_str(&self) -> &str {
        &self.title
    }
}
