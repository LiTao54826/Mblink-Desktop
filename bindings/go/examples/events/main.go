//go:build windows

package main

import (
	"fmt"
	"log"

	"mblink-go/examples/internal/autorun"
)

func main() {
	app, err := autorun.NewApp("MBlink Go Events", 900, 640)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	var resized bool
	must(app.OnResize(func(w, h int) {
		resized = true
		log.Printf("resize: %dx%d", w, h)
	}))
	must(app.OnFocus(func() { log.Println("focus") }))
	must(app.OnBlur(func() { log.Println("blur") }))
	must(app.OnClose(func() { log.Println("closed") }))
	must(app.OnCloseRequest(func() bool {
		log.Println("close requested")
		return false
	}))

	must(app.LoadHTML(`
<!doctype html>
<html>
<body style="font-family: sans-serif; padding: 24px;">
    <h1>Go events example</h1>
    <p>Try focusing, resizing, and closing the window.</p>
    <p>Callback output is printed by Go.</p>
</body>
</html>`))

	must(autorun.Run(app, func(tick int) error {
		switch tick {
		case 1:
			return app.SetSize(920, 660)
		case 2:
			if !resized {
				return fmt.Errorf("resize callback not triggered")
			}
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
