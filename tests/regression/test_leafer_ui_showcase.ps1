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
        '#scene-count-readout',
        '#shape-list-readout',
        '#selection-readout',
        '#last-action-readout'
    )) {
        Assert-SelectorRect $selector | Out-Null
    }

    Assert-InspectContains '#scene-count-readout' '3 objects'
    Assert-InspectContains '#shape-list-readout' 'Brief'
    Assert-InspectContains '#selection-readout' 'Brief'
    Assert-InspectContains '#mode-readout' 'Compose'
    Assert-ActiveClass '#mode-compose' $true
    Assert-ActiveClass '#mode-inspect' $false
    Assert-ActiveClass '#mode-motion' $false

    Assert-Click '#mode-inspect'
    Assert-InspectContains '#mode-readout' 'Inspect'
    Assert-InspectContains '#leafer-stage-caption' 'Mode Inspect'
    Assert-ActiveClass '#mode-compose' $false
    Assert-ActiveClass '#mode-inspect' $true
    Assert-ActiveClass '#mode-motion' $false
    $inspectSnapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    Copy-EvidenceFile $inspectSnapshot.snapshot.path $liveInspectSnapshot 'inspect snapshot'
    Copy-EvidenceFile $inspectSnapshot.screenshot.path $liveInspectScreenshot 'inspect screenshot'
    Assert-PixelNear $liveInspectScreenshot 700 190 15 63 58 8 'inspect mode Leafer banner'

    Assert-Click '#palette-coral'
    Assert-InspectContains '#color-readout' 'Coral'
    Assert-InspectContains '#last-action-readout' 'fill Coral'

    Assert-Click '#add-shape-button'
    Assert-InspectContains '#scene-count-readout' '4 objects'
    Assert-InspectContains '#shape-list-readout' 'Layer 1'
    Assert-InspectContains '#selection-readout' 'Layer 1'
    Assert-InspectContains '#last-action-readout' 'added Layer 1'

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

    Assert-Click '#clear-selection-button'
    Assert-InspectContains '#selection-readout' 'none selected'
    Assert-InspectContains '#last-action-readout' 'cleared selection'

    $afterSnapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    Copy-EvidenceFile $afterSnapshot.snapshot.path $liveAfterSnapshot 'post-click snapshot'
    Copy-EvidenceFile $afterSnapshot.screenshot.path $liveAfterScreenshot 'post-click screenshot'

    $afterText = Get-Content -Raw -LiteralPath $liveAfterSnapshot
    foreach ($needle in @('4 objects', 'Layer 1', 'Motion', 'none selected', 'cleared selection')) {
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
