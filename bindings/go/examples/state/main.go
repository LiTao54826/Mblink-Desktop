//go:build windows && cgo

package main

import (
	"encoding/json"
	"fmt"
	"log"
	"strings"

	"mblink-go/examples/internal/autorun"
)

func main() {
	app, err := autorun.NewApp("MBlink Go State", 900, 640)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	state := app.State()
	if err = state.CreateInt("count", 1); err != nil {
		log.Fatal(err)
	}
	if err = state.CreateString("title", "State from Go"); err != nil {
		log.Fatal(err)
	}
	if err = state.CreateJSON("profile", map[string]any{"name": "Alice", "role": "admin"}); err != nil {
		log.Fatal(err)
	}

	if err = state.ProcessQueue(); err != nil {
		log.Fatal(err)
	}

	var report string
	err = app.Bind("report", func(args any) (any, error) {
		data, _ := json.Marshal(args)
		report = string(data)
		return args, nil
	})
	if err != nil {
		log.Fatal(err)
	}

	err = app.LoadHTML(`
<!doctype html>
<html>
<body style="font-family: sans-serif; padding: 24px;">
    <h1>Go state example</h1>
    <p>This example initializes window state from Go.</p>
    <pre id="out"></pre>
    <script>
        document.getElementById('out').textContent = JSON.stringify({
            count: host.state.get('count'),
            title: host.state.get('title'),
            profile: host.state.get('profile'),
        }, null, 2);
    </script>
</body>
</html>`)
	if err != nil {
		log.Fatal(err)
	}

	err = autorun.Run(app, func(tick int) error {
		if tick == 1 {
			return app.EvalJS(`backend.report({ count: host.state.get('count'), title: host.state.get('title'), profile: host.state.get('profile') })`)
		}
		if report == "" {
			return nil
		}
		if !strings.Contains(report, "State from Go") || !strings.Contains(report, `"count":1`) {
			return fmt.Errorf("unexpected state payload: %s", report)
		}
		app.Stop()
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}
}
