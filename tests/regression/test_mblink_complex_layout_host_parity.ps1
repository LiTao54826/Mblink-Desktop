param(
    [int]$SampleSeconds = 15,
    [int]$WarmupSeconds = 2,
    [double]$MaxHostCpuPercentOneCore = 5.0,
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
$project = Resolve-Path 'examples\complex_layout_dynamics'
$appJs = Join-Path $project 'ui\app.js'
$tmpRoot = Join-Path $repoRoot 'tmp\complex_layout_host_parity'
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
    Assert (Test-Path -LiteralPath $path) "missing JSON evidence: $path"
    return Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
}

function Get-PngEvidence([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing PNG evidence: $path"
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::FromFile($path)
    try {
        $colors = [System.Collections.Generic.HashSet[string]]::new()
        $nonTransparent = 0
        $sampleCount = 0
        for ($yi = 0; $yi -lt 24; ++$yi) {
            $y = [Math]::Min($bitmap.Height - 1, [int][Math]::Round(($bitmap.Height - 1) * $yi / 23))
            for ($xi = 0; $xi -lt 32; ++$xi) {
                $x = [Math]::Min($bitmap.Width - 1, [int][Math]::Round(($bitmap.Width - 1) * $xi / 31))
                $color = $bitmap.GetPixel($x, $y)
                ++$sampleCount
                if ($color.A -gt 0) { ++$nonTransparent }
                [void]$colors.Add("$($color.A),$($color.R),$($color.G),$($color.B)")
            }
        }
        return [ordered]@{
            width = $bitmap.Width
            height = $bitmap.Height
            sampled_pixels = $sampleCount
            nontransparent_samples = $nonTransparent
            unique_sampled_colors = $colors.Count
        }
    }
    finally {
        $bitmap.Dispose()
    }
}

function Get-QueryEvidencePath([string]$hostOut, [string]$stage, [string]$name) {
    return Join-Path $hostOut ("query-{0}-{1}.json" -f $stage, $name)
}

function Get-CommandEvidencePath([string]$hostOut, [string]$name) {
    return Join-Path $hostOut ("command-{0}.json" -f $name)
}

function Get-InspectEvidencePath([string]$hostOut, [string]$name) {
    return Join-Path $hostOut ("inspect-{0}.json" -f $name)
}

function Get-QueryCount([string]$path) {
    $json = Read-JsonFile $path
    Assert ($json.ok -eq $true) "query failed: $path"
    return [int]$json.result.count
}

function Get-QueryRect([string]$path) {
    $json = Read-JsonFile $path
    Assert ($json.ok -eq $true) "query failed: $path"
    Assert ([int]$json.result.count -eq 1) "query count mismatch for $path; count=$($json.result.count)"
    $match = @($json.result.matches)[0]
    Assert ($null -ne $match.rect) "query rect missing: $path"
    $rect = $match.rect
    Assert ([double]$rect.w -gt 0 -and [double]$rect.h -gt 0) "empty query rect: $path"
    return [ordered]@{
        x = [Math]::Round([double]$rect.x, 3)
        y = [Math]::Round([double]$rect.y, 3)
        w = [Math]::Round([double]$rect.w, 3)
        h = [Math]::Round([double]$rect.h, 3)
    }
}

function Assert-CommandOk([string]$path) {
    $json = Read-JsonFile $path
    Assert ($json.ok -eq $true) "command failed: $path"
    return $json
}

function Assert-InspectContains([string]$path, [string]$needle) {
    $json = Read-JsonFile $path
    Assert ($json.ok -eq $true) "inspect failed: $path"
    Assert ($json.result.found -eq $true) "inspect did not find element: $path"
    $outer = [string]$json.result.outer_html
    Assert ($outer -like "*$needle*") "inspect missing '$needle': $path"
    return $json
}

function Compare-Rect([object]$a, [object]$b, [string]$label, [double]$tolerance = 0.75) {
    foreach ($key in @('x', 'y', 'w', 'h')) {
        $delta = [Math]::Abs([double]$a[$key] - [double]$b[$key])
        Assert ($delta -le $tolerance) "$label rect $key drifted by $delta; before=$($a | ConvertTo-Json -Compress) after=$($b | ConvertTo-Json -Compress)"
    }
}

function Assert-ShellStable([string]$hostName, [string]$hostOut, [hashtable]$baseline, [string]$stage) {
    foreach ($name in @('app_shell', 'sidebar', 'topbar', 'content', 'main_stage', 'right_rail')) {
        $path = Get-QueryEvidencePath $hostOut $stage $name
        Compare-Rect $baseline[$name] (Get-QueryRect $path) "$hostName $stage $name"
    }
}

function Assert-MenuGeometry([string]$hostName, [string]$hostOut, [string]$stage) {
    $items = @(
        @('nav_grid', '#nav-grid'),
        @('nav_board', '#nav-board'),
        @('nav_form', '#nav-form'),
        @('nav_analytics', '#nav-analytics')
    )
    $rects = [ordered]@{}
    foreach ($item in $items) {
        $name = $item[0]
        $selector = $item[1]
        $rect = Get-QueryRect (Get-QueryEvidencePath $hostOut $stage $name)
        $rects[$selector] = $rect
        Assert ([double]$rect.h -ge 34.0 -and [double]$rect.h -le 48.0) "$hostName $selector menu height abnormal: $($rect.h)"
    }
    for ($i = 1; $i -lt $items.Count; ++$i) {
        $previous = $rects[$items[$i - 1][1]]
        $current = $rects[$items[$i][1]]
        $gap = [double]$current.y - ([double]$previous.y + [double]$previous.h)
        Assert ([Math]::Abs($gap - 6.0) -le 1.0) "$hostName menu gap mismatch before $($items[$i][1]): $gap"
    }
    return $rects
}

function Assert-SnapshotEvidence([string]$hostName, [string]$hostOut, [string]$label) {
    $snapshotPath = Join-Path $hostOut "$label-snapshot.json"
    $screenshotPath = Join-Path $hostOut "$label-screenshot.png"
    $snapshot = Read-JsonFile $snapshotPath
    Assert ($snapshot.ok -eq $true) "$hostName $label snapshot ok=false"
    Assert ($snapshot.tree.tag -ne 'stub-root') "$hostName $label returned stub-root"
    if ($null -ne $snapshot.node_count) {
        Assert ([int]$snapshot.node_count -gt 30) "$hostName $label node_count too small: $($snapshot.node_count)"
    }
    $pngInfo = Get-Item -LiteralPath $screenshotPath
    Assert ($pngInfo.Length -gt 1024) "$hostName $label screenshot too small: $($pngInfo.Length)"
    $png = Get-PngEvidence $screenshotPath
    Assert ($png.width -ge 300 -and $png.height -ge 240) "$hostName $label screenshot dimensions too small"
    Assert ($png.nontransparent_samples -gt 0) "$hostName $label screenshot has no visible sampled pixels"
    Assert ($png.unique_sampled_colors -gt 6) "$hostName $label screenshot appears blank"
    return [ordered]@{
        label = $label
        snapshot = $snapshotPath
        screenshot = $screenshotPath
        snapshot_bytes = [int64](Get-Item -LiteralPath $snapshotPath).Length
        screenshot_bytes = [int64]$pngInfo.Length
        viewport = $snapshot.viewport
        render = $png
    }
}

function Assert-ObservationClean([string]$hostName, [string]$hostOut) {
    $errors = Read-JsonFile (Join-Path $hostOut 'observe-errors.json')
    $serialized = $errors | ConvertTo-Json -Depth 20 -Compress
    Assert ($serialized -notmatch '"severity"\s*:\s*"error"') "$hostName runtime reported JS errors: $serialized"
    $console = Read-JsonFile (Join-Path $hostOut 'observe-console.json')
    return [ordered]@{
        errors = Join-Path $hostOut 'observe-errors.json'
        console = Join-Path $hostOut 'observe-console.json'
        error_payload_bytes = [int64](Get-Item -LiteralPath (Join-Path $hostOut 'observe-errors.json')).Length
        console_payload_bytes = [int64](Get-Item -LiteralPath (Join-Path $hostOut 'observe-console.json')).Length
        console_payload = $console
    }
}

function Assert-HostEvidence([string]$hostName, [string]$hostOut, [object]$cpuEvidence, [object]$processOutput) {
    $screens = @()
    foreach ($label in @('01-grid-initial', '02-grid-mutated', '03-board-mutated', '04-form-drawer', '05-analytics-empty', '06-after-scroll')) {
        $screens += Assert-SnapshotEvidence $hostName $hostOut $label
    }

    $baseline = @{}
    foreach ($name in @('app_shell', 'sidebar', 'topbar', 'content', 'main_stage', 'right_rail', 'orders_table', 'orders_table_wrap')) {
        $baseline[$name] = Get-QueryRect (Get-QueryEvidencePath $hostOut 'initial' $name)
    }
    $menu = Assert-MenuGeometry $hostName $hostOut 'initial'

    $searchInput = Assert-CommandOk (Get-CommandEvidencePath $hostOut 'input-search-auth')
    Assert ($searchInput.result.value -eq 'Auth') "$hostName search input value mismatch"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'filter' 'row_mb_101')) -eq 1) "$hostName filtered row MB-101 missing"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'filter' 'row_mb_102')) -eq 0) "$hostName nonmatching row MB-102 still visible"
    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'grid-count-filter') '1 visible' | Out-Null
    Assert-ShellStable $hostName $hostOut $baseline 'after_search'

    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'clear' 'row_mb_102')) -eq 1) "$hostName row MB-102 not restored after clear"
    $compactTable = Get-QueryRect (Get-QueryEvidencePath $hostOut 'compact' 'orders_table')
    Assert ([double]$compactTable.h -lt [double]$baseline['orders_table'].h) "$hostName compact density did not reduce table height"
    Compare-Rect $baseline['orders_table_wrap'] (Get-QueryRect (Get-QueryEvidencePath $hostOut 'density_roundtrip' 'orders_table_wrap')) "$hostName table wrap density roundtrip"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'grid_mutated' 'row_mb_109')) -eq 1) "$hostName dynamic row MB-109 missing"
    Assert-ShellStable $hostName $hostOut $baseline 'after_grid_mutations'

    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'page-title-board') 'Kanban Board' | Out-Null
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'board' 'board_panel')) -eq 1) "$hostName board panel missing"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'board' 'task_1')) -eq 1) "$hostName task-1 missing"
    $boardBefore = Get-QueryRect (Get-QueryEvidencePath $hostOut 'board' 'kanban_board_before')
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'board_mutated' 'task_6')) -eq 1) "$hostName task-6 missing"
    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'lane-doing-after-advance') 'Dynamic layout card' | Out-Null
    Compare-Rect $boardBefore (Get-QueryRect (Get-QueryEvidencePath $hostOut 'board_mutated' 'kanban_board_after')) "$hostName kanban board after add/move"
    Assert-ShellStable $hostName $hostOut $baseline 'after_board'

    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'page-title-form') 'Form Studio' | Out-Null
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'form' 'form_panel')) -eq 1) "$hostName form panel missing"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'form_mutated' 'detail_drawer')) -eq 1) "$hostName drawer did not open"
    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'form-result') 'Runtime Layout Proof saved as incident / high.' | Out-Null
    Assert-ShellStable $hostName $hostOut $baseline 'after_form'

    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'page-title-analytics') 'Analytics' | Out-Null
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'analytics' 'analytics_panel')) -eq 1) "$hostName analytics panel missing"
    $chartBefore = Get-QueryRect (Get-QueryEvidencePath $hostOut 'analytics' 'chart_before')
    Assert-InspectContains (Get-InspectEvidencePath $hostOut 'analytics-state-preview') 'High churn data window' | Out-Null
    Compare-Rect $chartBefore (Get-QueryRect (Get-QueryEvidencePath $hostOut 'analytics_mutated' 'chart_after')) "$hostName chart geometry after stress"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'analytics_empty' 'analytics_empty_state')) -eq 1) "$hostName analytics empty state missing"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'analytics_empty' 'chart')) -eq 0) "$hostName chart still present in empty state"
    Assert-ShellStable $hostName $hostOut $baseline 'after_analytics'

    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'modal_open' 'settings_dialog')) -eq 1) "$hostName settings dialog did not open"
    Assert ((Get-QueryCount (Get-QueryEvidencePath $hostOut 'modal_closed' 'settings_dialog')) -eq 0) "$hostName settings dialog did not close"
    Assert-ShellStable $hostName $hostOut $baseline 'after_modal_scroll'

    $observations = Assert-ObservationClean $hostName $hostOut
    Assert ([double]$cpuEvidence.cpu_percent_one_core -le $MaxHostCpuPercentOneCore) "$hostName idle CPU too high: $($cpuEvidence.cpu_percent_one_core)%"

    return [ordered]@{
        ok = $true
        output_dir = $hostOut
        screenshots = $screens
        menu_geometry = $menu
        baseline_rects = $baseline
        cpu = $cpuEvidence
        observations = $observations
        stdout = $processOutput.stdout
        stderr = $processOutput.stderr
    }
}

function Wait-ForReadyFile([System.Diagnostics.Process]$proc, [string]$readyPath, [int]$timeoutMs) {
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    while ($timer.ElapsedMilliseconds -lt $timeoutMs) {
        if (Test-Path -LiteralPath $readyPath) { return }
        if ($proc.HasExited) { throw "host process exited before ready file: $readyPath" }
        Start-Sleep -Milliseconds 100
    }
    throw "timed out waiting for ready file: $readyPath"
}

function ConvertTo-NativeArgument([string]$argument) {
    if ($null -eq $argument) { return '""' }
    if ($argument.Length -eq 0) { return '""' }
    if ($argument -notmatch '[\s"]') { return $argument }

    $result = '"'
    $backslashes = 0
    foreach ($ch in $argument.ToCharArray()) {
        if ($ch -eq '\') {
            ++$backslashes
        } elseif ($ch -eq '"') {
            if ($backslashes -gt 0) {
                $result += ('\' * ($backslashes * 2))
                $backslashes = 0
            }
            $result += '\"'
        } else {
            if ($backslashes -gt 0) {
                $result += ('\' * $backslashes)
                $backslashes = 0
            }
            $result += $ch
        }
    }
    if ($backslashes -gt 0) {
        $result += ('\' * ($backslashes * 2))
    }
    $result += '"'
    return $result
}

function Join-NativeArguments([string[]]$arguments) {
    return (($arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' ')
}

function Invoke-CheckedProcess(
    [string]$label,
    [string]$fileName,
    [string[]]$arguments,
    [string]$workingDirectory,
    [hashtable]$environment
) {
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $fileName
    $psi.Arguments = Join-NativeArguments $arguments
    $psi.WorkingDirectory = $workingDirectory
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    foreach ($key in $environment.Keys) {
        $psi.Environment[$key] = [string]$environment[$key]
    }

    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
    if ($proc.ExitCode -ne 0) {
        throw "$label exited $($proc.ExitCode)`nSTDOUT:`n$stdout`nSTDERR:`n$stderr"
    }
    return [ordered]@{
        stdout = $stdout
        stderr = $stderr
    }
}

function Invoke-HostProcessWithCpuSample(
    [string]$hostName,
    [string]$fileName,
    [string[]]$arguments,
    [string]$workingDirectory,
    [hashtable]$environment,
    [string]$hostOut
) {
    $readyPath = Join-Path $hostOut 'ready.json'
    $continuePath = Join-Path $hostOut 'continue.signal'

    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $fileName
    $psi.Arguments = Join-NativeArguments $arguments
    $psi.WorkingDirectory = $workingDirectory
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    foreach ($key in $environment.Keys) {
        $psi.Environment[$key] = [string]$environment[$key]
    }

    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    try {
        Wait-ForReadyFile $proc $readyPath 180000
        Start-Sleep -Seconds $WarmupSeconds
        $before = Get-Process -Id $proc.Id -ErrorAction Stop
        $startedAt = Get-Date
        Start-Sleep -Seconds $SampleSeconds
        $elapsed = ((Get-Date) - $startedAt).TotalSeconds
        $after = Get-Process -Id $proc.Id -ErrorAction Stop
        $delta = [double]($after.CPU - $before.CPU)
        $cpuEvidence = [ordered]@{
            pid = [int]$proc.Id
            sample_seconds = $SampleSeconds
            warmup_seconds = $WarmupSeconds
            elapsed_seconds = [Math]::Round($elapsed, 3)
            cpu_seconds_delta = [Math]::Round($delta, 4)
            cpu_percent_one_core = [Math]::Round((100.0 * $delta / $elapsed), 3)
            max_cpu_percent_one_core = $MaxHostCpuPercentOneCore
        }
        Write-Utf8NoBomFile $continuePath 'continue'
        if (-not $proc.WaitForExit(90000)) {
            try { $proc.Kill() } catch {}
            throw "$hostName process did not exit after CPU sample"
        }
        $stdout = $proc.StandardOutput.ReadToEnd()
        $stderr = $proc.StandardError.ReadToEnd()
        if ($proc.ExitCode -ne 0) {
            throw "$hostName process exited $($proc.ExitCode)`nSTDOUT:`n$stdout`nSTDERR:`n$stderr"
        }
        return [ordered]@{
            cpu = $cpuEvidence
            stdout = $stdout
            stderr = $stderr
        }
    }
    catch {
        if (-not $proc.HasExited) {
            try { $proc.Kill() } catch {}
        }
        $stdout = ''
        $stderr = ''
        try { $stdout = $proc.StandardOutput.ReadToEnd() } catch {}
        try { $stderr = $proc.StandardError.ReadToEnd() } catch {}
        throw "$hostName process failed: $($_.Exception.Message)`nSTDOUT:`n$stdout`nSTDERR:`n$stderr"
    }
}

function Get-W64DevkitBin {
    foreach ($candidate in @(
        "$env:USERPROFILE\.codex\toolchains\w64devkit-1.23.0-root\w64devkit\bin",
        "$env:USERPROFILE\.codex\toolchains\w64devkit-2.3.0-root\w64devkit\bin",
        "$env:USERPROFILE\.codex\toolchains\w64devkit\bin"
    )) {
        if (Test-Path -LiteralPath (Join-Path $candidate 'gcc.exe')) {
            return $candidate
        }
    }
    $found = Get-Command gcc -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { return Split-Path -Parent $found.Source }
    return $null
}

$sharedHostRunnerPython = @'
import json
import os
import sys
import time
from pathlib import Path

from mblink import App


OUT_DIR = Path(sys.argv[1]).resolve()
APP_JS = Path(sys.argv[2]).resolve()
OUT_DIR.mkdir(parents=True, exist_ok=True)


def write_json(name, value):
    path = OUT_DIR / name
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def flush(app, frames=5):
    for _ in range(frames):
        app.render_frame(1)
        app.poll()
        time.sleep(0.01)


def snapshot(app, label):
    screenshot = OUT_DIR / f"{label}-screenshot.png"
    snap = app.ui_dev_snapshot(
        include_screenshot=True,
        screenshot_file=str(screenshot),
        max_nodes=4000,
        max_depth=80,
    )
    write_json(f"{label}-snapshot.json", snap)


def command(app, name, payload):
    result = app.ui_dev_command(payload)
    write_json(f"command-{name}.json", result)
    flush(app, 3)
    return result


def query(app, stage, name, selector):
    result = app.ui_dev_command({
        "id": f"query-{stage}-{name}",
        "type": "query_element",
        "selector": selector,
    })
    write_json(f"query-{stage}-{name}.json", result)
    return result


def inspect(app, name, selector):
    result = app.ui_dev_command({
        "id": f"inspect-{name}",
        "type": "inspect",
        "selector": selector,
    })
    write_json(f"inspect-{name}.json", result)
    return result


SHELL_SELECTORS = [
    ("app_shell", "#app-shell"),
    ("sidebar", "#sidebar"),
    ("topbar", "#topbar"),
    ("content", "#content"),
    ("main_stage", "#main-stage"),
    ("right_rail", "#right-rail"),
]

INITIAL_SELECTORS = SHELL_SELECTORS + [
    ("metric_grid", "#metric-grid"),
    ("grid_panel", "#grid-panel"),
    ("orders_table", "#orders-table"),
    ("orders_table_wrap", "#orders-table-wrap"),
    ("global_search", "#global-search"),
    ("activity_feed", "#activity-feed"),
    ("nav_grid", "#nav-grid"),
    ("nav_board", "#nav-board"),
    ("nav_form", "#nav-form"),
    ("nav_analytics", "#nav-analytics"),
]


def query_set(app, stage, selectors):
    flush(app, 3)
    for name, selector in selectors:
        query(app, stage, name, selector)


def run():
    app = App("Complex Layout Dynamics Python Host", 1240, 820)
    try:
        app.load_html("""<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <style>
    html, body, #root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    * { box-sizing: border-box; }
  </style>
</head>
<body><div id="root"></div></body>
</html>""")
        app.load_js_file(str(APP_JS))
        flush(app, 12)

        query_set(app, "initial", INITIAL_SELECTORS)
        snapshot(app, "01-grid-initial")

        command(app, "input-search-auth", {"id": "input-search-auth", "type": "input_text", "selector": "#global-search", "text": "Auth"})
        query(app, "filter", "row_mb_101", "#row-mb-101")
        query(app, "filter", "row_mb_102", "#row-mb-102")
        inspect(app, "grid-count-filter", "#grid-count")
        query_set(app, "after_search", SHELL_SELECTORS)

        command(app, "clear-search", {"id": "clear-search", "type": "click", "selector": "#clear-search"})
        query(app, "clear", "row_mb_102", "#row-mb-102")
        command(app, "density-compact", {"id": "density-compact", "type": "click", "selector": "#density-compact"})
        query(app, "compact", "orders_table", "#orders-table")
        command(app, "density-comfortable", {"id": "density-comfortable", "type": "click", "selector": "#density-comfortable"})
        query(app, "density_roundtrip", "orders_table_wrap", "#orders-table-wrap")
        command(app, "add-row", {"id": "add-row", "type": "click", "selector": "#add-row"})
        query(app, "grid_mutated", "row_mb_109", "#row-mb-109")
        query_set(app, "after_grid_mutations", SHELL_SELECTORS)
        snapshot(app, "02-grid-mutated")

        command(app, "nav-board", {"id": "nav-board", "type": "click", "selector": "#nav-board"})
        inspect(app, "page-title-board", "#page-title")
        query(app, "board", "board_panel", "#board-panel")
        query(app, "board", "task_1", "#task-1")
        query(app, "board", "kanban_board_before", "#kanban-board")
        command(app, "add-board-card", {"id": "add-board-card", "type": "click", "selector": "#add-board-card"})
        query(app, "board_mutated", "task_6", "#task-6")
        command(app, "advance-task-6", {"id": "advance-task-6", "type": "click", "selector": "#advance-task-6"})
        inspect(app, "lane-doing-after-advance", "#lane-doing")
        query(app, "board_mutated", "kanban_board_after", "#kanban-board")
        query_set(app, "after_board", SHELL_SELECTORS)
        snapshot(app, "03-board-mutated")

        command(app, "nav-form", {"id": "nav-form", "type": "click", "selector": "#nav-form"})
        inspect(app, "page-title-form", "#page-title")
        query(app, "form", "form_panel", "#form-panel")
        command(app, "request-name", {"id": "request-name", "type": "input_text", "selector": "#request-name", "text": "Runtime Layout Proof"})
        command(app, "request-type", {"id": "request-type", "type": "input_text", "selector": "#request-type", "text": "incident"})
        command(app, "request-priority", {"id": "request-priority", "type": "input_text", "selector": "#request-priority", "text": "high"})
        command(app, "request-notes", {"id": "request-notes", "type": "input_text", "selector": "#request-notes", "text": "line one\nline two"})
        command(app, "toggle-drawer", {"id": "toggle-drawer", "type": "click", "selector": "#toggle-drawer"})
        query(app, "form_mutated", "detail_drawer", "#detail-drawer")
        command(app, "drawer-action", {"id": "drawer-action", "type": "click", "selector": "#drawer-action"})
        command(app, "request-submit", {"id": "request-submit", "type": "click", "selector": "#request-submit"})
        inspect(app, "form-result", "#form-result")
        query_set(app, "after_form", SHELL_SELECTORS)
        snapshot(app, "04-form-drawer")

        command(app, "nav-analytics", {"id": "nav-analytics", "type": "click", "selector": "#nav-analytics"})
        inspect(app, "page-title-analytics", "#page-title")
        query(app, "analytics", "analytics_panel", "#analytics-panel")
        query(app, "analytics", "chart_before", "#chart")
        command(app, "analytics-stress", {"id": "analytics-stress", "type": "click", "selector": "#analytics-stress"})
        inspect(app, "analytics-state-preview", "#analytics-state-preview")
        query(app, "analytics_mutated", "chart_after", "#chart")
        command(app, "analytics-empty", {"id": "analytics-empty", "type": "click", "selector": "#analytics-empty"})
        query(app, "analytics_empty", "analytics_empty_state", "#analytics-empty-state")
        query(app, "analytics_empty", "chart", "#chart")
        query_set(app, "after_analytics", SHELL_SELECTORS)
        snapshot(app, "05-analytics-empty")

        command(app, "open-settings", {"id": "open-settings", "type": "click", "selector": "#open-settings"})
        query(app, "modal_open", "settings_dialog", "#settings-dialog")
        command(app, "settings-apply", {"id": "settings-apply", "type": "click", "selector": "#settings-apply"})
        query(app, "modal_closed", "settings_dialog", "#settings-dialog")
        command(app, "scroll-activity", {"id": "scroll-activity", "type": "scroll", "selector": "#activity-feed", "y": 120})
        command(app, "scroll-main", {"id": "scroll-main", "type": "scroll", "selector": "#main-stage", "y": 160})
        query_set(app, "after_modal_scroll", SHELL_SELECTORS)
        snapshot(app, "06-after-scroll")

        write_json("observe-console.json", app.observe_console())
        write_json("observe-errors.json", app.observe_errors())
        write_json("ready.json", {"ok": True, "pid": os.getpid(), "host": "python"})
        continue_path = OUT_DIR / "continue.signal"
        while not continue_path.exists():
            app.poll()
            time.sleep(0.02)
    finally:
        try:
            app.stop()
        except Exception:
            pass


if __name__ == "__main__":
    run()
'@

$sharedHostRunnerRust = @'
use std::{env, fs, path::{Path, PathBuf}, thread, time::Duration};

use mblink::{App, UiDevSnapshotOptions};
use serde_json::{json, Value};

fn write_json(path: &Path, value: &Value) -> mblink::Result<()> {
    let bytes = serde_json::to_vec_pretty(value).unwrap();
    fs::write(path, bytes).map_err(|err| mblink::Error::Message(err.to_string()))
}

fn flush(app: &App, frames: usize) -> mblink::Result<()> {
    for _ in 0..frames {
        app.render_frame(1)?;
        let _ = app.poll()?;
        thread::sleep(Duration::from_millis(10));
    }
    Ok(())
}

fn snapshot(app: &App, out: &Path, label: &str) -> mblink::Result<()> {
    let screenshot = out.join(format!("{label}-screenshot.png"));
    let snapshot = app.ui_dev_snapshot(UiDevSnapshotOptions {
        max_nodes: 4000,
        max_depth: 80,
        include_screenshot: true,
        screenshot_file: Some(screenshot.to_string_lossy().to_string()),
        ..Default::default()
    })?;
    write_json(&out.join(format!("{label}-snapshot.json")), &snapshot)
}

fn command(app: &App, out: &Path, name: &str, payload: Value) -> mblink::Result<Value> {
    let result = app.ui_dev_command(&payload)?;
    write_json(&out.join(format!("command-{name}.json")), &result)?;
    flush(app, 3)?;
    Ok(result)
}

fn query(app: &App, out: &Path, stage: &str, name: &str, selector: &str) -> mblink::Result<Value> {
    let result = app.ui_dev_command(&json!({
        "id": format!("query-{stage}-{name}"),
        "type": "query_element",
        "selector": selector,
    }))?;
    write_json(&out.join(format!("query-{stage}-{name}.json")), &result)?;
    Ok(result)
}

fn inspect(app: &App, out: &Path, name: &str, selector: &str) -> mblink::Result<Value> {
    let result = app.ui_dev_command(&json!({
        "id": format!("inspect-{name}"),
        "type": "inspect",
        "selector": selector,
    }))?;
    write_json(&out.join(format!("inspect-{name}.json")), &result)?;
    Ok(result)
}

const SHELL_SELECTORS: &[(&str, &str)] = &[
    ("app_shell", "#app-shell"),
    ("sidebar", "#sidebar"),
    ("topbar", "#topbar"),
    ("content", "#content"),
    ("main_stage", "#main-stage"),
    ("right_rail", "#right-rail"),
];

const INITIAL_EXTRA_SELECTORS: &[(&str, &str)] = &[
    ("metric_grid", "#metric-grid"),
    ("grid_panel", "#grid-panel"),
    ("orders_table", "#orders-table"),
    ("orders_table_wrap", "#orders-table-wrap"),
    ("global_search", "#global-search"),
    ("activity_feed", "#activity-feed"),
    ("nav_grid", "#nav-grid"),
    ("nav_board", "#nav-board"),
    ("nav_form", "#nav-form"),
    ("nav_analytics", "#nav-analytics"),
];

fn query_set(app: &App, out: &Path, stage: &str, selectors: &[(&str, &str)]) -> mblink::Result<()> {
    flush(app, 3)?;
    for (name, selector) in selectors {
        query(app, out, stage, name, selector)?;
    }
    Ok(())
}

fn main() -> mblink::Result<()> {
    let out = PathBuf::from(env::args().nth(1).expect("out dir"));
    let app_js = PathBuf::from(env::args().nth(2).expect("app js"));
    fs::create_dir_all(&out).map_err(|err| mblink::Error::Message(err.to_string()))?;

    let app = App::new("Complex Layout Dynamics Rust Host", 1240, 820)?;
    app.load_html(r#"<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <style>
    html, body, #root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    * { box-sizing: border-box; }
  </style>
</head>
<body><div id="root"></div></body>
</html>"#)?;
    app.load_js_file(&app_js.to_string_lossy())?;
    flush(&app, 12)?;

    query_set(&app, &out, "initial", SHELL_SELECTORS)?;
    query_set(&app, &out, "initial", INITIAL_EXTRA_SELECTORS)?;
    snapshot(&app, &out, "01-grid-initial")?;

    command(&app, &out, "input-search-auth", json!({"id":"input-search-auth","type":"input_text","selector":"#global-search","text":"Auth"}))?;
    query(&app, &out, "filter", "row_mb_101", "#row-mb-101")?;
    query(&app, &out, "filter", "row_mb_102", "#row-mb-102")?;
    inspect(&app, &out, "grid-count-filter", "#grid-count")?;
    query_set(&app, &out, "after_search", SHELL_SELECTORS)?;

    command(&app, &out, "clear-search", json!({"id":"clear-search","type":"click","selector":"#clear-search"}))?;
    query(&app, &out, "clear", "row_mb_102", "#row-mb-102")?;
    command(&app, &out, "density-compact", json!({"id":"density-compact","type":"click","selector":"#density-compact"}))?;
    query(&app, &out, "compact", "orders_table", "#orders-table")?;
    command(&app, &out, "density-comfortable", json!({"id":"density-comfortable","type":"click","selector":"#density-comfortable"}))?;
    query(&app, &out, "density_roundtrip", "orders_table_wrap", "#orders-table-wrap")?;
    command(&app, &out, "add-row", json!({"id":"add-row","type":"click","selector":"#add-row"}))?;
    query(&app, &out, "grid_mutated", "row_mb_109", "#row-mb-109")?;
    query_set(&app, &out, "after_grid_mutations", SHELL_SELECTORS)?;
    snapshot(&app, &out, "02-grid-mutated")?;

    command(&app, &out, "nav-board", json!({"id":"nav-board","type":"click","selector":"#nav-board"}))?;
    inspect(&app, &out, "page-title-board", "#page-title")?;
    query(&app, &out, "board", "board_panel", "#board-panel")?;
    query(&app, &out, "board", "task_1", "#task-1")?;
    query(&app, &out, "board", "kanban_board_before", "#kanban-board")?;
    command(&app, &out, "add-board-card", json!({"id":"add-board-card","type":"click","selector":"#add-board-card"}))?;
    query(&app, &out, "board_mutated", "task_6", "#task-6")?;
    command(&app, &out, "advance-task-6", json!({"id":"advance-task-6","type":"click","selector":"#advance-task-6"}))?;
    inspect(&app, &out, "lane-doing-after-advance", "#lane-doing")?;
    query(&app, &out, "board_mutated", "kanban_board_after", "#kanban-board")?;
    query_set(&app, &out, "after_board", SHELL_SELECTORS)?;
    snapshot(&app, &out, "03-board-mutated")?;

    command(&app, &out, "nav-form", json!({"id":"nav-form","type":"click","selector":"#nav-form"}))?;
    inspect(&app, &out, "page-title-form", "#page-title")?;
    query(&app, &out, "form", "form_panel", "#form-panel")?;
    command(&app, &out, "request-name", json!({"id":"request-name","type":"input_text","selector":"#request-name","text":"Runtime Layout Proof"}))?;
    command(&app, &out, "request-type", json!({"id":"request-type","type":"input_text","selector":"#request-type","text":"incident"}))?;
    command(&app, &out, "request-priority", json!({"id":"request-priority","type":"input_text","selector":"#request-priority","text":"high"}))?;
    command(&app, &out, "request-notes", json!({"id":"request-notes","type":"input_text","selector":"#request-notes","text":"line one\nline two"}))?;
    command(&app, &out, "toggle-drawer", json!({"id":"toggle-drawer","type":"click","selector":"#toggle-drawer"}))?;
    query(&app, &out, "form_mutated", "detail_drawer", "#detail-drawer")?;
    command(&app, &out, "drawer-action", json!({"id":"drawer-action","type":"click","selector":"#drawer-action"}))?;
    command(&app, &out, "request-submit", json!({"id":"request-submit","type":"click","selector":"#request-submit"}))?;
    inspect(&app, &out, "form-result", "#form-result")?;
    query_set(&app, &out, "after_form", SHELL_SELECTORS)?;
    snapshot(&app, &out, "04-form-drawer")?;

    command(&app, &out, "nav-analytics", json!({"id":"nav-analytics","type":"click","selector":"#nav-analytics"}))?;
    inspect(&app, &out, "page-title-analytics", "#page-title")?;
    query(&app, &out, "analytics", "analytics_panel", "#analytics-panel")?;
    query(&app, &out, "analytics", "chart_before", "#chart")?;
    command(&app, &out, "analytics-stress", json!({"id":"analytics-stress","type":"click","selector":"#analytics-stress"}))?;
    inspect(&app, &out, "analytics-state-preview", "#analytics-state-preview")?;
    query(&app, &out, "analytics_mutated", "chart_after", "#chart")?;
    command(&app, &out, "analytics-empty", json!({"id":"analytics-empty","type":"click","selector":"#analytics-empty"}))?;
    query(&app, &out, "analytics_empty", "analytics_empty_state", "#analytics-empty-state")?;
    query(&app, &out, "analytics_empty", "chart", "#chart")?;
    query_set(&app, &out, "after_analytics", SHELL_SELECTORS)?;
    snapshot(&app, &out, "05-analytics-empty")?;

    command(&app, &out, "open-settings", json!({"id":"open-settings","type":"click","selector":"#open-settings"}))?;
    query(&app, &out, "modal_open", "settings_dialog", "#settings-dialog")?;
    command(&app, &out, "settings-apply", json!({"id":"settings-apply","type":"click","selector":"#settings-apply"}))?;
    query(&app, &out, "modal_closed", "settings_dialog", "#settings-dialog")?;
    command(&app, &out, "scroll-activity", json!({"id":"scroll-activity","type":"scroll","selector":"#activity-feed","y":120}))?;
    command(&app, &out, "scroll-main", json!({"id":"scroll-main","type":"scroll","selector":"#main-stage","y":160}))?;
    query_set(&app, &out, "after_modal_scroll", SHELL_SELECTORS)?;
    snapshot(&app, &out, "06-after-scroll")?;

    write_json(&out.join("observe-console.json"), &app.observe_console()?)?;
    write_json(&out.join("observe-errors.json"), &app.observe_errors()?)?;
    write_json(&out.join("ready.json"), &json!({"ok": true, "pid": std::process::id(), "host": "rust"}))?;
    let continue_path = out.join("continue.signal");
    while !continue_path.exists() {
        let _ = app.poll()?;
        thread::sleep(Duration::from_millis(20));
    }
    app.stop();
    Ok(())
}
'@

$sharedHostRunnerGo = @'
//go:build windows

package main

import (
    "encoding/json"
    "fmt"
    "os"
    "path/filepath"
    "time"

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

func flush(app *mblink.App, frames int) {
    for i := 0; i < frames; i++ {
        must(app.RenderFrame(1))
        _, _ = app.Poll()
        time.Sleep(10 * time.Millisecond)
    }
}

func snapshot(app *mblink.App, out, label string) {
    screenshot := filepath.Join(out, fmt.Sprintf("%s-screenshot.png", label))
    snap, err := app.UiDevSnapshot(mblink.UiDevSnapshotOptions{
        MaxNodes: 4000,
        MaxDepth: 80,
        IncludeScreenshot: true,
        ScreenshotFile: screenshot,
    })
    must(err)
    writeJSON(filepath.Join(out, fmt.Sprintf("%s-snapshot.json", label)), snap)
}

func command(app *mblink.App, out, name string, payload map[string]any) any {
    result, err := app.UiDevCommand(payload)
    must(err)
    writeJSON(filepath.Join(out, fmt.Sprintf("command-%s.json", name)), result)
    flush(app, 3)
    return result
}

func query(app *mblink.App, out, stage, name, selector string) any {
    result, err := app.UiDevCommand(map[string]any{
        "id": fmt.Sprintf("query-%s-%s", stage, name),
        "type": "query_element",
        "selector": selector,
    })
    must(err)
    writeJSON(filepath.Join(out, fmt.Sprintf("query-%s-%s.json", stage, name)), result)
    return result
}

func inspect(app *mblink.App, out, name, selector string) any {
    result, err := app.UiDevCommand(map[string]any{
        "id": fmt.Sprintf("inspect-%s", name),
        "type": "inspect",
        "selector": selector,
    })
    must(err)
    writeJSON(filepath.Join(out, fmt.Sprintf("inspect-%s.json", name)), result)
    return result
}

var shellSelectors = [][2]string{
    {"app_shell", "#app-shell"},
    {"sidebar", "#sidebar"},
    {"topbar", "#topbar"},
    {"content", "#content"},
    {"main_stage", "#main-stage"},
    {"right_rail", "#right-rail"},
}

var initialExtraSelectors = [][2]string{
    {"metric_grid", "#metric-grid"},
    {"grid_panel", "#grid-panel"},
    {"orders_table", "#orders-table"},
    {"orders_table_wrap", "#orders-table-wrap"},
    {"global_search", "#global-search"},
    {"activity_feed", "#activity-feed"},
    {"nav_grid", "#nav-grid"},
    {"nav_board", "#nav-board"},
    {"nav_form", "#nav-form"},
    {"nav_analytics", "#nav-analytics"},
}

func querySet(app *mblink.App, out, stage string, selectors [][2]string) {
    flush(app, 3)
    for _, item := range selectors {
        query(app, out, stage, item[0], item[1])
    }
}

func main() {
    out, appJS := os.Args[1], os.Args[2]
    must(os.MkdirAll(out, 0755))

    app, err := mblink.New("Complex Layout Dynamics Go Host", 1240, 820)
    must(err)
    defer app.Close()
    must(app.LoadHTML(`<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <style>
    html, body, #root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    * { box-sizing: border-box; }
  </style>
</head>
<body><div id="root"></div></body>
</html>`))
    must(app.LoadJSFile(appJS))
    flush(app, 12)

    querySet(app, out, "initial", shellSelectors)
    querySet(app, out, "initial", initialExtraSelectors)
    snapshot(app, out, "01-grid-initial")

    command(app, out, "input-search-auth", map[string]any{"id": "input-search-auth", "type": "input_text", "selector": "#global-search", "text": "Auth"})
    query(app, out, "filter", "row_mb_101", "#row-mb-101")
    query(app, out, "filter", "row_mb_102", "#row-mb-102")
    inspect(app, out, "grid-count-filter", "#grid-count")
    querySet(app, out, "after_search", shellSelectors)

    command(app, out, "clear-search", map[string]any{"id": "clear-search", "type": "click", "selector": "#clear-search"})
    query(app, out, "clear", "row_mb_102", "#row-mb-102")
    command(app, out, "density-compact", map[string]any{"id": "density-compact", "type": "click", "selector": "#density-compact"})
    query(app, out, "compact", "orders_table", "#orders-table")
    command(app, out, "density-comfortable", map[string]any{"id": "density-comfortable", "type": "click", "selector": "#density-comfortable"})
    query(app, out, "density_roundtrip", "orders_table_wrap", "#orders-table-wrap")
    command(app, out, "add-row", map[string]any{"id": "add-row", "type": "click", "selector": "#add-row"})
    query(app, out, "grid_mutated", "row_mb_109", "#row-mb-109")
    querySet(app, out, "after_grid_mutations", shellSelectors)
    snapshot(app, out, "02-grid-mutated")

    command(app, out, "nav-board", map[string]any{"id": "nav-board", "type": "click", "selector": "#nav-board"})
    inspect(app, out, "page-title-board", "#page-title")
    query(app, out, "board", "board_panel", "#board-panel")
    query(app, out, "board", "task_1", "#task-1")
    query(app, out, "board", "kanban_board_before", "#kanban-board")
    command(app, out, "add-board-card", map[string]any{"id": "add-board-card", "type": "click", "selector": "#add-board-card"})
    query(app, out, "board_mutated", "task_6", "#task-6")
    command(app, out, "advance-task-6", map[string]any{"id": "advance-task-6", "type": "click", "selector": "#advance-task-6"})
    inspect(app, out, "lane-doing-after-advance", "#lane-doing")
    query(app, out, "board_mutated", "kanban_board_after", "#kanban-board")
    querySet(app, out, "after_board", shellSelectors)
    snapshot(app, out, "03-board-mutated")

    command(app, out, "nav-form", map[string]any{"id": "nav-form", "type": "click", "selector": "#nav-form"})
    inspect(app, out, "page-title-form", "#page-title")
    query(app, out, "form", "form_panel", "#form-panel")
    command(app, out, "request-name", map[string]any{"id": "request-name", "type": "input_text", "selector": "#request-name", "text": "Runtime Layout Proof"})
    command(app, out, "request-type", map[string]any{"id": "request-type", "type": "input_text", "selector": "#request-type", "text": "incident"})
    command(app, out, "request-priority", map[string]any{"id": "request-priority", "type": "input_text", "selector": "#request-priority", "text": "high"})
    command(app, out, "request-notes", map[string]any{"id": "request-notes", "type": "input_text", "selector": "#request-notes", "text": "line one\nline two"})
    command(app, out, "toggle-drawer", map[string]any{"id": "toggle-drawer", "type": "click", "selector": "#toggle-drawer"})
    query(app, out, "form_mutated", "detail_drawer", "#detail-drawer")
    command(app, out, "drawer-action", map[string]any{"id": "drawer-action", "type": "click", "selector": "#drawer-action"})
    command(app, out, "request-submit", map[string]any{"id": "request-submit", "type": "click", "selector": "#request-submit"})
    inspect(app, out, "form-result", "#form-result")
    querySet(app, out, "after_form", shellSelectors)
    snapshot(app, out, "04-form-drawer")

    command(app, out, "nav-analytics", map[string]any{"id": "nav-analytics", "type": "click", "selector": "#nav-analytics"})
    inspect(app, out, "page-title-analytics", "#page-title")
    query(app, out, "analytics", "analytics_panel", "#analytics-panel")
    query(app, out, "analytics", "chart_before", "#chart")
    command(app, out, "analytics-stress", map[string]any{"id": "analytics-stress", "type": "click", "selector": "#analytics-stress"})
    inspect(app, out, "analytics-state-preview", "#analytics-state-preview")
    query(app, out, "analytics_mutated", "chart_after", "#chart")
    command(app, out, "analytics-empty", map[string]any{"id": "analytics-empty", "type": "click", "selector": "#analytics-empty"})
    query(app, out, "analytics_empty", "analytics_empty_state", "#analytics-empty-state")
    query(app, out, "analytics_empty", "chart", "#chart")
    querySet(app, out, "after_analytics", shellSelectors)
    snapshot(app, out, "05-analytics-empty")

    command(app, out, "open-settings", map[string]any{"id": "open-settings", "type": "click", "selector": "#open-settings"})
    query(app, out, "modal_open", "settings_dialog", "#settings-dialog")
    command(app, out, "settings-apply", map[string]any{"id": "settings-apply", "type": "click", "selector": "#settings-apply"})
    query(app, out, "modal_closed", "settings_dialog", "#settings-dialog")
    command(app, out, "scroll-activity", map[string]any{"id": "scroll-activity", "type": "scroll", "selector": "#activity-feed", "y": 120})
    command(app, out, "scroll-main", map[string]any{"id": "scroll-main", "type": "scroll", "selector": "#main-stage", "y": 160})
    querySet(app, out, "after_modal_scroll", shellSelectors)
    snapshot(app, out, "06-after-scroll")

    console, err := app.ObserveConsole()
    must(err)
    errors, err := app.ObserveErrors()
    must(err)
    writeJSON(filepath.Join(out, "observe-console.json"), console)
    writeJSON(filepath.Join(out, "observe-errors.json"), errors)
    writeJSON(filepath.Join(out, "ready.json"), map[string]any{"ok": true, "pid": os.Getpid(), "host": "go"})
    continuePath := filepath.Join(out, "continue.signal")
    for {
        if _, err := os.Stat(continuePath); err == nil {
            break
        }
        _, _ = app.Poll()
        time.Sleep(20 * time.Millisecond)
    }
    app.Stop()
}
'@

Assert (Test-Path -LiteralPath $mblinkDll) "missing mblink.dll: $mblinkDll"
Assert (Test-Path -LiteralPath $devtoolsDll) "missing mblink_devtools.dll: $devtoolsDll"
Assert (Test-Path -LiteralPath $appJs) "missing complex layout app.js: $appJs"

if (Test-Path -LiteralPath $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

$envCommon = @{
    Path = "$releaseDir;$env:Path"
    MBLINK_DEVTOOLS_PATH = $devtoolsDll
}

$pythonHost = Join-Path $tmpRoot 'python_host.py'
Write-Utf8NoBomFile $pythonHost $sharedHostRunnerPython

$rustHost = Join-Path $tmpRoot 'rust_host'
New-Item -ItemType Directory -Force -Path (Join-Path $rustHost 'src') | Out-Null
Write-Utf8NoBomFile (Join-Path $rustHost 'Cargo.toml') @'
[package]
name = "mblink_complex_layout_rust_host_parity"
version = "0.1.0"
edition = "2021"

[dependencies]
mblink = { path = "../../../bindings/rust/mblink" }
serde_json = "1"
'@
Write-Utf8NoBomFile (Join-Path $rustHost 'src\main.rs') $sharedHostRunnerRust

$goHost = Join-Path $tmpRoot 'go_host'
New-Item -ItemType Directory -Force -Path $goHost | Out-Null
Write-Utf8NoBomFile (Join-Path $goHost 'go.mod') @'
module mblink_complex_layout_go_host_parity

go 1.23.5

require mblink-go v0.0.0

replace mblink-go => ../../../bindings/go
'@
Write-Utf8NoBomFile (Join-Path $goHost 'main.go') $sharedHostRunnerGo

$results = [ordered]@{}
$allGreen = $true

Write-Host '[RUN] Python complex layout host parity'
$pythonOut = Join-Path $tmpRoot 'python'
New-Item -ItemType Directory -Force -Path $pythonOut | Out-Null
$pythonEnv = $envCommon.Clone()
$pythonEnv['PYTHONPATH'] = Join-Path $repoRoot 'bindings\python'
$pythonEnv['MBLINK_DLL_PATH'] = $releaseDir.Path
$pythonProcess = Invoke-HostProcessWithCpuSample 'Python' 'py' @('-3', $pythonHost, $pythonOut, $appJs) $repoRoot.Path $pythonEnv $pythonOut
$results['python'] = Assert-HostEvidence 'Python' $pythonOut $pythonProcess.cpu $pythonProcess
Write-Host '[OK]  Python complex layout host parity'

Write-Host '[RUN] Rust complex layout host parity'
$rustOut = Join-Path $tmpRoot 'rust'
New-Item -ItemType Directory -Force -Path $rustOut | Out-Null
$rustEnv = $envCommon.Clone()
$rustEnv['MBLINK_LIB_DIR'] = $releaseDir.Path
$rustEnv['MBLINK_DLL_PATH'] = $mblinkDll
$rustProcess = Invoke-HostProcessWithCpuSample 'Rust' 'cargo' @('run', '--quiet', '--', $rustOut, $appJs) $rustHost $rustEnv $rustOut
$results['rust'] = Assert-HostEvidence 'Rust' $rustOut $rustProcess.cpu $rustProcess
Write-Host '[OK]  Rust complex layout host parity'

Write-Host '[RUN] Go complex layout host parity'
$goOut = Join-Path $tmpRoot 'go'
New-Item -ItemType Directory -Force -Path $goOut | Out-Null
$goEnv = $envCommon.Clone()
$goEnv['MBLINK_DLL_PATH'] = $mblinkDll
$goEnv['CGO_ENABLED'] = '1'
$w64devkitBin = Get-W64DevkitBin
if ($w64devkitBin) {
    $goEnv['Path'] = "$w64devkitBin;$($goEnv['Path'])"
    $goEnv['CC'] = Join-Path $w64devkitBin 'gcc.exe'
}

try {
    $goExe = Join-Path $goOut 'mblink_complex_layout_go_host_parity.exe'
    $goBuild = Invoke-CheckedProcess 'Go build' 'go' @('build', '-o', $goExe, '.') $goHost $goEnv
    $goProcess = Invoke-HostProcessWithCpuSample 'Go' $goExe @($goOut, $appJs) $goHost $goEnv $goOut
    $goEvidence = Assert-HostEvidence 'Go' $goOut $goProcess.cpu $goProcess
    $goEvidence['build_stdout'] = $goBuild.stdout
    $goEvidence['build_stderr'] = $goBuild.stderr
    $goEvidence['executable'] = $goExe
    $results['go'] = $goEvidence
    Write-Host '[OK]  Go complex layout host parity'
}
catch {
    $allGreen = $false
    $reason = $_.Exception.Message
    $isToolchainMissing = ($reason -match 'gcc|C compiler|cgo|executable file not found|cannot find -lmblink|ld returned')
    $results['go'] = [ordered]@{
        ok = $false
        blocked = $isToolchainMissing
        output_dir = $goOut
        reason = $reason
        cgo_enabled = $goEnv['CGO_ENABLED']
        cc = if ($goEnv.ContainsKey('CC')) { $goEnv['CC'] } else { $null }
        w64devkit_bin = $w64devkitBin
    }
    if (-not ($AllowGoToolchainMissing -and $isToolchainMissing)) {
        throw
    }
    Write-Host "[BLOCKED] Go complex layout host parity: $reason"
}

$report = [ordered]@{
    ok = $allGreen -or ($AllowGoToolchainMissing -and $results['go'].blocked -eq $true)
    all_hosts_green = $allGreen
    project = $project.Path
    app_js = $appJs
    release_dir = $releaseDir.Path
    sample_seconds = $SampleSeconds
    warmup_seconds = $WarmupSeconds
    max_host_cpu_percent_one_core = $MaxHostCpuPercentOneCore
    results = $results
}
Write-Utf8NoBomFile $reportPath ($report | ConvertTo-Json -Depth 20)

if (-not $allGreen -and -not ($AllowGoToolchainMissing -and $results['go'].blocked -eq $true)) {
    throw "complex layout host parity failed; see $reportPath"
}

Write-Host "[OK] complex layout Python/Go/Rust host parity report: $reportPath"
