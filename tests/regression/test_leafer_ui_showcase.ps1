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
$project = Join-Path $repo 'examples/leafer_ui_showcase'
$uiDev = Join-Path $repo 'build/bin/Release/mblink-ui-dev.exe'
$esmLoader = Join-Path $repo 'build/bin/Release/esm_loader.exe'
$verify = Join-Path $repo 'tmp/leafer_ui_showcase_verify'
$loaderSnapshot = Join-Path $verify 'esm_loader_snapshot.json'
$loaderScreenshot = Join-Path $verify 'esm_loader_snapshot.png'
$console = Join-Path $verify 'console.json'
$errors = Join-Path $verify 'errors.json'
$lifecycle = Join-Path $verify 'lifecycle.json'
$liveInitialSnapshot = Join-Path $verify 'mblink_ui_dev_initial_snapshot.json'
$liveInitialScreenshot = Join-Path $verify 'mblink_ui_dev_initial_snapshot.png'
$liveInspectSnapshot = Join-Path $verify 'mblink_ui_dev_inspect_snapshot.json'
$liveInspectScreenshot = Join-Path $verify 'mblink_ui_dev_inspect_snapshot.png'
$liveAfterSnapshot = Join-Path $verify 'mblink_ui_dev_after_clicks_snapshot.json'
$liveAfterScreenshot = Join-Path $verify 'mblink_ui_dev_after_clicks_snapshot.png'
$devTapScreenshot = Join-Path $verify 'mblink_ui_dev_tap_snapshot.png'
$devDragScreenshot = Join-Path $verify 'mblink_ui_dev_drag_snapshot.png'
$devReorderScreenshot = Join-Path $verify 'mblink_ui_dev_reorder_snapshot.png'
$devRafScreenshot = Join-Path $verify 'mblink_ui_dev_raf_snapshot.png'
$devAnimationScreenshot = Join-Path $verify 'mblink_ui_dev_animation_snapshot.png'
$devVisibleScreenshot = Join-Path $verify 'mblink_ui_dev_visible_snapshot.png'
$devRemovedScreenshot = Join-Path $verify 'mblink_ui_dev_removed_snapshot.png'

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) {
        throw $message
    }
}

function Read-JsonFile([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing json file: $path"
    return Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
}

function Invoke-MblinkCli([string[]]$arguments) {
    $raw = & $uiDev @arguments 2>&1 | Out-String
    $exit = $LASTEXITCODE
    Assert ($exit -eq 0) "mblink-ui-dev exited $exit for: $($arguments -join ' ')`n$raw"
    $json = $raw | ConvertFrom-Json
    Assert ($json.ok -eq $true) "mblink-ui-dev returned ok=false for: $($arguments -join ' ')`n$raw"
    return $json
}

function Stop-ShowcaseRuntime() {
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

function Wait-InspectContains([string]$selector, [string]$needle, [int]$timeoutMs = 3000) {
    $deadline = (Get-Date).AddMilliseconds($timeoutMs)
    $lastHtml = ''
    do {
        $inspect = Invoke-MblinkCli @('inspect', $selector, '--project', $project)
        Assert ($inspect.result.found -eq $true) "inspect did not find $selector"
        $lastHtml = [string]$inspect.result.outer_html
        if ($lastHtml -like "*$needle*") {
            return
        }
        Start-Sleep -Milliseconds 120
    } while ((Get-Date) -lt $deadline)

    throw "inspect $selector missing '$needle' after ${timeoutMs}ms: $lastHtml"
}

function Assert-ActiveClass([string]$selector, [bool]$expected) {
    $inspect = Invoke-MblinkCli @('inspect', $selector, '--project', $project)
    Assert ($inspect.result.found -eq $true) "inspect did not find $selector"
    $classes = @([string]$inspect.result.element.class_name -split ' ' | Where-Object { $_ })
    $hasActive = $classes -contains 'active'
    Assert ($hasActive -eq $expected) "active class mismatch for $selector; expected=$expected actual=$hasActive class=$($inspect.result.element.class_name)"
}

function Assert-Click([string]$selector) {
    $click = Invoke-MblinkCli @('click', $selector, '--project', $project)
    Assert ($click.result.clicked -eq $true) "click did not report clicked=true for $selector"
    Start-Sleep -Milliseconds 120
}

function Assert-NoLiveErrors([string]$label) {
    $liveErrors = Invoke-MblinkCli @('errors', '--project', $project)
    Assert (@($liveErrors.errors).Count -eq 0) "$label reported JS errors: $($liveErrors | ConvertTo-Json -Compress -Depth 8)"
}

function Get-ColorCounts([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing PNG file: $path"
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::new($path)
    try {
        $counts = [ordered]@{
            blue = 0
            green = 0
            dark = 0
            coral = 0
        }
        for ($x = 0; $x -lt $bitmap.Width; $x += 8) {
            for ($y = 0; $y -lt $bitmap.Height; $y += 8) {
                $pixel = $bitmap.GetPixel($x, $y)
                if ($pixel.B -gt 170 -and $pixel.G -gt 90 -and $pixel.R -lt 100) { $counts.blue++ }
                if ($pixel.G -gt 130 -and $pixel.R -lt 80 -and $pixel.B -gt 70 -and $pixel.B -lt 180) { $counts.green++ }
                if ($pixel.R -lt 75 -and $pixel.G -lt 95 -and $pixel.B -gt 45 -and $pixel.B -lt 135) { $counts.dark++ }
                if ($pixel.R -gt 170 -and $pixel.G -gt 50 -and $pixel.G -lt 140 -and $pixel.B -gt 70 -and $pixel.B -lt 170) { $counts.coral++ }
            }
        }
        return $counts
    } finally {
        $bitmap.Dispose()
    }
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
  const canvas = document.querySelector('#leafer-stage-frame canvas');
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

function Assert-ScreenshotCanvasPixelNear([string]$path, [int]$canvasX, [int]$canvasY, [int]$r, [int]$g, [int]$b, [int]$tolerance, [string]$label) {
    $match = Assert-SelectorRect '#leafer-stage-frame canvas'
    $screenX = [int][Math]::Round([double]$match.rect.x + $canvasX)
    $screenY = [int][Math]::Round([double]$match.rect.y + $canvasY)
    Assert-PixelNear $path $screenX $screenY $r $g $b $tolerance $label
}

function Assert-LeaferTextAlignment {
    $code = @'
(() => {
  const api = globalThis.__leaferShowcase;
  if (!api || typeof api.getAlignmentState !== 'function') {
    return { ok: false, failures: ['alignment state API missing'] };
  }
  const state = api.getAlignmentState();
  const failures = [];
  const checkCentered = (name, node) => {
    if (!node) {
      failures.push(`${name} missing`);
      return;
    }
    if (node.textAlign !== 'center') failures.push(`${name} textAlign ${node.textAlign}`);
    if (node.verticalAlign !== 'middle') failures.push(`${name} verticalAlign ${node.verticalAlign}`);
    if (!(node.width > 0 && node.height > 0)) failures.push(`${name} empty text box`);
  };
  for (const shape of state.shapes) {
    checkCentered(`${shape.id}.label`, shape.label);
    checkCentered(`${shape.id}.detail`, shape.detail);
  }
  checkCentered('eventText', state.eventText);
  checkCentered('dragText', state.dragText);
  checkCentered('animationLabel', state.animationLabel);
  checkCentered('trackText', state.trackText);
  checkCentered('counterText', state.counterText);
  return { ok: failures.length === 0, failures, state };
})()
'@
    $alignment = Invoke-MblinkCli @('eval', $code, '--project', $project)
    Assert ($alignment.result.ok -eq $true) "Leafer text alignment mismatch: $($alignment | ConvertTo-Json -Compress -Depth 12)"
}

function Assert-LeaferAnimationSettled {
    $code = @'
(() => {
  const api = globalThis.__leaferShowcase;
  const state = api && typeof api.getAlignmentState === 'function' ? api.getAlignmentState() : null;
  if (!state) return { ok: false, failures: ['alignment state API missing'] };
  const failures = [];
  const puck = state.animationPuck;
  const trail = state.animationTrail;
  const label = state.animationLabel;
  if (!puck) failures.push('animationPuck missing');
  else {
    if (puck.x !== 64) failures.push(`puck x ${puck.x}`);
    if (puck.y !== 235) failures.push(`puck y ${puck.y}`);
    if (puck.width !== 30 || puck.height !== 30) failures.push(`puck size ${puck.width}x${puck.height}`);
    if (puck.fill !== '#f97316') failures.push(`puck fill ${puck.fill}`);
  }
  if (!trail) failures.push('animationTrail missing');
  else {
    if (trail.x !== 64) failures.push(`trail x ${trail.x}`);
    if (trail.y !== 245) failures.push(`trail y ${trail.y}`);
    if (trail.width !== 34) failures.push(`trail width ${trail.width}`);
  }
  if (!label) failures.push('animationLabel missing');
  else if (label.text !== 'RAF animation drives Leafer node props') failures.push(`label text ${label.text}`);
  return { ok: failures.length === 0, failures, state };
})()
'@
    $settled = Invoke-MblinkCli @('eval', $code, '--project', $project)
    Assert ($settled.result.ok -eq $true) "Leafer animation did not settle cleanly: $($settled | ConvertTo-Json -Compress -Depth 12)"
}

Assert (Test-Path -LiteralPath $project) "missing example project: $project"
Assert (Test-Path -LiteralPath $uiDev) "missing mblink-ui-dev: $uiDev"
Assert (Test-Path -LiteralPath $esmLoader) "missing esm_loader: $esmLoader"

$leaferVendor = Join-Path $project 'js/leafer-ui/web.module.min.js'
Assert (Test-Path -LiteralPath $leaferVendor) "missing vendored leafer-ui runtime: $leaferVendor"

Write-Host '[RUN] build leafer_ui_showcase'
$buildRaw = & $uiDev build --project $project 2>&1
$buildExit = $LASTEXITCODE
Assert ($buildExit -eq 0) "mblink-ui-dev build failed $buildExit`n$buildRaw"
$buildJson = $buildRaw | ConvertFrom-Json
Assert ($buildJson.ok -eq $true) "mblink-ui-dev build returned not ok: $($buildJson | ConvertTo-Json -Compress -Depth 8)"

if (Test-Path -LiteralPath $verify) {
    Remove-Item -LiteralPath $verify -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $verify | Out-Null

$entry = Join-Path $project '.dist/App.js'
Assert (Test-Path -LiteralPath $entry) "missing bundled entry: $entry"

Write-Host '[RUN] esm_loader leafer_ui_showcase'
$runtimeRaw = & $esmLoader $entry `
    --width 1024 `
    --height 700 `
    --title leafer-ui-showcase `
    --ui-dev-runtime-epoch leafer-showcase `
    --ui-dev-snapshot-file $loaderSnapshot `
    --ui-dev-snapshot-include-screenshot `
    --ui-dev-screenshot-file $loaderScreenshot `
    --ui-dev-console-file $console `
    --ui-dev-errors-file $errors `
    --ui-dev-lifecycle-file $lifecycle `
    --quit 1 2>&1
$runtimeExit = $LASTEXITCODE
Assert ($runtimeExit -eq 0) "esm_loader failed $runtimeExit`n$runtimeRaw"

$consoleJson = Read-JsonFile $console
$messages = @($consoleJson.entries | ForEach-Object { $_.message })
Assert ($messages -contains 'leafer-ui interactive showcase ready') "missing interactive ready console marker`n$($messages -join "`n")"

$errorsJson = Read-JsonFile $errors
Assert (@($errorsJson.errors).Count -eq 0) "unexpected JS errors: $($errorsJson | ConvertTo-Json -Compress -Depth 8)"

$snapshotText = Get-Content -Raw -LiteralPath $loaderSnapshot
foreach ($needle in @(
    'leafer-showcase-root',
    'leafer-showcase-status',
    'leafer-stage-host',
    'leafer-stage-frame',
    'add-shape-button',
    'shuffle-scene-button',
    'simulate-canvas-tap-button',
    'simulate-drag-button',
    'reorder-group-button',
    'remove-group-child-button',
    'raf-step-button',
    'play-animation-button',
    'toggle-visible-button',
    'api-shapes-readout',
    'group-state-readout',
    'canvas-event-readout',
    'drag-state-readout',
    'raf-readout',
    'animation-readout',
    'asset-readout',
    'selection-readout',
    'ready: compose mode, Brief',
    'leafer-canvas-view'
)) {
    Assert ($snapshotText.Contains($needle)) "snapshot missing $needle"
}

$initialColors = Get-ColorCounts $loaderScreenshot
Assert ($initialColors.blue -gt 60) "initial screenshot missing blue Leafer region; blue=$($initialColors.blue)"
Assert ($initialColors.green -gt 20) "initial screenshot missing green Leafer region; green=$($initialColors.green)"
Assert ($initialColors.dark -gt 20) "initial screenshot missing dark Leafer region; dark=$($initialColors.dark)"

Write-Host '[RUN] mblink-ui-dev interactive verification'
Stop-ShowcaseRuntime
try {
    Invoke-MblinkCli @('open', '--project', $project) | Out-Null

    $initialSnapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    Copy-EvidenceFile $initialSnapshot.snapshot.path $liveInitialSnapshot 'initial snapshot'
    Copy-EvidenceFile $initialSnapshot.screenshot.path $liveInitialScreenshot 'initial screenshot'
    Assert-PixelNear $liveInitialScreenshot 700 190 23 50 77 8 'compose mode Leafer banner'

    foreach ($selector in @(
        '#leafer-showcase-root',
        '#leafer-control-pane',
        '#leafer-stage-host',
        '#leafer-stage-frame',
        '#mode-compose',
        '#mode-inspect',
        '#mode-motion',
        '#palette-coral',
        '#add-shape-button',
        '#select-next-button',
        '#shuffle-scene-button',
        '#step-motion-button',
        '#clear-selection-button',
        '#simulate-canvas-tap-button',
        '#simulate-drag-button',
        '#reorder-group-button',
        '#remove-group-child-button',
        '#raf-step-button',
        '#play-animation-button',
        '#toggle-visible-button',
        '#scene-count-readout',
        '#shape-list-readout',
        '#selection-readout',
        '#api-shapes-readout',
        '#group-state-readout',
        '#canvas-event-readout',
        '#drag-state-readout',
        '#raf-readout',
        '#animation-readout',
        '#asset-readout',
        '#last-action-readout'
    )) {
        Assert-SelectorRect $selector | Out-Null
    }

    Assert-InspectContains '#scene-count-readout' '3 objects'
    Assert-InspectContains '#shape-list-readout' 'Brief'
    Assert-InspectContains '#selection-readout' 'Brief'
    Assert-InspectContains '#mode-readout' 'Compose'
    Assert-InspectContains '#api-shapes-readout' 'Rect'
    Assert-InspectContains '#api-shapes-readout' 'Image'
    Assert-InspectContains '#group-state-readout' 'step 0'
    Assert-InspectContains '#canvas-event-readout' 'tap 0 click 0 events 0'
    Assert-InspectContains '#drag-state-readout' '0 drags'
    Assert-InspectContains '#raf-readout' 'step 0'
    Assert-InspectContains '#animation-readout' 'idle frame 0'
    Wait-InspectContains '#asset-readout' 'image 4x4 loaded'
    Assert-LeaferTextAlignment
    Assert-LeaferAnimationSettled
    Assert-ActiveClass '#mode-compose' $true
    Assert-ActiveClass '#mode-inspect' $false
    Assert-ActiveClass '#mode-motion' $false

    Assert-Click '#mode-inspect'
    Assert-CanvasPixelNear 326 64 15 63 58 18 'inspect mode immediate repaint'
    Assert-InspectContains '#mode-readout' 'Inspect'
    Assert-InspectContains '#leafer-stage-caption' 'Mode Inspect'
    Assert-ActiveClass '#mode-compose' $false
    Assert-ActiveClass '#mode-inspect' $true
    Assert-ActiveClass '#mode-motion' $false
    Capture-DevScreenshot $liveInspectScreenshot 'inspect mode' $liveInspectSnapshot | Out-Null
    Assert-PixelNear $liveInspectScreenshot 700 190 15 63 58 8 'inspect mode Leafer banner'

    Assert-Click '#palette-coral'
    Assert-InspectContains '#color-readout' 'Coral'
    Assert-InspectContains '#last-action-readout' 'fill Coral'

    Assert-Click '#add-shape-button'
    Assert-InspectContains '#scene-count-readout' '4 objects'
    Assert-InspectContains '#shape-list-readout' 'Layer 1'
    Assert-InspectContains '#selection-readout' 'Layer 1'
    Assert-InspectContains '#last-action-readout' 'added Layer 1'
    Assert-LeaferTextAlignment

    Assert-Click '#shuffle-scene-button'
    Assert-InspectContains '#last-action-readout' 'shuffled scene'

    Assert-Click '#mode-motion'
    Assert-InspectContains '#mode-readout' 'Motion'
    Assert-InspectContains '#leafer-stage-caption' 'Mode Motion'
    Assert-ActiveClass '#mode-compose' $false
    Assert-ActiveClass '#mode-inspect' $false
    Assert-ActiveClass '#mode-motion' $true

    Assert-Click '#step-motion-button'
    Assert-InspectContains '#last-action-readout' 'motion step'
    Assert-InspectContains '#leafer-showcase-status' 'motion mode'
    Assert-LeaferTextAlignment

    Assert-Click '#clear-selection-button'
    Assert-InspectContains '#selection-readout' 'none selected'
    Assert-InspectContains '#last-action-readout' 'cleared selection'

    Assert-Click '#simulate-canvas-tap-button'
    Assert-InspectContains '#canvas-event-readout' 'tap 1 click 1 events 1'
    Assert-InspectContains '#last-action-readout' 'simulated canvas tap'
    Assert-CanvasPixelNear 52 350 192 38 211 45 'tap event immediate repaint'
    Capture-DevScreenshot $devTapScreenshot 'canvas tap' | Out-Null
    Assert-PixelNear $devTapScreenshot 450 475 192 38 211 45 'dev screenshot tap event Leafer fill'
    Assert-NoLiveErrors 'canvas tap interaction'

    Assert-Click '#simulate-drag-button'
    Assert-InspectContains '#drag-state-readout' '1 drags'
    Assert-InspectContains '#last-action-readout' 'simulated drag'
    Assert-CanvasPixelNear 198 364 239 68 68 50 'drag event immediate repaint'
    Capture-DevScreenshot $devDragScreenshot 'drag event' | Out-Null
    Assert-PixelNear $devDragScreenshot 570 505 239 68 68 50 'dev screenshot drag event Leafer fill'
    Assert-NoLiveErrors 'drag interaction'

    Assert-Click '#reorder-group-button'
    Assert-InspectContains '#group-state-readout' 'step 1'
    Assert-InspectContains '#last-action-readout' 'reordered group 1'
    Assert-CanvasPixelNear 226 306 37 99 235 55 'group reorder immediate repaint'
    Capture-DevScreenshot $devReorderScreenshot 'group reorder' | Out-Null
    Assert-PixelNear $devReorderScreenshot 622 440 37 99 235 55 'dev screenshot reordered path fill'
    Assert-NoLiveErrors 'group reorder interaction'

    Assert-Click '#raf-step-button'
    Wait-InspectContains '#raf-readout' 'step 1'
    Wait-InspectContains '#last-action-readout' 'raf frame 1'
    Assert-CanvasPixelNear 400 340 249 115 22 55 'RAF frame immediate repaint'
    Capture-DevScreenshot $devRafScreenshot 'RAF frame' | Out-Null
    Assert-PixelNear $devRafScreenshot 760 485 249 115 22 55 'dev screenshot RAF frame fill'
    Assert-NoLiveErrors 'RAF frame interaction'

    Assert-Click '#play-animation-button'
    Wait-InspectContains '#animation-readout' 'complete frame 18'
    Assert-InspectContains '#last-action-readout' 'animation complete'
    Assert-LeaferAnimationSettled
    Assert-CanvasPixelNear 79 250 249 115 22 65 'Leafer animation settled puck repaint'
    Assert-CanvasPixelNear 543 251 227 237 247 65 'Leafer animation settled without right-edge drift'
    Capture-DevScreenshot $devAnimationScreenshot 'Leafer animation' | Out-Null
    Assert-ScreenshotCanvasPixelNear $devAnimationScreenshot 79 250 249 115 22 65 'dev screenshot Leafer animation settled puck'
    Assert-ScreenshotCanvasPixelNear $devAnimationScreenshot 543 251 227 237 247 65 'dev screenshot Leafer animation no right-edge drift'
    Assert-NoLiveErrors 'Leafer animation interaction'

    Assert-Click '#toggle-visible-button'
    Assert-InspectContains '#last-action-readout' 'visible node on'
    Assert-CanvasPixelNear 456 290 124 58 237 55 'visible toggle immediate repaint'
    Capture-DevScreenshot $devVisibleScreenshot 'visible toggle' | Out-Null
    Assert-PixelNear $devVisibleScreenshot 845 440 124 58 237 55 'dev screenshot visible node fill'
    Assert-NoLiveErrors 'visible toggle interaction'

    Assert-Click '#remove-group-child-button'
    Assert-InspectContains '#group-state-readout' 'removed yes'
    Assert-InspectContains '#last-action-readout' 'removed group child'
    Assert-CanvasPixelNear 456 290 248 250 252 55 'remove child immediate repaint'
    Capture-DevScreenshot $devRemovedScreenshot 'remove child' | Out-Null
    Assert-PixelNear $devRemovedScreenshot 845 440 248 250 252 55 'dev screenshot removed child background'
    Assert-NoLiveErrors 'remove group child interaction'

    $afterSnapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    Copy-EvidenceFile $afterSnapshot.snapshot.path $liveAfterSnapshot 'post-click snapshot'
    Copy-EvidenceFile $afterSnapshot.screenshot.path $liveAfterScreenshot 'post-click screenshot'

    $afterText = Get-Content -Raw -LiteralPath $liveAfterSnapshot
    foreach ($needle in @(
        '4 objects',
        'Layer 1',
        'Motion',
        'none selected',
        'tap 1 click 1 events 1',
        '1 drags',
        'step 1',
        'complete frame 18',
        'removed yes',
        'image 4x4 loaded',
        'removed group child'
    )) {
        Assert ($afterText.Contains($needle)) "post-click snapshot missing $needle"
    }

    $afterColors = Get-ColorCounts $liveAfterScreenshot
    Assert ($afterColors.coral -gt 20) "post-click screenshot missing coral Leafer region; coral=$($afterColors.coral)"
    Assert ($afterColors.green -gt 20) "post-click screenshot missing green Leafer region; green=$($afterColors.green)"
    Assert ($afterColors.dark -gt 20) "post-click screenshot missing dark Leafer region; dark=$($afterColors.dark)"
    Assert-PixelNear $liveAfterScreenshot 700 190 91 53 18 8 'motion mode Leafer banner'

    $logs = Invoke-MblinkCli @('logs', '--project', $project)
    $logText = $logs | ConvertTo-Json -Compress -Depth 8
    Assert ($logText.Contains('leafer-ui interactive showcase ready')) 'runtime logs missing ready marker'

    $liveErrors = Invoke-MblinkCli @('errors', '--project', $project)
    Assert (@($liveErrors.errors).Count -eq 0) "live runtime JS errors were reported: $($liveErrors | ConvertTo-Json -Compress -Depth 8)"
}
finally {
    Stop-ShowcaseRuntime
}

Write-Host '[PASS] leafer-ui showcase renders and responds through vendored js package'
