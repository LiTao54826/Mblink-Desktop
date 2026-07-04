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

function Assert-SelectorRect([string]$selector) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $project)
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

function Assert-Click([string]$selector) {
    $click = Invoke-MblinkCli @('click', $selector, '--project', $project)
    Assert ($click.result.clicked -eq $true) "click did not report clicked=true for $selector"
    Start-Sleep -Milliseconds 140
}

function Assert-NoLiveErrors([string]$label) {
    $liveErrors = Invoke-MblinkCli @('errors', '--project', $project)
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
    Assert-CanvasPixelNear 425 137 32 180 134 55 'initial selected node accent'
    Assert-CanvasPixelNear 371 184 47 128 236 65 'initial model connection'
    Assert-PixelNear $initialScreenshot 740 241 32 180 134 65 'initial dev screenshot model accent'
    Assert-NoLiveErrors 'initial render'

    Assert-Click '#move-node-button'
    Assert-InspectContains '#selection-readout' 'Model (4,-134)'
    Assert-InspectContains '#last-action-readout' 'moved Model'
    Assert-CanvasPixelNear 524 171 32 180 134 65 'moved node immediate repaint'
    Capture-DevScreenshot $afterMoveScreenshot 'after move' $afterMoveSnapshot | Out-Null
    Assert-PixelNear $afterMoveScreenshot 840 275 32 180 134 70 'dev screenshot moved node'
    Assert-NoLiveErrors 'move node interaction'

    Assert-Click '#zoom-in-button'
    Assert-InspectContains '#view-readout' '112%, 480,300'
    Assert-InspectContains '#last-action-readout' 'zoomed in'
    Assert-CanvasPixelNear 530 155 32 180 134 70 'zoom immediate repaint'
    Capture-DevScreenshot $afterZoomScreenshot 'after zoom' | Out-Null
    Assert-NoLiveErrors 'zoom interaction'

    Assert-Click '#pan-right-button'
    Assert-InspectContains '#view-readout' '112%, 572,300'
    Assert-InspectContains '#last-action-readout' 'panned right'
    Capture-DevScreenshot $afterPanScreenshot 'after pan' | Out-Null
    Assert-PixelNear $afterPanScreenshot 1080 260 32 180 134 80 'dev screenshot panned node'
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

    $logs = Invoke-MblinkCli @('logs', '--project', $project)
    $logText = $logs | ConvertTo-Json -Compress -Depth 8
    Assert ($logText.Contains('leafer infinite canvas ready')) 'runtime logs missing ready marker'
}
finally {
    Stop-InfiniteCanvasRuntime
}

Write-Host '[PASS] leafer infinite canvas renders nodes, links, pan, zoom, add, trace links, and live repaint'
