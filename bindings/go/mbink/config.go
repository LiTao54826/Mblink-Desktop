package mbink

func (c Config) WithTitle(value string) Config          { c.Title = value; return c }
func (c Config) WithSize(width, height int) Config      { c.Width, c.Height = width, height; return c }
func (c Config) WithHeadless(value bool) Config         { c.Headless = value; return c }
func (c Config) WithBorderless(value bool) Config       { c.Borderless = value; return c }
func (c Config) WithTransparent(value bool) Config      { c.Transparent = value; return c }
func (c Config) WithAlwaysOnTop(value bool) Config      { c.AlwaysOnTop = value; return c }
func (c Config) WithResizable(value bool) Config        { c.Resizable = value; return c }
func (c Config) WithGPU(value bool) Config              { c.GPU = value; return c }
func (c Config) WithFullscreen(value bool) Config       { c.Fullscreen = value; return c }
func (c Config) WithResizeBorderWidth(value int) Config { c.ResizeBorderWidth = value; return c }
func (c Config) WithMinSize(width, height int) Config {
	c.MinWidth, c.MinHeight = width, height
	return c
}
func (c Config) WithMaxSize(width, height int) Config {
	c.MaxWidth, c.MaxHeight = width, height
	return c
}
