//go:build windows

package autorun

import (
	"os"
	"strconv"

	"mblink-go/mblink"
)

func Enabled() bool {
	return os.Getenv("MBLINK_AUTORUN") == "1"
}

func Headless() bool {
	return os.Getenv("MBLINK_AUTORUN_HEADLESS") == "1"
}

func NewApp(title string, width, height int) (*mblink.App, error) {
	if !Enabled() {
		return mblink.New(title, width, height)
	}
	cfg := mblink.DefaultConfig().
		WithTitle(title).
		WithSize(width, height).
		WithGPU(false)
	if Headless() {
		cfg = cfg.WithHeadless(true)
	}
	return mblink.NewWithConfig(cfg)
}

func Run(app *mblink.App, step func(int) error) error {
	if !Enabled() {
		app.Run()
		return nil
	}
	maxTicks := 12
	if value := os.Getenv("MBLINK_AUTORUN_TICKS"); value != "" {
		if n, err := strconv.Atoi(value); err == nil && n > 0 {
			maxTicks = n
		}
	}
	var tick int
	var runErr error
	if err := app.OnUpdate(func(float32) {
		tick++
		if runErr == nil && step != nil {
			runErr = step(tick)
		}
		if runErr != nil || tick >= maxTicks {
			app.Stop()
		}
	}); err != nil {
		return err
	}
	app.Run()
	return runErr
}
