//go:build windows

package main

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"mblink-go/mblink"
)

type matrixState struct {
	mu       sync.Mutex
	sequence int
	rows     int
}

func main() {
	app, err := mblink.New("MBlink Layout Host Matrix (Go)", 1180, 820)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	state := &matrixState{rows: 8}

	app.Bind("getMatrixInfo", func(args any) (any, error) {
		purpose := strArg(args, "purpose")
		return map[string]any{
			"host": "Go", "runtime": "go", "mode": "host-runtime",
			"receivedPurpose": purpose,
			"capabilities": map[string]any{
				"backend": true, "asyncRoundTrip": true,
				"layoutMutation": true, "errorPath": true,
			},
		}, nil
	})

	app.Bind("mutateMatrix", func(args any) (any, error) {
		delta := intArg(args, "delta")
		state.mu.Lock()
		state.sequence++
		state.rows = clamp(state.rows+delta, 3, 24)
		seq, rows := state.sequence, state.rows
		state.mu.Unlock()
		return map[string]any{
			"ok": true, "source": "go", "sequence": seq, "rows": rows,
			"label": fmt.Sprintf("go mutation %d", seq),
		}, nil
	})

	app.Bind("validatePayload", func(args any) (any, error) {
		text := strings.TrimSpace(strArg(args, "text"))
		msg := fmt.Sprintf("go accepted %d chars", len([]rune(text)))
		if text == "" {
			msg = "go rejected empty text"
		}
		return map[string]any{
			"ok": text != "", "source": "go",
			"normalized": strings.ToUpper(text),
			"length": len([]rune(text)), "message": msg,
		}, nil
	})

	app.Bind("simulateFailure", func(args any) (any, error) {
		return map[string]any{
			"ok": false, "source": "go",
			"code":    "GO_INTENTIONAL_FAILURE",
			"message": fmt.Sprintf("intentional failure path reached for %s", strArg(args, "caseName")),
		}, nil
	})

	if err := app.LoadHTML(`<!doctype html><html><head><meta charset="utf-8"/></head><body><div id="root"></div></body></html>`); err != nil {
		log.Fatal(err)
	}

	mbrp := resolvePackage()
	if mbrp != "" {
		if err := app.MountResourcePackage(mbrp, "", "/"); err != nil {
			log.Fatal(err)
		}
		app.LoadJSFile("/app.js")
	}

	app.Run()
}

func resolvePackage() string {
	exe, _ := os.Executable()
	exeDir := filepath.Dir(exe)
	candidates := []string{
		filepath.Join(exeDir, "app.mbrp"),
		// reuse Rust example's built package
		filepath.Join(exeDir, "..", "..", "layout_host_matrix_rust", ".dist", "app.mbrp"),
		filepath.Join("examples", "layout_host_matrix_rust", ".dist", "app.mbrp"),
	}
	for _, p := range candidates {
		if _, err := os.Stat(p); err == nil {
			abs, _ := filepath.Abs(p)
			return abs
		}
	}
	return ""
}

func strArg(args any, key string) string {
	if m, ok := args.(map[string]any); ok {
		if v, ok := m[key].(string); ok {
			return v
		}
	}
	return ""
}

func intArg(args any, key string) int {
	if m, ok := args.(map[string]any); ok {
		if v, ok := m[key].(float64); ok {
			return int(v)
		}
	}
	return 0
}

func clamp(v, lo, hi int) int {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}
