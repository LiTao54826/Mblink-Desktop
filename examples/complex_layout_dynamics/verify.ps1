param(
    [int]$SampleSeconds = 20,
    [int]$WarmupSeconds = 3,
    [double]$MaxRuntimeCpuPercentOneCore = 5.0,
    [double]$MaxDaemonCpuPercentOneCore = 2.0
)

$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$project = Resolve-Path $PSScriptRoot
$exe = Join-Path $repoRoot 'build\bin\Release\mblink-ui-dev.exe'
$tmpRoot = Join-Path $repoRoot 'tmp\complex_layout_dynamics'
$evidencePath = Join-Path $tmpRoot 'evidence.json'

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

function Invoke-MblinkCli([string[]]$arguments) {
    $raw = & $exe @arguments 2>&1 | Out-String
    $exit = $LASTEXITCODE
    Assert ($exit -eq 0) "mblink-ui-dev exited $exit for: $($arguments -join ' ')`n$raw"
    Assert ($raw.Trim().Length -gt 0) "mblink-ui-dev returned empty output for: $($arguments -join ' ')"
    $json = $raw | ConvertFrom-Json
    Assert ($json.ok -eq $true) "mblink-ui-dev returned ok=false for: $($arguments -join ' ')`n$raw"
    return $json
}

function Get-PngEvidence([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing PNG file: $path"
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

function Capture-EvidenceSnapshot([string]$label) {
    $snapshot = Invoke-MblinkCli @('snapshot', '--project', $project.Path, '--response', 'file', '--include-screenshot')
    Assert ($snapshot.response_mode -eq 'file') "$label snapshot did not use file response mode"
    Assert ($snapshot.snapshot.path -and (Test-Path -LiteralPath $snapshot.snapshot.path)) "$label snapshot JSON missing"
    Assert ($snapshot.screenshot.path -and (Test-Path -LiteralPath $snapshot.screenshot.path)) "$label screenshot PNG missing"

    $snapshotCopy = Join-Path $tmpRoot "$label-snapshot.json"
    $screenshotCopy = Join-Path $tmpRoot "$label-screenshot.png"
    Copy-Item -LiteralPath $snapshot.snapshot.path -Destination $snapshotCopy -Force
    Copy-Item -LiteralPath $snapshot.screenshot.path -Destination $screenshotCopy -Force

    $snapshotJson = Get-Content -Raw -LiteralPath $snapshotCopy | ConvertFrom-Json
    Assert ($snapshotJson.ok -eq $true) "$label snapshot JSON ok=false"
    Assert ($snapshotJson.tree.tag -ne 'stub-root') "$label snapshot returned stub-root"
    if ($null -ne $snapshotJson.node_count) {
        Assert ([int]$snapshotJson.node_count -gt 30) "$label snapshot node_count too small: $($snapshotJson.node_count)"
    }
    $png = Get-PngEvidence $screenshotCopy
    Assert ($png.width -ge 300 -and $png.height -ge 240) "$label screenshot dimensions too small"
    Assert ($png.nontransparent_samples -gt 0) "$label screenshot has no visible sampled pixels"
    Assert ($png.unique_sampled_colors -gt 6) "$label screenshot appears blank"

    return [ordered]@{
        label = $label
        snapshot = $snapshotCopy
        screenshot = $screenshotCopy
        snapshot_bytes = [int64]$snapshot.snapshot.bytes
        screenshot_bytes = [int64](Get-Item -LiteralPath $screenshotCopy).Length
        viewport = $snapshot.viewport
        render = $png
    }
}

function Get-Rect([string]$selector) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $project.Path)
    Assert ([int]$query.result.count -eq 1) "selector count mismatch for $selector; count=$($query.result.count)"
    $match = @($query.result.matches)[0]
    Assert ($null -ne $match.rect) "missing rect for $selector"
    $rect = $match.rect
    Assert ([double]$rect.w -gt 0 -and [double]$rect.h -gt 0) "empty rect for $selector"
    return [ordered]@{
        x = [Math]::Round([double]$rect.x, 3)
        y = [Math]::Round([double]$rect.y, 3)
        w = [Math]::Round([double]$rect.w, 3)
        h = [Math]::Round([double]$rect.h, 3)
    }
}

function Assert-QueryCount([string]$selector, [int]$expected, [string]$label) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $project.Path)
    Assert ([int]$query.result.count -eq $expected) "$label selector count mismatch for $selector; expected=$expected actual=$($query.result.count)"
    return $query
}

function Assert-InspectContains([string]$selector, [string]$needle, [string]$label) {
    $inspect = Invoke-MblinkCli @('inspect', $selector, '--project', $project.Path)
    Assert ($inspect.result.found -eq $true) "$label inspect did not find $selector"
    Assert ($inspect.result.outer_html -like "*$needle*") "$label missing text '$needle' in $selector"
    return $inspect
}

function Compare-Rect([object]$a, [object]$b, [string]$label, [double]$tolerance = 0.75) {
    foreach ($key in @('x', 'y', 'w', 'h')) {
        $delta = [Math]::Abs([double]$a[$key] - [double]$b[$key])
        Assert ($delta -le $tolerance) "$label rect $key drifted by $delta; before=$($a | ConvertTo-Json -Compress) after=$($b | ConvertTo-Json -Compress)"
    }
}

function Compare-StableShell([object]$baseline, [string]$label) {
    foreach ($selector in @('#app-shell', '#sidebar', '#topbar', '#content', '#main-stage', '#right-rail')) {
        Compare-Rect $baseline[$selector] (Get-Rect $selector) "$label $selector"
    }
}

function Assert-NormalMenuGeometry([string]$label) {
    $selectors = @('#nav-grid', '#nav-board', '#nav-form', '#nav-analytics')
    $rects = [ordered]@{}
    foreach ($selector in $selectors) {
        $rect = Get-Rect $selector
        $rects[$selector] = $rect
        Assert ([double]$rect.h -ge 34.0 -and [double]$rect.h -le 48.0) "$label $selector menu height is abnormal: $($rect.h)"
    }

    for ($index = 1; $index -lt $selectors.Count; ++$index) {
        $previous = $rects[$selectors[$index - 1]]
        $current = $rects[$selectors[$index]]
        $gap = [double]$current.y - ([double]$previous.y + [double]$previous.h)
        Assert ([Math]::Abs($gap - 6.0) -le 1.0) "$label menu gap mismatch between $($selectors[$index - 1]) and $($selectors[$index]): $gap"
    }

    return $rects
}

function Measure-IdleCpu([int]$daemonPid, [int]$runtimePid) {
    $ids = @($daemonPid, $runtimePid) | Where-Object { $_ -gt 0 }
    Assert ($ids.Count -ge 2) "CPU sampling requires daemon and runtime process ids; daemon=$daemonPid runtime=$runtimePid"
    Start-Sleep -Seconds $WarmupSeconds
    $before = Get-Process -Id $ids -ErrorAction Stop | Select-Object Id, ProcessName, CPU
    $startedAt = Get-Date
    Start-Sleep -Seconds $SampleSeconds
    $elapsed = ((Get-Date) - $startedAt).TotalSeconds
    $after = Get-Process -Id $ids -ErrorAction Stop | Select-Object Id, ProcessName, CPU

    $processes = @()
    foreach ($proc in $after) {
        $startProc = $before | Where-Object { $_.Id -eq $proc.Id } | Select-Object -First 1
        if ($null -eq $startProc) { continue }
        $delta = [double]($proc.CPU - $startProc.CPU)
        $role = if ($proc.Id -eq $daemonPid) { 'daemon' } elseif ($proc.Id -eq $runtimePid) { 'runtime' } else { 'unknown' }
        $processes += [ordered]@{
            id = [int]$proc.Id
            role = $role
            process_name = $proc.ProcessName
            cpu_seconds_delta = [Math]::Round($delta, 4)
            cpu_percent_one_core = [Math]::Round((100.0 * $delta / $elapsed), 3)
        }
    }

    $runtime = $processes | Where-Object { $_['role'] -eq 'runtime' } | Select-Object -First 1
    $daemon = $processes | Where-Object { $_['role'] -eq 'daemon' } | Select-Object -First 1
    Assert ($null -ne $runtime) 'runtime process sample missing'
    Assert ($null -ne $daemon) 'daemon process sample missing'
    Assert ([double]$runtime['cpu_percent_one_core'] -le $MaxRuntimeCpuPercentOneCore) "runtime idle CPU too high: $($runtime['cpu_percent_one_core'])%"
    Assert ([double]$daemon['cpu_percent_one_core'] -le $MaxDaemonCpuPercentOneCore) "daemon idle CPU too high: $($daemon['cpu_percent_one_core'])%"

    return [ordered]@{
        sample_seconds = $SampleSeconds
        warmup_seconds = $WarmupSeconds
        elapsed_seconds = [Math]::Round($elapsed, 3)
        max_runtime_cpu_percent_one_core = $MaxRuntimeCpuPercentOneCore
        max_daemon_cpu_percent_one_core = $MaxDaemonCpuPercentOneCore
        processes = $processes
    }
}

Assert (Test-Path -LiteralPath $exe) "missing mblink-ui-dev: $exe"
if (Test-Path -LiteralPath $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

$timer = [System.Diagnostics.Stopwatch]::StartNew()
$snapshots = @()
$layoutRects = [ordered]@{}
$checks = [System.Collections.Generic.List[string]]::new()
$cpu = $null
$failure = $null

function Capture-FailureDiagnostics([string]$message) {
    $diagnostics = [ordered]@{
        ok = $false
        message = $message
        project = $project.Path
        exe = $exe
        timestamp = (Get-Date).ToString('o')
        snapshots = $snapshots
        layout_rects = $layoutRects
        checks = $checks
        info = $null
        logs = $null
        errors = $null
        processes = @()
    }
    try {
        $rawInfo = & $exe @('info', '--project', $project.Path) 2>&1 | Out-String
        $diagnostics.info = $rawInfo | ConvertFrom-Json
    } catch {
        $diagnostics.info = [ordered]@{ failed = $_.Exception.Message }
    }
    try {
        $rawLogs = & $exe @('logs', '--project', $project.Path) 2>&1 | Out-String
        $diagnostics.logs = $rawLogs | ConvertFrom-Json
    } catch {
        $diagnostics.logs = [ordered]@{ failed = $_.Exception.Message }
    }
    try {
        $rawErrors = & $exe @('errors', '--project', $project.Path) 2>&1 | Out-String
        $diagnostics.errors = $rawErrors | ConvertFrom-Json
    } catch {
        $diagnostics.errors = [ordered]@{ failed = $_.Exception.Message }
    }
    try {
        $diagnostics.processes = @(Get-Process -Name 'esm_loader','mblink-ui-dev' -ErrorAction SilentlyContinue |
            Select-Object Id, ProcessName, MainWindowTitle, CPU, StartTime)
    } catch {}

    $path = Join-Path $tmpRoot 'failure-diagnostics.json'
    Write-Utf8NoBomFile $path ($diagnostics | ConvertTo-Json -Depth 12)
    return $diagnostics
}

try {
    Invoke-MblinkCli @('stop', '--project', $project.Path) | Out-Null
    $open = Invoke-MblinkCli @('open', '--project', $project.Path)
    $info = Invoke-MblinkCli @('info', '--project', $project.Path)
    Assert ($info.daemon.runtime_status -eq 'running') "runtime is not running: $($info.daemon.runtime_status)"

    $snapshots += Capture-EvidenceSnapshot '01-grid-initial'
    foreach ($selector in @(
        '#app-shell',
        '#sidebar',
        '#topbar',
        '#content',
        '#main-stage',
        '#right-rail',
        '#metric-grid',
        '#grid-panel',
        '#orders-table-wrap',
        '#orders-table',
        '#global-search',
        '#activity-feed',
        '#nav-grid',
        '#nav-board',
        '#nav-form',
        '#nav-analytics'
    )) {
        $layoutRects[$selector] = Get-Rect $selector
    }
    $layoutRects['menu_geometry'] = Assert-NormalMenuGeometry 'initial nav'
    for ($sample = 1; $sample -le 2; ++$sample) {
        Compare-StableShell $layoutRects "repeated stable sample $sample"
    }
    [void]$checks.Add('initial snapshot, screenshot, stable shell rects, and normal menu item geometry')

    $search = Invoke-MblinkCli @('input-text', '#global-search', '--text', 'Auth', '--project', $project.Path)
    Assert ($search.result.value -eq 'Auth') 'global search input value mismatch'
    Assert-QueryCount '#row-mb-101' 1 'filtered Auth row' | Out-Null
    Assert-QueryCount '#row-mb-102' 0 'filtered nonmatching row' | Out-Null
    Assert-InspectContains '#grid-count' '1 visible' 'grid count after filter' | Out-Null
    Compare-StableShell $layoutRects 'after search filter'

    Invoke-MblinkCli @('click', '#clear-search', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#row-mb-102' 1 'row restored after clear' | Out-Null
    Invoke-MblinkCli @('click', '#density-compact', '--project', $project.Path) | Out-Null
    $compactTable = Get-Rect '#orders-table'
    Assert ([double]$compactTable.h -lt [double]$layoutRects['#orders-table'].h) 'compact density did not reduce table height'
    Invoke-MblinkCli @('click', '#density-comfortable', '--project', $project.Path) | Out-Null
    Compare-Rect $layoutRects['#orders-table-wrap'] (Get-Rect '#orders-table-wrap') 'table wrap after density roundtrip'
    Invoke-MblinkCli @('click', '#add-row', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#row-mb-109' 1 'dynamic row insertion' | Out-Null
    Compare-StableShell $layoutRects 'after grid dynamic mutations'
    $snapshots += Capture-EvidenceSnapshot '02-grid-mutated'
    [void]$checks.Add('grid filter, density roundtrip, dynamic row insertion, and stable shell')

    Invoke-MblinkCli @('click', '#nav-board', '--project', $project.Path) | Out-Null
    Assert-InspectContains '#page-title' 'Kanban Board' 'board navigation title' | Out-Null
    Assert-QueryCount '#board-panel' 1 'board panel' | Out-Null
    Assert-QueryCount '#task-1' 1 'initial board card' | Out-Null
    $boardRect = Get-Rect '#kanban-board'
    Invoke-MblinkCli @('click', '#add-board-card', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#task-6' 1 'dynamic board card' | Out-Null
    Invoke-MblinkCli @('click', '#advance-task-6', '--project', $project.Path) | Out-Null
    Assert-InspectContains '#lane-doing' 'Dynamic layout card' 'advanced board card lane' | Out-Null
    Compare-Rect $boardRect (Get-Rect '#kanban-board') 'kanban board after add/move'
    Compare-StableShell $layoutRects 'after board mutations'
    $snapshots += Capture-EvidenceSnapshot '03-board-mutated'
    [void]$checks.Add('kanban card insertion, lane move, and stable board geometry')

    Invoke-MblinkCli @('click', '#nav-form', '--project', $project.Path) | Out-Null
    Assert-InspectContains '#page-title' 'Form Studio' 'form navigation title' | Out-Null
    Assert-QueryCount '#form-panel' 1 'form panel' | Out-Null
    Invoke-MblinkCli @('input-text', '#request-name', '--text', 'Runtime Layout Proof', '--project', $project.Path) | Out-Null
    Invoke-MblinkCli @('input-text', '#request-type', '--text', 'incident', '--project', $project.Path) | Out-Null
    Invoke-MblinkCli @('input-text', '#request-priority', '--text', 'high', '--project', $project.Path) | Out-Null
    Invoke-MblinkCli @('input-text', '#request-notes', '--text', "line one`nline two", '--project', $project.Path) | Out-Null
    Invoke-MblinkCli @('click', '#toggle-drawer', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#detail-drawer' 1 'drawer opened' | Out-Null
    Invoke-MblinkCli @('click', '#drawer-action', '--project', $project.Path) | Out-Null
    Invoke-MblinkCli @('click', '#request-submit', '--project', $project.Path) | Out-Null
    Assert-InspectContains '#form-result' 'Runtime Layout Proof saved as incident / high.' 'form submit result' | Out-Null
    Compare-StableShell $layoutRects 'after form mutations'
    $snapshots += Capture-EvidenceSnapshot '04-form-drawer'
    [void]$checks.Add('form input/select/textarea/checkbox path and drawer toggle')

    Invoke-MblinkCli @('click', '#nav-analytics', '--project', $project.Path) | Out-Null
    Assert-InspectContains '#page-title' 'Analytics' 'analytics navigation title' | Out-Null
    Assert-QueryCount '#analytics-panel' 1 'analytics panel' | Out-Null
    $chartRect = Get-Rect '#chart'
    Invoke-MblinkCli @('click', '#analytics-stress', '--project', $project.Path) | Out-Null
    Assert-InspectContains '#analytics-state-preview' 'High churn data window' 'stress chart state' | Out-Null
    Compare-Rect $chartRect (Get-Rect '#chart') 'chart geometry after stress data'
    Invoke-MblinkCli @('click', '#analytics-empty', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#analytics-empty-state' 1 'analytics empty state' | Out-Null
    Assert-QueryCount '#chart' 0 'chart removed in empty state' | Out-Null
    Compare-StableShell $layoutRects 'after analytics state switches'
    $snapshots += Capture-EvidenceSnapshot '05-analytics-empty'
    [void]$checks.Add('analytics chart state switch and empty-state replacement')

    Invoke-MblinkCli @('click', '#open-settings', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#settings-dialog' 1 'settings modal open' | Out-Null
    Invoke-MblinkCli @('click', '#settings-apply', '--project', $project.Path) | Out-Null
    Assert-QueryCount '#settings-dialog' 0 'settings modal closed' | Out-Null
    Invoke-MblinkCli @('scroll', '#activity-feed', '--y', '120', '--project', $project.Path) | Out-Null
    Invoke-MblinkCli @('scroll', '#main-stage', '--y', '160', '--project', $project.Path) | Out-Null
    Compare-StableShell $layoutRects 'after modal and scroll'
    $snapshots += Capture-EvidenceSnapshot '06-after-scroll'
    [void]$checks.Add('modal open/apply, nested scroll containers, and final screenshot')

    $logsResult = Invoke-MblinkCli @('logs', '--project', $project.Path)
    Assert ($logsResult.ok -eq $true) 'logs command failed'
    $errors = Invoke-MblinkCli @('errors', '--project', $project.Path)
    Assert (@($errors.errors).Count -eq 0) 'runtime JS errors were reported'
    [void]$checks.Add('logs command ok and runtime JS errors empty')

    $infoForCpu = Invoke-MblinkCli @('info', '--project', $project.Path)
    $cpu = Measure-IdleCpu ([int]$infoForCpu.daemon.pid) ([int]$infoForCpu.daemon.runtime_pid)
    [void]$checks.Add('idle CPU sampled under configured thresholds')
}
catch {
    $failure = Capture-FailureDiagnostics $_.Exception.Message
    throw
}
finally {
    try { Invoke-MblinkCli @('stop', '--project', $project.Path) | Out-Null } catch {}
}

$timer.Stop()
$evidence = [ordered]@{
    ok = $true
    elapsed_ms = [int]$timer.Elapsed.TotalMilliseconds
    project = $project.Path
    exe = $exe
    open_runtime_status = if ($null -ne $open) { $open.runtime_status } else { $null }
    sample_seconds = $SampleSeconds
    snapshots = $snapshots
    layout_rects = $layoutRects
    cpu = $cpu
    checks = $checks
}
Write-Utf8NoBomFile $evidencePath ($evidence | ConvertTo-Json -Depth 12)
Write-Host "[OK] complex layout dynamics evidence: $evidencePath"
foreach ($shot in $snapshots) {
    Write-Host "[OK] screenshot: $($shot.screenshot)"
}
