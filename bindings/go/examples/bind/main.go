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
	app, err := autorun.NewApp("MBlink Go Bind", 900, 600)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	err = app.Bind("greet", func(args any) (any, error) {
		name := "world"
		if m, ok := args.(map[string]any); ok {
			if v, ok := m["name"].(string); ok && v != "" {
				name = v
			}
		}
		return map[string]any{"message": "Hello, " + name + "!", "from": "Go"}, nil
	})
	if err != nil {
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
    <h1>Go bind example</h1>
    <button id="btn">Call backend.greet()</button>
    <pre id="out"></pre>
    <script>
        document.getElementById('btn').onclick = async () => {
            const result = await backend.greet({ name: 'MBlink' });
            document.getElementById('out').textContent = JSON.stringify(result, null, 2);
        };
    </script>
</body>
</html>`)
	if err != nil {
		log.Fatal(err)
	}

	err = autorun.Run(app, func(tick int) error {
		if tick == 1 {
			return app.EvalJS(`(() => { try { const v = backend.greet({ name: 'MBlink' }); backend.report(v); } catch (err) { backend.report({ error: String(err) }); } })()`)
		}
		if report == "" {
			return nil
		}
		if !strings.Contains(report, "Hello, MBlink!") {
			return fmt.Errorf("unexpected bind result: %s", report)
		}
		app.Stop()
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}
}
