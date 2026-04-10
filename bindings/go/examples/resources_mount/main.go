//go:build windows

package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"

	"mbink-go/examples/internal/autorun"
	"mbink-go/mbink"
)

func main() {
	base := filepath.Join(os.TempDir(), "mbink-go-resource-mount-example")
	inputDir := filepath.Join(base, "input")
	packageFile := filepath.Join(base, "demo.mbk")
	must(os.RemoveAll(base))
	must(os.MkdirAll(inputDir, 0o755))

	html := `<!doctype html><html><body style="font-family:sans-serif;padding:24px;"><h1 id="title">MBink Go Resource Mount</h1><p id="desc">This page is loaded from a compiled resource package.</p></body></html>`
	must(os.WriteFile(filepath.Join(inputDir, "index.html"), []byte(html), 0o644))
	must(mbink.CompileResources(inputDir, packageFile, ""))

	app, err := autorun.NewApp("MBink Go Resources", 800, 600)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	var report string
	must(app.Bind("report", func(args any) (any, error) {
		data, _ := json.Marshal(args)
		report = string(data)
		return args, nil
	}))
	must(app.MountResourcePackage(packageFile, "", "/"))
	must(app.LoadHTMLFile("/index.html"))
	must(autorun.Run(app, func(tick int) error {
		if report == "" || report == "null" || report == `""` {
			if tick <= 6 {
				return app.EvalJS(`backend.report((function () {
					const title = document.getElementById('title');
					const desc = document.getElementById('desc');
					return title && desc ? (title.textContent + "\n" + desc.textContent) : null;
				})())`)
			}
			return fmt.Errorf("unexpected mounted content: %s", report)
		}
		if !strings.Contains(report, "MBink Go Resource Mount") {
			return fmt.Errorf("unexpected mounted content: %s", report)
		}
		app.Stop()
		return nil
	}))
}

func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}
