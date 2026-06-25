//go:build windows

package main

import (
	"encoding/json"
	"fmt"
	"log"
	"strings"

	"mblink-go/examples/internal/autorun"
)

func main() {
	app, err := autorun.NewApp("MBlink Go Shared", 900, 600)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	shared, err := app.Shared("data")
	if err != nil {
		log.Fatal(err)
	}
	must(shared.SetInt("count", 1))
	must(shared.SetString("title", "Shared state from Go"))
	must(shared.SetJSON("user", map[string]any{"name": "Alice", "role": "admin"}))

	var report string
	must(app.Bind("report", func(args any) (any, error) {
		data, _ := json.Marshal(args)
		report = string(data)
		return args, nil
	}))

	html := fmt.Sprintf(`
<!doctype html>
<html>
<body style="font-family: sans-serif; padding: 24px;">
    <h1 id="title"></h1>
    <pre id="out"></pre>
    <script>
        const render = () => {
            document.getElementById('title').textContent = %s.title;
            document.getElementById('out').textContent = JSON.stringify({
                count: %s.count,
                user: %s.user,
            }, null, 2);
        };
        render();
    </script>
</body>
</html>`, "data", "data", "data")
	must(app.LoadHTML(html))

	must(autorun.Run(app, func(tick int) error {
		if tick == 1 {
			return app.EvalJS(`backend.report({ title: data.title, count: data.count, user: data.user })`)
		}
		if report == "" {
			return nil
		}
		if !strings.Contains(report, "Shared state from Go") || !strings.Contains(report, `"count":1`) {
			return fmt.Errorf("unexpected shared payload: %s", report)
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
