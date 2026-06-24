from __future__ import annotations

import os
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
LOCAL_VENDOR = SCRIPT_DIR / "vendor"
TARGET_ROOT = Path(os.environ.get("MBINK_AUDIO_TARGET_ROOT", r"D:\code\python\广州娜\自动外呼电话"))
AUDIO_PATH = Path(os.environ.get("MBINK_AUDIO_SMOKE_WAV", SCRIPT_DIR / "audio" / "mbink-audio-smoke-1khz.wav"))

if (LOCAL_VENDOR / "mbink").exists():
    sys.path.insert(0, str(LOCAL_VENDOR))
    os.environ.setdefault("MBINK_DLL_PATH", str(LOCAL_VENDOR / "mbink" / "bin"))
else:
    sys.path.insert(0, str(TARGET_ROOT / "vendor"))
    sys.path.insert(0, str(TARGET_ROOT))
    os.environ.setdefault("MBINK_DLL_PATH", str(TARGET_ROOT / "vendor" / "mbink" / "bin"))


def main() -> None:
    from mbink import App

    driver = os.environ.get("SDL_AUDIO_DRIVER", "SDL default")
    audio_src = AUDIO_PATH.as_posix()
    app = App(f"MBink Python audio smoke [{driver}]", 760, 280, resizable=True, gpu=True)
    app.load_html(f"""<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    html, body {{ margin: 0; width: 100%; height: 100%; font-family: Segoe UI, sans-serif; background: #f7f8fb; color: #172033; }}
    body {{ display: grid; place-items: center; }}
    main {{ width: 660px; display: grid; gap: 14px; padding: 22px; background: #fff; border: 1px solid #d8deeb; border-radius: 8px; }}
    h1 {{ margin: 0; font-size: 18px; }}
    p {{ margin: 0; color: #566274; font-size: 13px; line-height: 1.5; }}
    audio {{ width: 100%; height: 54px; }}
  </style>
</head>
<body>
  <main>
    <h1>MBink Python audio smoke</h1>
    <p>Driver: {driver}</p>
    <p>Audio: {audio_src}</p>
    <audio id="audio" controls loop src="{audio_src}"></audio>
  </main>
  <script>
    setTimeout(function() {{
      var audio = document.getElementById('audio');
      audio.volume = 1;
      audio.muted = false;
      audio.loop = true;
      audio.play();
    }}, 700);
  </script>
</body>
</html>""")
    app.run()


if __name__ == "__main__":
    main()
