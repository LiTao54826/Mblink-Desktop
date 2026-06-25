//go:build windows

package main

import (
	"log"
	"sync/atomic"

	"mblink-go/examples/internal/autorun"
	"mblink-go/mblink"
)

func main() {
	cfg := mblink.DefaultConfig().
		WithTitle("MBlink Go Tray Demo").
		WithSize(1000, 700).
		WithBorderless(true).
		WithMinSize(900, 620).
		WithResizable(true)
	if autorun.Enabled() {
		cfg = cfg.WithGPU(false)
	}
	app, err := mblink.NewWithConfig(cfg)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	var quitting atomic.Bool
	var topmost atomic.Bool
	handle := app.Handle()

	must(app.LoadHTML(`<!doctype html><html><body style="font-family:sans-serif;padding:24px;background:#0f172a;color:#e5e7eb;"><h1>Go Tray Demo</h1><p>关闭窗口会隐藏到托盘，点击托盘可恢复。</p></body></html>`))
	must(app.CreateTray("MBlink Go Tray Demo"))
	must(app.SetTrayMenu([]map[string]any{{"id": "show", "label": "显示窗口"}, {"id": "toggle_top", "label": "置顶窗口", "checked": false}, {"type": "separator"}, {"id": "quit", "label": "退出"}}))
	must(app.OnCloseRequest(func() bool {
		if quitting.Load() {
			return true
		}
		log.Println("close requested -> hide to tray")
		_ = app.Hide()
		return false
	}))
	must(app.OnTrayClick(func() {
		log.Println("tray clicked -> restore window")
		_ = handle.Show()
		_ = handle.Restore()
	}))
	must(app.OnTrayMenu(func(payload any) (any, error) {
		id := menuID(payload)
		switch id {
		case "show":
			_ = handle.Show()
			_ = handle.Restore()
			_ = handle.SetTitle("MBlink Go Tray Demo")
		case "toggle_top":
			next := !topmost.Load()
			_ = handle.SetAlwaysOnTop(next)
			topmost.Store(next)
		case "quit":
			quitting.Store(true)
			handle.Stop()
		}
		return map[string]any{"ok": true, "id": id}, nil
	}))

	must(autorun.Run(app, func(tick int) error {
		if tick == 2 {
			app.Stop()
		}
		return nil
	}))
}

func menuID(payload any) string {
	if m, ok := payload.(map[string]any); ok {
		if v, ok := m["id"].(string); ok {
			return v
		}
	}
	return ""
}

func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}
