//go:build windows

package main

import (
	"fmt"
	"log"

	"mbink-go/examples/internal/autorun"
)

func main() {
	app, err := autorun.NewApp("MBink Go Window", 900, 640)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	must(app.SetPosition(120, 120))
	must(app.SetMinSize(480, 320))
	must(app.SetMaxSize(1400, 1000))
	must(app.SetResizable(true))
	must(app.SetAlwaysOnTop(false))

	x, y, err := app.Position()
	if err != nil {
		log.Fatal(err)
	}
	w, h, err := app.Size()
	if err != nil {
		log.Fatal(err)
	}

	html := fmt.Sprintf(`
<!doctype html>
<html>
<body style="font-family: sans-serif; padding: 24px;">
    <h1>MBink Go window controls</h1>
    <p>Window position: (%d, %d)</p>
    <p>Window size: %d x %d</p>
</body>
</html>`, x, y, w, h)
	must(app.LoadHTML(html))
	if !autorun.Enabled() {
		_ = app.DevtoolsOpen()
	}
	must(autorun.Run(app, func(tick int) error {
		if tick == 1 {
			app.Stop()
		}
		return nil
	}))
}

func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}
