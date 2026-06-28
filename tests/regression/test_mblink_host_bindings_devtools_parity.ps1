param(
    [switch]$AllowGoToolchainMissing
)

$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Set-Location $repoRoot

$releaseDir = Resolve-Path 'build\bin\Release'
$mblinkDll = Join-Path $releaseDir 'mblink.dll'
$devtoolsDll = Join-Path $releaseDir 'mblink_devtools.dll'
$tmpRoot = Join-Path $repoRoot 'tmp\host_binding_devtools_parity'
$fixturePath = Join-Path $tmpRoot 'fixture.html'
$reportPath = Join-Path $tmpRoot 'report.json'

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Write-Utf8NoBomFile([string]$path, [string]$content) {
    $parent = Split-Path -Parent $path
    if ($parent -and !(Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    [System.IO.File]::WriteAllText($path, $content, [System.Text.UTF8Encoding]::new($false))
}

function Read-JsonFile([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing JSON file: $path"
    return Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
}

function Get-PngEvidence([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing PNG file: $path"
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::FromFile($path)
    try {
        $colors = [System.Collections.Generic.HashSet[string]]::new()
        $nonTransparent = 0
        for ($yi = 0; $yi -lt 8; ++$yi) {
            $y = [Math]::Min($bitmap.Height - 1, [int][Math]::Round(($bitmap.Height - 1) * $yi / 7))
            for ($xi = 0; $xi -lt 12; ++$xi) {
                $x = [Math]::Min($bitmap.Width - 1, [int][Math]::Round(($bitmap.Width - 1) * $xi / 11))
                $color = $bitmap.GetPixel($x, $y)
                if ($color.A -gt 0) { ++$nonTransparent }
                [void]$colors.Add("$($color.A),$($color.R),$($color.G),$($color.B)")
            }
        }
        return [ordered]@{
            width = $bitmap.Width
            height = $bitmap.Height
            nontransparent_samples = $nonTransparent
            unique_sampled_colors = $colors.Count
        }
    }
    finally {
        $bitmap.Dispose()
    }
}

function Assert-HostEvidence([string]$hostName, [string]$outDir) {
    $snapshot = Read-JsonFile (Join-Path $outDir 'snapshot.json')
    Assert ($snapshot.ok -eq $true) "$hostName snapshot ok=false"
    Assert ([int]$snapshot.node_count -ge 7) "$hostName snapshot node_count too small: $($snapshot.node_count)"
    $query = Read-JsonFile (Join-Path $outDir 'query.json')
    Assert ($query.ok -eq $true -and [int]$query.result.count -eq 1) "$hostName query failed"
    $input = Read-JsonFile (Join-Path $outDir 'input.json')
    Assert ($input.ok -eq $true -and $input.result.value -eq 'host parity input') "$hostName input_text failed"
    $click = Read-JsonFile (Join-Path $outDir 'click.json')
    Assert ($click.ok -eq $true -and $click.result.clicked -eq $true) "$hostName click failed"
    $inspect = Read-JsonFile (Join-Path $outDir 'inspect.json')
    Assert ($inspect.ok -eq $true -and $inspect.result.outer_html -like '*clicked*') "$hostName inspect did not see clicked state"
    $png = Join-Path $outDir 'screenshot.png'
    $pngInfo = Get-Item -LiteralPath $png
    Assert ($pngInfo.Length -gt 1024) "$hostName screenshot too small: $($pngInfo.Length)"
    $pngEvidence = Get-PngEvidence $png
    Assert ($pngEvidence.width -ge 100 -and $pngEvidence.height -ge 100) "$hostName screenshot dimensions too small"
    Assert ($pngEvidence.unique_sampled_colors -gt 2) "$hostName screenshot looks blank"
    return [ordered]@{
        ok = $true
        snapshot = Join-Path $outDir 'snapshot.json'
        screenshot = $png
        screenshot_bytes = $pngInfo.Length
        screenshot_render = $pngEvidence
        node_count = [int]$snapshot.node_count
    }
}

function Invoke-Checked([string]$label, [scriptblock]$body) {
    Write-Host "[RUN] $label"
    & $body
    Write-Host "[OK]  $label"
}

function Invoke-NativeCapture([string]$fileName, [string[]]$arguments, [string]$workingDirectory) {
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $fileName
    $psi.Arguments = ($arguments | ForEach-Object {
        if ($null -eq $_ -or $_.Length -eq 0) {
            '""'
        } elseif ($_ -notmatch '[\s"]') {
            $_
        } else {
            '"' + ($_ -replace '\\(?=("|$))', '\\' -replace '"', '\"') + '"'
        }
    }) -join ' '
    $psi.WorkingDirectory = $workingDirectory
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
    return [pscustomobject]@{
        exit_code = $proc.ExitCode
        stdout = $stdout
        stderr = $stderr
        combined = (($stdout, $stderr) -join "`n").Trim()
    }
}

Assert (Test-Path -LiteralPath $mblinkDll) "missing mblink.dll: $mblinkDll"
Assert (Test-Path -LiteralPath $devtoolsDll) "missing mblink_devtools.dll: $devtoolsDll"

if (Test-Path -LiteralPath $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

Write-Utf8NoBomFile $fixturePath @'
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    * { box-sizing: border-box; }
    html, body { width: 100%; height: 100%; margin: 0; overflow: hidden; font-family: Segoe UI, sans-serif; }
    body { background: #f6f8fb; color: #14213d; }
    #app { display: grid; grid-template-columns: 180px minmax(0, 1fr); gap: 12px; padding: 16px; min-height: 100vh; }
    #nav { background: #243b53; color: #fff; padding: 14px; }
    #content { display: grid; grid-template-rows: auto auto 1fr; gap: 12px; min-width: 0; }
    .toolbar { display: flex; gap: 8px; align-items: center; min-width: 0; }
    button, input { padding: 8px 10px; border: 1px solid #9fb3c8; border-radius: 4px; }
    input { min-width: 180px; }
    #status { min-height: 44px; padding: 12px; background: #d9f99d; }
    .grid { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 10px; }
    .card { min-height: 72px; padding: 10px; background: #fff; border: 1px solid #cbd5e1; }
  </style>
</head>
<body>
  <main id="app">
    <aside id="nav">Host binding parity</aside>
    <section id="content">
      <div class="toolbar">
        <input id="field" value="start">
        <button id="btn" onclick="document.getElementById('status').textContent='clicked'">Click</button>
      </div>
      <div id="status">ready</div>
      <div class="grid">
        <article class="card">grid A</article>
        <article class="card">grid B</article>
        <article class="card">grid C</article>
      </div>
    </section>
  </main>
</body>
</html>
'@

$env:Path = "$releaseDir;$env:Path"
$env:MBLINK_DLL_PATH = $releaseDir.Path
$env:MBLINK_DEVTOOLS_PATH = $devtoolsDll

$pythonOut = Join-Path $tmpRoot 'python'
$pythonScript = Join-Path $tmpRoot 'python_host.py'
Write-Utf8NoBomFile $pythonScript @'
from mblink import App
import json
import os
import sys

out_dir, fixture_path = sys.argv[1], sys.argv[2]
os.makedirs(out_dir, exist_ok=True)
with open(fixture_path, "r", encoding="utf-8") as f:
    html = f.read()

app = App("python host parity", 720, 520)
try:
    app.load_html(html)
    for _ in range(5):
        app.render_frame(1)
        app.poll()
    screenshot = os.path.abspath(os.path.join(out_dir, "screenshot.png"))
    snapshot = app.ui_dev_snapshot(include_screenshot=True, screenshot_file=screenshot)
    with open(os.path.join(out_dir, "snapshot.json"), "w", encoding="utf-8") as f:
        json.dump(snapshot, f, ensure_ascii=False, indent=2)
    commands = {
        "query": {"id": "query-field", "type": "query_element", "selector": "#field"},
        "input": {"id": "input-field", "type": "input_text", "selector": "#field", "text": "host parity input"},
        "click": {"id": "click-button", "type": "click", "selector": "#btn"},
        "inspect": {"id": "inspect-status", "type": "inspect", "selector": "#status"},
    }
    for name, payload in commands.items():
        result = app.ui_dev_command(payload)
        with open(os.path.join(out_dir, f"{name}.json"), "w", encoding="utf-8") as f:
            json.dump(result, f, ensure_ascii=False, indent=2)
    if "clicked" not in json.dumps(result, ensure_ascii=False):
        raise RuntimeError("inspect did not observe clicked state")
finally:
    app.stop()
'@

$env:PYTHONPATH = Join-Path $repoRoot 'bindings\python'
Invoke-Checked 'Python binding devtools snapshot/input/click' {
    & py -3 $pythonScript $pythonOut $fixturePath
    Assert ($LASTEXITCODE -eq 0) 'Python host parity script failed'
}
$pythonEvidence = Assert-HostEvidence 'Python' $pythonOut

$rustOut = Join-Path $tmpRoot 'rust'
$rustHost = Join-Path $tmpRoot 'rust_host'
New-Item -ItemType Directory -Force -Path (Join-Path $rustHost 'src') | Out-Null
Write-Utf8NoBomFile (Join-Path $rustHost 'Cargo.toml') @'
[package]
name = "mblink_rust_host_parity"
version = "0.1.0"
edition = "2021"

[dependencies]
mblink = { path = "../../../bindings/rust/mblink" }
serde_json = "1"
'@
Write-Utf8NoBomFile (Join-Path $rustHost 'src\main.rs') @'
use std::{env, fs, path::PathBuf};
use serde_json::json;
use mblink::{App, UiDevSnapshotOptions};

fn write_json(path: PathBuf, value: &serde_json::Value) -> mblink::Result<()> {
    fs::write(path, serde_json::to_vec_pretty(value).unwrap())
        .map_err(|err| mblink::Error::Message(err.to_string()))
}

fn main() -> mblink::Result<()> {
    let out = PathBuf::from(env::args().nth(1).expect("out dir"));
    let fixture = PathBuf::from(env::args().nth(2).expect("fixture path"));
    fs::create_dir_all(&out).map_err(|err| mblink::Error::Message(err.to_string()))?;
    let html = fs::read_to_string(fixture).map_err(|err| mblink::Error::Message(err.to_string()))?;
    let app = App::new("rust host parity", 720, 520)?;
    app.load_html(&html)?;
    for _ in 0..5 {
        app.render_frame(1)?;
        let _ = app.poll()?;
    }
    let screenshot = out.join("screenshot.png");
    let snapshot = app.ui_dev_snapshot(UiDevSnapshotOptions {
        include_screenshot: true,
        screenshot_file: Some(screenshot.to_string_lossy().to_string()),
        ..Default::default()
    })?;
    write_json(out.join("snapshot.json"), &snapshot)?;
    let query = app.ui_dev_command(&json!({"id":"query-field","type":"query_element","selector":"#field"}))?;
    write_json(out.join("query.json"), &query)?;
    let input = app.ui_dev_command(&json!({"id":"input-field","type":"input_text","selector":"#field","text":"host parity input"}))?;
    write_json(out.join("input.json"), &input)?;
    let click = app.ui_dev_command(&json!({"id":"click-button","type":"click","selector":"#btn"}))?;
    write_json(out.join("click.json"), &click)?;
    let inspect = app.ui_dev_command(&json!({"id":"inspect-status","type":"inspect","selector":"#status"}))?;
    write_json(out.join("inspect.json"), &inspect)?;
    if !inspect.to_string().contains("clicked") {
        return Err(mblink::Error::Message("inspect did not observe clicked state".to_string()));
    }
    app.stop();
    Ok(())
}
'@

$env:MBLINK_LIB_DIR = $releaseDir.Path
$env:MBLINK_DLL_PATH = $mblinkDll
Invoke-Checked 'Rust binding devtools snapshot/input/click' {
    Push-Location $rustHost
    try {
        & cargo run --quiet -- $rustOut $fixturePath
        Assert ($LASTEXITCODE -eq 0) 'Rust host parity script failed'
    }
    finally {
        Pop-Location
    }
}
$rustEvidence = Assert-HostEvidence 'Rust' $rustOut

$goEvidence = [ordered]@{ ok = $false; blocked = $false; reason = $null }
$goHost = Join-Path $tmpRoot 'go_host'
$goOut = Join-Path $tmpRoot 'go'
New-Item -ItemType Directory -Force -Path $goHost | Out-Null
Write-Utf8NoBomFile (Join-Path $goHost 'go.mod') @'
module mblink_go_host_parity

go 1.23.5

require mblink-go v0.0.0

replace mblink-go => ../../../bindings/go
'@
Write-Utf8NoBomFile (Join-Path $goHost 'main.go') @'
package main

import (
    "encoding/json"
    "os"
    "path/filepath"
    "strings"

    "mblink-go/mblink"
)

func must(err error) {
    if err != nil {
        panic(err)
    }
}

func writeJSON(path string, value any) {
    data, err := json.MarshalIndent(value, "", "  ")
    must(err)
    must(os.WriteFile(path, data, 0644))
}

func main() {
    out, fixture := os.Args[1], os.Args[2]
    must(os.MkdirAll(out, 0755))
    data, err := os.ReadFile(fixture)
    must(err)
    app, err := mblink.New("go host parity", 720, 520)
    must(err)
    defer app.Close()
    must(app.LoadHTML(string(data)))
    for i := 0; i < 5; i++ {
        must(app.RenderFrame(1))
        _, _ = app.Poll()
    }
    screenshot := filepath.Join(out, "screenshot.png")
    snapshot, err := app.UiDevSnapshot(mblink.UiDevSnapshotOptions{IncludeScreenshot: true, ScreenshotFile: screenshot})
    must(err)
    writeJSON(filepath.Join(out, "snapshot.json"), snapshot)
    query, err := app.UiDevCommand(map[string]any{"id": "query-field", "type": "query_element", "selector": "#field"})
    must(err)
    writeJSON(filepath.Join(out, "query.json"), query)
    input, err := app.UiDevCommand(map[string]any{"id": "input-field", "type": "input_text", "selector": "#field", "text": "host parity input"})
    must(err)
    writeJSON(filepath.Join(out, "input.json"), input)
    click, err := app.UiDevCommand(map[string]any{"id": "click-button", "type": "click", "selector": "#btn"})
    must(err)
    writeJSON(filepath.Join(out, "click.json"), click)
    inspect, err := app.UiDevCommand(map[string]any{"id": "inspect-status", "type": "inspect", "selector": "#status"})
    must(err)
    writeJSON(filepath.Join(out, "inspect.json"), inspect)
    encoded, _ := json.Marshal(inspect)
    if !strings.Contains(string(encoded), "clicked") {
        panic("inspect did not observe clicked state")
    }
    app.Stop()
}
'@

$oldCgo = $env:CGO_ENABLED
$env:CGO_ENABLED = '1'
try {
    Push-Location $goHost
    try {
        $goRun = Invoke-NativeCapture 'go' @('run', '.', $goOut, $fixturePath) $goHost
        $goRaw = $goRun.combined
        if ($goRun.exit_code -eq 0) {
            $goEvidence = Assert-HostEvidence 'Go' $goOut
        } elseif ($goRaw -like '*C compiler*not found*' -or $goRaw -like '*gcc*not found*') {
            $goEvidence = [ordered]@{
                ok = $false
                blocked = $true
                reason = 'Go cgo requires a Windows C compiler; gcc/MinGW was not found in PATH.'
                output = $goRaw.Trim()
            }
            if (-not $AllowGoToolchainMissing) {
                throw $goEvidence.reason
            }
            Write-Host "[BLOCKED] Go binding runtime check: $($goEvidence.reason)"
        } else {
            throw "Go host parity failed`n$goRaw"
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    $env:CGO_ENABLED = $oldCgo
}

$allGreen = ($pythonEvidence.ok -eq $true) -and ($rustEvidence.ok -eq $true) -and ($goEvidence.ok -eq $true)
$report = [ordered]@{
    ok = $allGreen -or ($AllowGoToolchainMissing -and $goEvidence.blocked -eq $true)
    all_hosts_green = $allGreen
    release_dir = $releaseDir.Path
    fixture = $fixturePath
    python = $pythonEvidence
    rust = $rustEvidence
    go = $goEvidence
}
Write-Utf8NoBomFile $reportPath ($report | ConvertTo-Json -Depth 10)

if (-not $allGreen -and -not ($AllowGoToolchainMissing -and $goEvidence.blocked -eq $true)) {
    throw "host binding devtools parity failed; see $reportPath"
}

Write-Host "[OK] host binding parity report: $reportPath"
