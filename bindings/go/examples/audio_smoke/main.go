//go:build windows

package main

import (
	"html"
	"log"
	"os"
	"path/filepath"

	"mblink-go/examples/internal/autorun"
)

func main() {
	exePath, err := os.Executable()
	if err != nil {
		log.Fatal(err)
	}
	exeDir := filepath.Dir(exePath)
	audioPath := filepath.Join(exeDir, "audio", "mblink-audio-smoke-1khz.wav")
	if override := os.Getenv("MBLINK_AUDIO_SMOKE_WAV"); override != "" {
		audioPath = override
	}
	if _, err := os.Stat(audioPath); err != nil {
		log.Fatalf("audio file not found: %s", audioPath)
	}
	audioURL := filepath.ToSlash(audioPath)

	app, err := autorun.NewApp("MBlink Go audio smoke", 760, 280)
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()

	driver := os.Getenv("SDL_AUDIO_DRIVER")
	if driver == "" {
		driver = "SDL default"
	}
	htmlDoc := `<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    html, body { margin: 0; width: 100%; height: 100%; font-family: Segoe UI, sans-serif; background: #f7f8fb; color: #172033; }
    body { display: grid; place-items: center; }
    main { width: 660px; display: grid; gap: 14px; padding: 22px; background: #fff; border: 1px solid #d8deeb; border-radius: 8px; }
    h1 { margin: 0; font-size: 18px; }
    p { margin: 0; color: #566274; font-size: 13px; line-height: 1.5; }
    audio { width: 100%; height: 54px; }
  </style>
</head>
<body>
  <main>
    <h1>MBlink Go audio smoke</h1>
    <p>Driver: ` + html.EscapeString(driver) + `</p>
    <p>Audio: ` + html.EscapeString(audioURL) + `</p>
    <audio id="audio" controls loop src="` + html.EscapeString(audioURL) + `"></audio>
  </main>
  <script>
    setTimeout(function() {
      var audio = document.getElementById('audio');
      audio.volume = 1;
      audio.muted = false;
      audio.loop = true;
      audio.play();
    }, 700);
  </script>
</body>
</html>`

	if err := app.LoadHTML(htmlDoc); err != nil {
		log.Fatal(err)
	}

	if err = autorun.Run(app, nil); err != nil {
		log.Fatal(err)
	}
}
