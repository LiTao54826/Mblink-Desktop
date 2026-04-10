//go:build windows

package main

import (
	"log"

	"mbink-go/examples/internal/autorun"
)

func main() {
	app, err := autorun.NewApp("MBink Go Hello", 800, 600)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	err = app.LoadHTML(`
<!doctype html>
<html>
<body style="font-family: sans-serif; padding: 24px;">
    <h1>Hello from Go</h1>
    <p>If you can see this window, the Go binding works.</p>
</body>
</html>`)
	if err != nil {
		log.Fatal(err)
	}

	if err = autorun.Run(app, nil); err != nil {
		log.Fatal(err)
	}
}
