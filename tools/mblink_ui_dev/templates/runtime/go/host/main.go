//go:build windows

package main

import (
	_ "embed"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"

	"mblink-go/mblink"
)

//go:embed resources/app.mbrp
var embeddedResourcePackage []byte

const embeddedConfig = `{"name":"MBlink","purpose":"minimal","runtime":"go","window":{"title":"MBlink","width":900,"height":640,"resizable":true}}`
const resourceConfigPath = "app/mblink.config.json"
const resourceAppPath = "/app/app.js"

type projectConfig struct {
	Name    string       `json:"name"`
	Purpose string       `json:"purpose"`
	Runtime string       `json:"runtime"`
	Window  windowConfig `json:"window"`
}

type windowConfig struct {
	Title      string `json:"title"`
	Width      int    `json:"width"`
	Height     int    `json:"height"`
	Borderless bool   `json:"borderless"`
	Resizable  *bool  `json:"resizable"`
}

func main() {
	root := projectRoot()
	resourcePackage := resolveResourcePackage(root)
	config := loadConfig(root, resourcePackage)
	title := firstNonEmpty(config.Window.Title, config.Name, "MBlink")
	width := withDefault(config.Window.Width, 900)
	height := withDefault(config.Window.Height, 640)
	resizable := true
	if config.Window.Resizable != nil {
		resizable = *config.Window.Resizable
	}

	appConfig := mblink.DefaultConfig().
		WithTitle(title).
		WithSize(width, height).
		WithBorderless(config.Window.Borderless).
		WithResizable(resizable)

	app, err := mblink.NewWithConfig(appConfig)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	var counter atomic.Int64
	var quitting atomic.Bool
	var topmost atomic.Bool
	handle := app.Handle()

	must(app.Bind("getTemplateInfo", func(args any) (any, error) {
		return map[string]any{
			"purpose": config.Purpose,
			"runtime": config.Runtime,
			"host":    "Go",
			"mode":    "host-runtime",
			"capabilities": map[string]any{
				"backend":    true,
				"borderless": config.Window.Borderless,
				"tray":       config.Purpose == "desktop-app",
				"reload":     false,
				"snapshot":   false,
			},
		}, nil
	}))

	must(app.Bind("incrementCounter", func(args any) (any, error) {
		value := counter.Add(numberField(args, "delta", 1))
		return map[string]any{"count": value, "source": "go"}, nil
	}))

	must(app.Bind("submitValidation", func(args any) (any, error) {
		value := strings.TrimSpace(stringField(args, "text"))
		message := "Go host accepted an empty value"
		if value != "" {
			message = "Go host accepted: " + value
		}
		return map[string]any{"ok": true, "value": value, "message": message}, nil
	}))

	must(app.Bind("trayAction", func(args any) (any, error) {
		action := firstNonEmpty(stringField(args, "action"), "status")
		switch action {
		case "hide":
			_ = handle.Hide()
		case "show":
			_ = handle.Show()
			_ = handle.Restore()
		case "quit":
			quitting.Store(true)
			handle.Stop()
		}
		return map[string]any{"ok": true, "action": action, "message": "Go tray action: " + action}, nil
	}))

	must(app.LoadHTML(`<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <style>
    html, body, #root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    * { box-sizing: border-box; }
  </style>
</head>
<body><div id="root"></div></body>
</html>`))

	if config.Purpose == "desktop-app" {
		must(app.CreateTray(title))
		must(app.SetTrayMenu([]map[string]any{
			{"id": "show", "label": "Show window"},
			{"id": "hide", "label": "Hide to tray"},
			{"type": "separator"},
			{"id": "toggle_top", "label": "Toggle always on top"},
			{"id": "quit", "label": "Quit"},
		}))
		must(app.OnCloseRequest(func() bool {
			if quitting.Load() {
				return true
			}
			_ = handle.Hide()
			return false
		}))
		must(app.OnTrayClick(func() {
			_ = handle.Show()
			_ = handle.Restore()
		}))
		must(app.OnTrayMenu(func(payload any) (any, error) {
			itemID := stringField(payload, "id")
			switch itemID {
			case "show":
				_ = handle.Show()
				_ = handle.Restore()
			case "hide":
				_ = handle.Hide()
			case "toggle_top":
				next := !topmost.Load()
				_ = handle.SetAlwaysOnTop(next)
				topmost.Store(next)
			case "quit":
				quitting.Store(true)
				handle.Stop()
			}
			return map[string]any{"ok": true, "id": itemID}, nil
		}))
	}

	if resourcePackage != "" {
		must(app.MountResourcePackage(resourcePackage, "", "/"))
		must(app.LoadJSFile(resourceAppPath))
	} else {
		must(app.LoadJSFile(filepath.Join(root, "ui", "app.js")))
	}
	app.Run()
}

func projectRoot() string {
	candidates := []string{}
	if cwd, err := os.Getwd(); err == nil {
		candidates = append(candidates, cwd)
	}
	if exe, err := os.Executable(); err == nil {
		candidates = append(candidates, filepath.Dir(exe))
	}
	for _, candidate := range candidates {
		if root := findProjectRoot(candidate); root != "" {
			return root
		}
	}
	if len(candidates) > 0 {
		return candidates[0]
	}
	return "."
}

func findProjectRoot(start string) string {
	current, err := filepath.Abs(start)
	if err != nil {
		return ""
	}
	for {
		if exists(filepath.Join(current, "mblink.config.json")) ||
			exists(filepath.Join(current, ".dist", "app.mbrp")) ||
			exists(filepath.Join(current, "ui", "app.js")) {
			return current
		}
		next := filepath.Dir(current)
		if next == current {
			return ""
		}
		current = next
	}
}

func resolveResourcePackage(root string) string {
	for _, candidate := range resourcePackageCandidates(root) {
		if isValidPackageFile(candidate) {
			return candidate
		}
	}
	if isValidEmbeddedPackage(embeddedResourcePackage) {
		path := filepath.Join(os.TempDir(), "mblink-template-go", executableStem(), "app.mbrp")
		if err := writeEmbeddedFile(path, embeddedResourcePackage); err != nil {
			log.Fatal(err)
		}
		return path
	}
	return ""
}

func resourcePackageCandidates(root string) []string {
	exeDir := ""
	if exe, err := os.Executable(); err == nil {
		exeDir = filepath.Dir(exe)
	}
	return []string{
		filepath.Join(exeDir, "app.mbrp"),
		filepath.Join(exeDir, "resources", "app.mbrp"),
		filepath.Join(root, ".dist", "app.mbrp"),
		filepath.Join(root, "host", "resources", "app.mbrp"),
	}
}

func loadConfig(root, resourcePackage string) projectConfig {
	content, err := os.ReadFile(filepath.Join(root, "mblink.config.json"))
	if err != nil && resourcePackage != "" {
		if file, resourceErr := mblink.LoadResourceFile(resourcePackage, resourceConfigPath, ""); resourceErr == nil {
			content = file.Data()
			err = nil
		}
	}
	if err != nil {
		content = []byte(embeddedConfig)
	}
	var config projectConfig
	if err := json.Unmarshal(content, &config); err != nil {
		log.Fatal(err)
	}
	if config.Purpose == "" {
		config.Purpose = "minimal"
	}
	if config.Runtime == "" {
		config.Runtime = "go"
	}
	return config
}

func exists(path string) bool {
	if path == "" {
		return false
	}
	_, err := os.Stat(path)
	return err == nil
}

func isValidPackageFile(path string) bool {
	data, err := os.ReadFile(path)
	return err == nil && isValidEmbeddedPackage(data)
}

func isValidEmbeddedPackage(data []byte) bool {
	return len(data) >= 4 && string(data[:4]) == "MBRP"
}

func executableStem() string {
	exe, err := os.Executable()
	if err != nil {
		return "app"
	}
	base := filepath.Base(exe)
	ext := filepath.Ext(base)
	return strings.TrimSuffix(base, ext)
}

func writeEmbeddedFile(path string, data []byte) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	existing, err := os.ReadFile(path)
	if err == nil && string(existing) == string(data) {
		return nil
	}
	if err := os.WriteFile(path, data, 0o644); err != nil {
		return fmt.Errorf("write embedded resource package: %w", err)
	}
	return nil
}

func stringField(args any, key string) string {
	values, ok := args.(map[string]any)
	if !ok {
		return ""
	}
	value, ok := values[key].(string)
	if !ok {
		return ""
	}
	return value
}

func numberField(args any, key string, fallback int64) int64 {
	values, ok := args.(map[string]any)
	if !ok {
		return fallback
	}
	switch value := values[key].(type) {
	case float64:
		return int64(value)
	case int64:
		return value
	case int:
		return int64(value)
	default:
		return fallback
	}
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if value != "" {
			return value
		}
	}
	return ""
}

func withDefault(value, fallback int) int {
	if value == 0 {
		return fallback
	}
	return value
}

func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}
