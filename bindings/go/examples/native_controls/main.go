//go:build windows

package main

import (
	"log"

	"mbink-go/examples/internal/autorun"
)

func main() {
	app, err := autorun.NewApp("MBink Go Native Controls", 1180, 760)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	must(app.LoadHTML(`<!doctype html><html><head><style>*{box-sizing:border-box}html,body{width:100%;height:100%;margin:0;background:#0f172a;color:#e2e8f0;font-family:"Segoe UI",sans-serif}.layout{display:grid;grid-template-columns:340px minmax(0,1fr);gap:12px;width:100vw;height:100vh;padding:12px}.panel{background:#111827;border:1px solid #334155;border-radius:12px;padding:14px;min-height:0}.control-host{min-height:0;padding:0;overflow:hidden}.right{display:grid;grid-template-rows:minmax(0,1fr) minmax(0,1fr);gap:12px;min-height:0}logview,terminal{width:100%;height:100%;display:block;background:#020617;border:1px solid #334155;border-radius:12px;overflow:hidden}</style></head><body><div class="layout"><div class="panel"><h1>Go Native Controls Demo</h1><p>LogView 与 Terminal 由 Go 初始化。</p></div><div class="right"><div class="panel control-host"><logview id="logs"></logview></div><div class="panel control-host"><terminal id="term"></terminal></div></div></div></body></html>`))

	logs, err := app.LogView("logs")
	if err != nil {
		log.Fatal(err)
	}
	must(logs.Append("INFO", "go", "native controls demo booted"))
	must(logs.Append("INFO", "go", "logview acquired from Go binding"))

	term, err := app.Terminal("term")
	if err != nil {
		log.Fatal(err)
	}
	term.Resize(28, 100)
	must(term.Write("MBink terminal ready.\r\n"))
	must(term.Write("This output is written by Go before app.Run().\r\n"))
	_ = term.Execute("echo hello from terminal.execute")

	must(autorun.Run(app, func(tick int) error {
		if tick == 2 {
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
