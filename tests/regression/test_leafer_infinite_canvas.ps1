$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$processPath = [Environment]::GetEnvironmentVariable('Path', 'Process')
if ($processPath) {
    [Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
    [Environment]::SetEnvironmentVariable('Path', $processPath, 'Process')
}

Set-Location (Join-Path $PSScriptRoot '..\..')

$repo = Get-Location
$project = Join-Path $repo 'examples/leafer_infinite_canvas'
$uiDev = Join-Path $repo 'build/bin/Release/mblink-ui-dev.exe'
$verify = Join-Path $repo 'tmp/leafer_infinite_canvas_verify'
$initialSnapshot = Join-Path $verify 'initial_snapshot.json'
$initialScreenshot = Join-Path $verify 'initial_screenshot.png'
$afterMoveSnapshot = Join-Path $verify 'after_move_snapshot.json'
$afterMoveScreenshot = Join-Path $verify 'after_move_screenshot.png'
$afterZoomScreenshot = Join-Path $verify 'after_zoom_screenshot.png'
$afterPanScreenshot = Join-Path $verify 'after_pan_screenshot.png'
$afterAddScreenshot = Join-Path $verify 'after_add_screenshot.png'
$afterTraceScreenshot = Join-Path $verify 'after_trace_screenshot.png'
$afterTopLeftScreenshot = Join-Path $verify 'after_top_left_screenshot.png'
$vendor = Join-Path $repo 'examples/leafer_ui_showcase/js/leafer-ui/web.module.min.js'
$strokeIsolationFixture = Join-Path $repo 'tests/js/leafer_top_left_stroke_isolation_fixture.js'
$strokeIsolationProject = Join-Path $repo 'tmp/leafer_top_left_stroke_isolation_project'
$strokeIsolationVerify = Join-Path $verify 'stroke_isolation'
$strokeInitialSnapshot = Join-Path $strokeIsolationVerify 'initial_snapshot.json'
$strokeInitialScreenshot = Join-Path $strokeIsolationVerify 'initial_screenshot.png'
$strokeTopLeftScreenshot = Join-Path $strokeIsolationVerify 'after_top_left_screenshot.png'
$strokeHiddenScreenshot = Join-Path $strokeIsolationVerify 'after_hide_accent_screenshot.png'

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) {
        throw $message
    }
}

function Invoke-MblinkCli([string[]]$arguments) {
    $raw = & $uiDev @arguments 2>&1 | Out-String
    $exit = $LASTEXITCODE
    Assert ($exit -eq 0) "mblink-ui-dev exited $exit for: $($arguments -join ' ')`n$raw"
    $json = $raw | ConvertFrom-Json
    Assert ($json.ok -eq $true) "mblink-ui-dev returned ok=false for: $($arguments -join ' ')`n$raw"
    return $json
}

function Stop-InfiniteCanvasRuntime() {
    try { Invoke-MblinkCli @('stop', '--project', $project) | Out-Null } catch {}
}

function Copy-EvidenceFile([object]$source, [string]$destination, [string]$label) {
    Assert ($source -and (Test-Path -LiteralPath $source)) "missing $label evidence: $source"
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

function Capture-DevScreenshot([string]$screenshotDestination, [string]$label, [string]$snapshotDestination = '') {
    $snapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    if ($snapshotDestination) {
        Copy-EvidenceFile $snapshot.snapshot.path $snapshotDestination "$label snapshot"
    }
    Copy-EvidenceFile $snapshot.screenshot.path $screenshotDestination "$label screenshot"
    return $snapshot
}

function Capture-ProjectDevScreenshot([string]$targetProject, [string]$screenshotDestination, [string]$label, [string]$snapshotDestination = '') {
    $snapshot = Invoke-MblinkCli @('snapshot', '--project', $targetProject, '--response', 'file', '--include-screenshot')
    if ($snapshotDestination) {
        Copy-EvidenceFile $snapshot.snapshot.path $snapshotDestination "$label snapshot"
    }
    Copy-EvidenceFile $snapshot.screenshot.path $screenshotDestination "$label screenshot"
    return $snapshot
}

function Assert-SelectorRect([string]$selector) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $project)
    Assert ([int]$query.result.count -eq 1) "selector count mismatch for $selector"
    $match = @($query.result.matches)[0]
    Assert ($null -ne $match.rect) "missing rect for $selector"
    Assert ([double]$match.rect.w -gt 0 -and [double]$match.rect.h -gt 0) "empty rect for $selector"
    return $match
}

function Assert-ProjectSelectorRect([string]$targetProject, [string]$selector) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $targetProject)
    Assert ([int]$query.result.count -eq 1) "selector count mismatch for $selector"
    $match = @($query.result.matches)[0]
    Assert ($null -ne $match.rect) "missing rect for $selector"
    Assert ([double]$match.rect.w -gt 0 -and [double]$match.rect.h -gt 0) "empty rect for $selector"
    return $match
}

function Assert-InspectContains([string]$selector, [string]$needle) {
    $inspect = Invoke-MblinkCli @('inspect', $selector, '--project', $project)
    Assert ($inspect.result.found -eq $true) "inspect did not find $selector"
    Assert ($inspect.result.outer_html -like "*$needle*") "inspect $selector missing '$needle': $($inspect.result.outer_html)"
}

function Assert-ProjectInspectContains([string]$targetProject, [string]$selector, [string]$needle) {
    $inspect = Invoke-MblinkCli @('inspect', $selector, '--project', $targetProject)
    Assert ($inspect.result.found -eq $true) "inspect did not find $selector"
    Assert ($inspect.result.outer_html -like "*$needle*") "inspect $selector missing '$needle': $($inspect.result.outer_html)"
}

function Assert-Click([string]$selector) {
    $click = Invoke-MblinkCli @('click', $selector, '--project', $project)
    Assert ($click.result.clicked -eq $true) "click did not report clicked=true for $selector"
    Start-Sleep -Milliseconds 140
}

function Assert-ProjectClick([string]$targetProject, [string]$selector) {
    $click = Invoke-MblinkCli @('click', $selector, '--project', $targetProject)
    Assert ($click.result.clicked -eq $true) "click did not report clicked=true for $selector"
    Start-Sleep -Milliseconds 160
}

function Assert-NoLiveErrors([string]$label) {
    $liveErrors = Invoke-MblinkCli @('errors', '--project', $project)
    Assert (@($liveErrors.errors).Count -eq 0) "$label reported JS errors: $($liveErrors | ConvertTo-Json -Compress -Depth 8)"
}

function Assert-ProjectNoLiveErrors([string]$targetProject, [string]$label) {
    $liveErrors = Invoke-MblinkCli @('errors', '--project', $targetProject)
    Assert (@($liveErrors.errors).Count -eq 0) "$label reported JS errors: $($liveErrors | ConvertTo-Json -Compress -Depth 8)"
}

function Assert-PixelNear([string]$path, [int]$x, [int]$y, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    Assert (Test-Path -LiteralPath $path) "missing PNG file: $path"
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::new($path)
    try {
        $pixel = $bitmap.GetPixel($x, $y)
        $delta = [Math]::Abs([int]$pixel.R - $r) + [Math]::Abs([int]$pixel.G - $g) + [Math]::Abs([int]$pixel.B - $b)
        Assert ($delta -le $tolerance) "$label pixel mismatch at $x,$y; expected rgb($r,$g,$b) actual rgb($($pixel.R),$($pixel.G),$($pixel.B)) delta=$delta"
    } finally {
        $bitmap.Dispose()
    }
}

function Assert-PixelNotNear([string]$path, [int]$x, [int]$y, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    Assert (Test-Path -LiteralPath $path) "missing PNG file: $path"
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::new($path)
    try {
        $pixel = $bitmap.GetPixel($x, $y)
        $delta = [Math]::Abs([int]$pixel.R - $r) + [Math]::Abs([int]$pixel.G - $g) + [Math]::Abs([int]$pixel.B - $b)
        Assert ($delta -gt $tolerance) "$label unexpected pixel at $x,$y; avoided rgb($r,$g,$b) actual rgb($($pixel.R),$($pixel.G),$($pixel.B)) delta=$delta"
    } finally {
        $bitmap.Dispose()
    }
}

function Assert-CanvasPixelNear([int]$x, [int]$y, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    $code = @"
(() => {
  const canvas = document.querySelector('#infinite-stage-frame canvas');
  if (!canvas) return { ok: false, error: 'stage canvas missing' };
  const context = canvas.getContext('2d');
  if (!context) return { ok: false, error: '2d context missing' };
  const data = context.getImageData($x, $y, 1, 1).data;
  return { ok: true, rgba: [data[0], data[1], data[2], data[3]] };
})()
"@
    $sample = Invoke-MblinkCli @('eval', $code, '--project', $project)
    Assert ($sample.result.ok -eq $true) "$label canvas sample failed: $($sample | ConvertTo-Json -Compress -Depth 8)"
    $rgba = @($sample.result.rgba)
    $delta = [Math]::Abs([int]$rgba[0] - $r) + [Math]::Abs([int]$rgba[1] - $g) + [Math]::Abs([int]$rgba[2] - $b)
    Assert ($delta -le $tolerance) "$label canvas pixel mismatch at $x,$y; expected rgb($r,$g,$b) actual rgb($($rgba[0]),$($rgba[1]),$($rgba[2])) delta=$delta"
}

function Assert-CanvasPixelNotNear([int]$x, [int]$y, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    $code = @"
(() => {
  const canvas = document.querySelector('#infinite-stage-frame canvas');
  if (!canvas) return { ok: false, error: 'stage canvas missing' };
  const context = canvas.getContext('2d');
  if (!context) return { ok: false, error: '2d context missing' };
  const data = context.getImageData($x, $y, 1, 1).data;
  return { ok: true, rgba: [data[0], data[1], data[2], data[3]] };
})()
"@
    $sample = Invoke-MblinkCli @('eval', $code, '--project', $project)
    Assert ($sample.result.ok -eq $true) "$label canvas sample failed: $($sample | ConvertTo-Json -Compress -Depth 8)"
    $rgba = @($sample.result.rgba)
    $delta = [Math]::Abs([int]$rgba[0] - $r) + [Math]::Abs([int]$rgba[1] - $g) + [Math]::Abs([int]$rgba[2] - $b)
    Assert ($delta -gt $tolerance) "$label unexpected canvas pixel at $x,$y; avoided rgb($r,$g,$b) actual rgb($($rgba[0]),$($rgba[1]),$($rgba[2])) delta=$delta"
}

function Assert-IsolationCanvasPixelNear([int]$x, [int]$y, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    $code = @"
(() => {
  const api = globalThis.__leaferTopLeftStrokeIsolation;
  if (!api || typeof api.sampleCanvasPixel !== 'function') return { ok: false, error: 'isolation API missing' };
  const rgba = api.sampleCanvasPixel($x, $y);
  return { ok: true, rgba, state: api.state() };
})()
"@
    $sample = Invoke-MblinkCli @('eval', $code, '--project', $strokeIsolationProject)
    Assert ($sample.result.ok -eq $true) "$label canvas sample failed: $($sample | ConvertTo-Json -Compress -Depth 8)"
    $rgba = @($sample.result.rgba)
    $delta = [Math]::Abs([int]$rgba[0] - $r) + [Math]::Abs([int]$rgba[1] - $g) + [Math]::Abs([int]$rgba[2] - $b)
    Assert ($delta -le $tolerance) "$label canvas pixel mismatch at $x,$y; expected rgb($r,$g,$b) actual rgb($($rgba[0]),$($rgba[1]),$($rgba[2])) delta=$delta"
}

function Assert-IsolationCanvasPixelNotNear([int]$x, [int]$y, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    $code = @"
(() => {
  const api = globalThis.__leaferTopLeftStrokeIsolation;
  if (!api || typeof api.sampleCanvasPixel !== 'function') return { ok: false, error: 'isolation API missing' };
  const rgba = api.sampleCanvasPixel($x, $y);
  return { ok: true, rgba, state: api.state() };
})()
"@
    $sample = Invoke-MblinkCli @('eval', $code, '--project', $strokeIsolationProject)
    Assert ($sample.result.ok -eq $true) "$label canvas sample failed: $($sample | ConvertTo-Json -Compress -Depth 8)"
    $rgba = @($sample.result.rgba)
    $delta = [Math]::Abs([int]$rgba[0] - $r) + [Math]::Abs([int]$rgba[1] - $g) + [Math]::Abs([int]$rgba[2] - $b)
    Assert ($delta -gt $tolerance) "$label unexpected canvas pixel at $x,$y; avoided rgb($r,$g,$b) actual rgb($($rgba[0]),$($rgba[1]),$($rgba[2])) delta=$delta"
}

function Stop-ProjectRuntime([string]$targetProject) {
    try { Invoke-MblinkCli @('stop', '--project', $targetProject) | Out-Null } catch {}
    $projectNeedle = [string]$targetProject
    $currentPid = $PID
    Get-CimInstance Win32_Process |
        Where-Object {
            ($_.Name -in @('mblink-ui-dev.exe', 'esm_loader.exe')) -and
            ($_.CommandLine -like "*$projectNeedle*") -and
            ([int]$_.ProcessId -ne [int]$currentPid)
        } |
        ForEach-Object {
            try { Stop-Process -Id $_.ProcessId -Force } catch {}
        }
    Start-Sleep -Milliseconds 250
}

function Assert-IsolationState([string]$expectedAction) {
    $code = @"
(() => {
  const status = document.querySelector('#isolation-status');
  const api = globalThis.__leaferTopLeftStrokeIsolation;
  return {
    state: api && api.state(),
    status: status && status.getAttribute('data-action')
  };
})()
"@
    $state = Invoke-MblinkCli @(
        'eval',
        $code,
        '--project',
        $strokeIsolationProject
    )
    Assert ($state.result.status -eq $expectedAction) "isolation status action mismatch: expected $expectedAction got $($state.result.status)"
    Assert ($state.result.state.bodyStroke -eq '#6f8299') "isolation body stroke changed unexpectedly: $($state.result.state.bodyStroke)"
    return $state.result.state
}

function New-StrokeIsolationProject() {
    Assert (Test-Path -LiteralPath $vendor) "missing Leafer vendor: $vendor"
    Assert (Test-Path -LiteralPath $strokeIsolationFixture) "missing stroke isolation fixture: $strokeIsolationFixture"

    Stop-ProjectRuntime $strokeIsolationProject
    if (Test-Path -LiteralPath $strokeIsolationProject) {
        Remove-Item -LiteralPath $strokeIsolationProject -Recurse -Force
    }
    if (Test-Path -LiteralPath $strokeIsolationVerify) {
        Remove-Item -LiteralPath $strokeIsolationVerify -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path (Join-Path $strokeIsolationProject 'ui/leafer-ui') | Out-Null
    New-Item -ItemType Directory -Force -Path $strokeIsolationVerify | Out-Null
    Copy-Item -LiteralPath $vendor -Destination (Join-Path $strokeIsolationProject 'ui/leafer-ui/web.module.min.js') -Force
    Copy-Item -LiteralPath $strokeIsolationFixture -Destination (Join-Path $strokeIsolationProject 'ui/app.js') -Force

    $config = @{
        name = 'leafer_top_left_stroke_isolation'
        entry = 'ui/app.js'
        src_dir = 'ui'
        out_dir = '.dist'
        runtime = 'tool'
        purpose = 'showcase'
        window = @{
            title = 'Leafer Top Left Stroke Isolation'
            width = 560
            height = 380
            resizable = $true
        }
        build = @{
            builder = 'esbuild'
            minify = $false
        }
    }
    [System.IO.File]::WriteAllText(
        (Join-Path $strokeIsolationProject 'mblink.config.json'),
        ($config | ConvertTo-Json -Depth 8),
        [System.Text.UTF8Encoding]::new($false)
    )
}

function Run-StrokeIsolationRegression() {
    Write-Host '[RUN] mblink-ui-dev Leafer top-left stroke isolation'
    New-StrokeIsolationProject

    $buildRaw = & $uiDev build --project $strokeIsolationProject 2>&1
    $buildExit = $LASTEXITCODE
    Assert ($buildExit -eq 0) "stroke isolation build failed $buildExit`n$buildRaw"
    $buildJson = $buildRaw | ConvertFrom-Json
    Assert ($buildJson.ok -eq $true) "stroke isolation build returned not ok: $($buildJson | ConvertTo-Json -Compress -Depth 8)"

    Stop-ProjectRuntime $strokeIsolationProject
    try {
        Invoke-MblinkCli @('open', '--project', $strokeIsolationProject) | Out-Null
        foreach ($selector in @(
            '#isolation-root',
            '#isolation-toolbar',
            '#isolation-stage',
            '#isolation-status',
            '#isolation-move-top-left-button',
            '#isolation-hide-accent-button',
            '#isolation-recolor-accent-button'
        )) {
            Assert-ProjectSelectorRect $strokeIsolationProject $selector | Out-Null
        }

        Assert-ProjectInspectContains $strokeIsolationProject '#isolation-status' 'ready'
        $stageMatch = Assert-ProjectSelectorRect $strokeIsolationProject '#isolation-stage'
        $stageX = [int][Math]::Round([double]$stageMatch.rect.x)
        $stageY = [int][Math]::Round([double]$stageMatch.rect.y)

        Assert-IsolationState 'ready' | Out-Null
        Assert-IsolationCanvasPixelNotNear 42 38 47 143 122 45 'isolation initial outside-node has no accent'
        Assert-IsolationCanvasPixelNear 90 49 111 130 153 85 'isolation initial body top stroke is neutral'
        Assert-IsolationCanvasPixelNear 90 60 47 143 122 45 'isolation initial accent cap'
        Capture-ProjectDevScreenshot $strokeIsolationProject $strokeInitialScreenshot 'stroke isolation initial' $strokeInitialSnapshot | Out-Null
        Assert-PixelNear $strokeInitialScreenshot ($stageX + 42) ($stageY + 38) 217 228 239 35 'stroke isolation initial screenshot background'
        Assert-PixelNear $strokeInitialScreenshot ($stageX + 90) ($stageY + 60) 47 143 122 45 'stroke isolation initial screenshot accent cap'
        Assert-ProjectNoLiveErrors $strokeIsolationProject 'stroke isolation initial render'

        Assert-ProjectClick $strokeIsolationProject '#isolation-move-top-left-button'
        Assert-ProjectInspectContains $strokeIsolationProject '#isolation-status' 'top-left'
        $state = Assert-IsolationState 'top-left'
        Assert ([int]$state.nodeX -eq -116 -and [int]$state.nodeY -eq -84) "isolation node did not move to top-left: $($state | ConvertTo-Json -Compress -Depth 8)"
        Assert ($state.capFill -eq '#2f8f7a' -and $state.connectorStroke -eq '#2f8f7a') "isolation accent sources changed unexpectedly: $($state | ConvertTo-Json -Compress -Depth 8)"
        Assert-IsolationCanvasPixelNotNear 8 8 47 143 122 45 'isolation top-left outside-node has no accent'
        Assert-IsolationCanvasPixelNear 42 13 111 130 153 85 'isolation top-left body stroke stays neutral'
        Assert-IsolationCanvasPixelNotNear 42 13 47 143 122 45 'isolation top-left body stroke is not accent'
        Assert-IsolationCanvasPixelNear 42 24 47 143 122 45 'isolation top-left accent cap'
        Capture-ProjectDevScreenshot $strokeIsolationProject $strokeTopLeftScreenshot 'stroke isolation top-left' | Out-Null
        Assert-PixelNear $strokeTopLeftScreenshot ($stageX + 8) ($stageY + 8) 217 228 239 35 'stroke isolation top-left screenshot background'
        Assert-PixelNear $strokeTopLeftScreenshot ($stageX + 42) ($stageY + 24) 47 143 122 45 'stroke isolation top-left screenshot accent cap'
        Assert-PixelNotNear $strokeTopLeftScreenshot ($stageX + 42) ($stageY + 13) 47 143 122 45 'stroke isolation top-left screenshot body stroke is not accent'
        Assert-ProjectNoLiveErrors $strokeIsolationProject 'stroke isolation top-left move'

        Assert-ProjectClick $strokeIsolationProject '#isolation-hide-accent-button'
        Assert-ProjectInspectContains $strokeIsolationProject '#isolation-status' 'accent hidden'
        $hiddenState = Assert-IsolationState 'accent hidden'
        Assert ($hiddenState.capVisible -eq $false -and $hiddenState.connectorVisible -eq $false) "isolation accent objects did not hide: $($hiddenState | ConvertTo-Json -Compress -Depth 8)"
        Assert-IsolationCanvasPixelNear 42 24 255 255 255 35 'isolation hidden cap area repaints to card fill'
        Assert-IsolationCanvasPixelNotNear 42 24 47 143 122 45 'isolation hidden cap has no stale accent pixel'
        Assert-IsolationCanvasPixelNear 42 13 111 130 153 85 'isolation hidden body stroke remains neutral'
        Capture-ProjectDevScreenshot $strokeIsolationProject $strokeHiddenScreenshot 'stroke isolation hidden accent' | Out-Null
        Assert-PixelNear $strokeHiddenScreenshot ($stageX + 42) ($stageY + 24) 255 255 255 35 'stroke isolation hidden screenshot cap repainted'
        Assert-PixelNotNear $strokeHiddenScreenshot ($stageX + 42) ($stageY + 24) 47 143 122 45 'stroke isolation hidden screenshot no stale accent'
        Assert-ProjectNoLiveErrors $strokeIsolationProject 'stroke isolation hidden accent'

        $logs = Invoke-MblinkCli @('logs', '--project', $strokeIsolationProject)
        $logText = $logs | ConvertTo-Json -Compress -Depth 8
        Assert ($logText.Contains('[leafer-top-left-stroke-isolation] ready')) 'stroke isolation logs missing ready marker'
    }
    finally {
        Stop-ProjectRuntime $strokeIsolationProject
    }
}

Assert (Test-Path -LiteralPath $project) "missing example project: $project"
Assert (Test-Path -LiteralPath $uiDev) "missing mblink-ui-dev: $uiDev"

Write-Host '[RUN] build leafer_infinite_canvas'
$buildRaw = & $uiDev build --project $project 2>&1
$buildExit = $LASTEXITCODE
Assert ($buildExit -eq 0) "mblink-ui-dev build failed $buildExit`n$buildRaw"
$buildJson = $buildRaw | ConvertFrom-Json
Assert ($buildJson.ok -eq $true) "mblink-ui-dev build returned not ok: $($buildJson | ConvertTo-Json -Compress -Depth 8)"

if (Test-Path -LiteralPath $verify) {
    Remove-Item -LiteralPath $verify -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $verify | Out-Null

Write-Host '[RUN] mblink-ui-dev leafer_infinite_canvas'
Stop-InfiniteCanvasRuntime
try {
    Invoke-MblinkCli @('open', '--project', $project) | Out-Null

    Capture-DevScreenshot $initialScreenshot 'initial' $initialSnapshot | Out-Null
    foreach ($selector in @(
        '#infinite-canvas-root',
        '#inspector-pane',
        '#canvas-workspace',
        '#canvas-toolbar',
        '#infinite-stage-frame',
        '#zoom-in-button',
        '#zoom-out-button',
        '#pan-left-button',
        '#pan-right-button',
        '#center-view-button',
        '#move-node-button',
        '#add-node-button',
        '#trace-links-button',
        '#selection-readout',
        '#view-readout',
        '#node-count-readout',
        '#edge-count-readout',
        '#render-readout',
        '#last-action-readout'
    )) {
        Assert-SelectorRect $selector | Out-Null
    }

    Assert-InspectContains '#selection-readout' 'Model'
    Assert-InspectContains '#view-readout' '100%, 480,300'
    Assert-InspectContains '#node-count-readout' '4'
    Assert-InspectContains '#edge-count-readout' '4'
    Assert-InspectContains '#last-action-readout' 'ready'
    Assert-PixelNear $initialScreenshot 350 120 217 228 239 20 'initial dev screenshot canvas background'
    Assert-CanvasPixelNear 440 142 47 143 122 65 'initial selected node accent'
    Assert-PixelNear $initialScreenshot 750 248 47 143 122 55 'initial dev screenshot model accent'
    Assert-PixelNear $initialScreenshot 825 360 130 148 168 50 'initial dev screenshot neutral connection'
    Assert-NoLiveErrors 'initial render'

    Assert-Click '#move-node-button'
    Assert-InspectContains '#selection-readout' 'Model (4,-134)'
    Assert-InspectContains '#last-action-readout' 'moved Model'
    Assert-CanvasPixelNear 530 176 47 143 122 65 'moved node immediate repaint'
    Capture-DevScreenshot $afterMoveScreenshot 'after move' $afterMoveSnapshot | Out-Null
    Assert-PixelNear $afterMoveScreenshot 830 282 47 143 122 55 'dev screenshot moved node'
    Assert-NoLiveErrors 'move node interaction'

    Assert-Click '#zoom-in-button'
    Assert-InspectContains '#view-readout' '112%, 480,300'
    Assert-InspectContains '#last-action-readout' 'zoomed in'
    Assert-CanvasPixelNear 530 161 47 143 122 65 'zoom immediate repaint'
    Capture-DevScreenshot $afterZoomScreenshot 'after zoom' | Out-Null
    Assert-NoLiveErrors 'zoom interaction'

    Assert-Click '#pan-right-button'
    Assert-InspectContains '#view-readout' '112%, 572,300'
    Assert-InspectContains '#last-action-readout' 'panned right'
    Capture-DevScreenshot $afterPanScreenshot 'after pan' | Out-Null
    Assert-PixelNear $afterPanScreenshot 930 266 47 143 122 55 'dev screenshot panned node'
    Assert-NoLiveErrors 'pan interaction'

    Assert-Click '#add-node-button'
    Assert-InspectContains '#selection-readout' 'Idea 1'
    Assert-InspectContains '#node-count-readout' '5'
    Assert-InspectContains '#edge-count-readout' '5'
    Assert-InspectContains '#last-action-readout' 'added Idea 1'
    Capture-DevScreenshot $afterAddScreenshot 'after add' | Out-Null
    Assert-NoLiveErrors 'add node interaction'

    Assert-Click '#trace-links-button'
    Assert-InspectContains '#last-action-readout' 'trace 1'
    Capture-DevScreenshot $afterTraceScreenshot 'after trace' | Out-Null
    Assert-NoLiveErrors 'trace interaction'

    Stop-InfiniteCanvasRuntime
    Invoke-MblinkCli @('open', '--project', $project) | Out-Null
    Start-Sleep -Milliseconds 300
    $dragToTopLeftCode = @"
(() => {
  const frame = document.querySelector('#infinite-stage-frame');
  const canvas = frame && frame.querySelector('canvas');
  if (!canvas) return { ok: false, error: 'stage canvas missing' };
  const rect = frame.getBoundingClientRect();
  const dispatch = (type, x, y, buttons) => canvas.dispatchEvent(new MouseEvent(type, {
    bubbles: true,
    cancelable: true,
    clientX: rect.left + x,
    clientY: rect.top + y,
    screenX: rect.left + x,
    screenY: rect.top + y,
    button: 0,
    buttons
  }));
  dispatch('mousedown', 502, 184, 1);
  for (const point of [[470, 180], [430, 176], [390, 172], [350, 168], [250, 130], [166, 82]]) {
    dispatch('mousemove', point[0], point[1], 1);
  }
  dispatch('mouseup', 166, 82, 0);
  return { ok: true };
})()
"@
    $dragToTopLeft = Invoke-MblinkCli @('eval', $dragToTopLeftCode, '--project', $project)
    Assert ($dragToTopLeft.result.ok -eq $true) "drag to top-left fixture failed: $($dragToTopLeft | ConvertTo-Json -Compress -Depth 8)"
    Start-Sleep -Milliseconds 160
    Assert-InspectContains '#selection-readout' 'Model (-410,-270)'
    Assert-CanvasPixelNotNear 72 35 47 143 122 40 'top-left selected node outline uses neutral color'
    Assert-CanvasPixelNear 95 43 47 143 122 55 'top-left model accent remains on node cap'
    Capture-DevScreenshot $afterTopLeftScreenshot 'after top-left drag' | Out-Null
    Assert-PixelNotNear $afterTopLeftScreenshot 397 139 47 143 122 40 'top-left dev screenshot outline uses neutral color'
    Assert-PixelNear $afterTopLeftScreenshot 420 147 47 143 122 55 'top-left dev screenshot model accent remains'
    Assert-NoLiveErrors 'top-left drag interaction'

    $logs = Invoke-MblinkCli @('logs', '--project', $project)
    $logText = $logs | ConvertTo-Json -Compress -Depth 8
    Assert ($logText.Contains('leafer infinite canvas ready')) 'runtime logs missing ready marker'
}
finally {
    Stop-InfiniteCanvasRuntime
}

Run-StrokeIsolationRegression

Write-Host '[PASS] leafer infinite canvas renders nodes, links, pan, zoom, add, trace links, live repaint, and top-left stroke isolation'
