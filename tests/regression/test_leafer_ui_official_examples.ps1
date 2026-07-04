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
$uiDev = Join-Path $repo 'build/bin/Release/mblink-ui-dev.exe'
$vendor = Join-Path $repo 'examples/leafer_ui_showcase/js/leafer-ui/web.module.min.js'
$fixture = Join-Path $repo 'tests/js/leafer_ui_official_examples_fixture.js'
$project = Join-Path $repo 'tmp/leafer_official_examples_project'
$verify = Join-Path $repo 'tmp/leafer_official_examples_verify'
$resultsFile = Join-Path $verify 'official_example_results.json'
$initialSnapshot = Join-Path $verify 'initial_snapshot.json'
$initialScreenshot = Join-Path $verify 'initial_screenshot.png'
$afterSnapshot = Join-Path $verify 'after_run_snapshot.json'
$afterScreenshot = Join-Path $verify 'after_run_screenshot.png'

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

function Stop-OfficialRuntime() {
    try { Invoke-MblinkCli @('stop', '--project', $project) | Out-Null } catch {}
    $projectNeedle = [string]$project
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

function Copy-EvidenceFile([object]$source, [string]$destination, [string]$label) {
    Assert ($source -and (Test-Path -LiteralPath $source)) "missing $label evidence: $source"
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

function Capture-DevSnapshot([string]$snapshotDestination, [string]$screenshotDestination, [string]$label) {
    $snapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    Copy-EvidenceFile $snapshot.snapshot.path $snapshotDestination "$label snapshot"
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

function Wait-InspectContains([string]$selector, [string]$needle, [int]$timeoutMs = 120000) {
    $deadline = (Get-Date).AddMilliseconds($timeoutMs)
    $lastHtml = ''
    do {
        $inspect = Invoke-MblinkCli @('inspect', $selector, '--project', $project)
        Assert ($inspect.result.found -eq $true) "inspect did not find $selector"
        $lastHtml = [string]$inspect.result.outer_html
        if ($lastHtml -like "*$needle*") {
            return
        }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)

    throw "inspect $selector missing '$needle' after ${timeoutMs}ms: $lastHtml"
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

function Assert-ProofCanvasPixel() {
    $code = @"
(() => {
  const canvas = document.querySelector('#official-example-proof canvas');
  if (!canvas) return { ok: false, error: 'proof canvas missing' };
  const context = canvas.getContext('2d');
  if (!context) return { ok: false, error: '2d context missing' };
  const data = context.getImageData(36, 36, 1, 1).data;
  return { ok: true, rgba: [data[0], data[1], data[2], data[3]] };
})()
"@
    $sample = Invoke-MblinkCli @('eval', $code, '--project', $project)
    Assert ($sample.result.ok -eq $true) "proof canvas sample failed: $($sample | ConvertTo-Json -Compress -Depth 8)"
    $rgba = @($sample.result.rgba)
    $delta = [Math]::Abs([int]$rgba[0] - 22) + [Math]::Abs([int]$rgba[1] - 163) + [Math]::Abs([int]$rgba[2] - 74)
    Assert ($delta -le 35) "proof canvas pixel mismatch; expected green actual rgb($($rgba[0]),$($rgba[1]),$($rgba[2])) delta=$delta"
}

function Get-OfficialDocsRoot() {
    if ($env:LEAFER_AI_DOCS_ROOT -and (Test-Path -LiteralPath $env:LEAFER_AI_DOCS_ROOT)) {
        return (Resolve-Path -LiteralPath $env:LEAFER_AI_DOCS_ROOT).Path
    }

    $cached = Join-Path $env:LOCALAPPDATA 'Temp/codex-leafer-ai-docs'
    if (Test-Path -LiteralPath (Join-Path $cached 'examples')) {
        return (Resolve-Path -LiteralPath $cached).Path
    }

    $zip = 'C:\Users\Administrator\Downloads\ai-docs-main.zip'
    Assert (Test-Path -LiteralPath $zip) "missing Leafer official docs zip: $zip; set LEAFER_AI_DOCS_ROOT to an extracted leaferjs/ai-docs checkout"
    $extractParent = Join-Path $env:LOCALAPPDATA 'Temp/codex-leafer-ai-docs-extract'
    if (Test-Path -LiteralPath $extractParent) {
        Remove-Item -LiteralPath $extractParent -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $extractParent | Out-Null
    Expand-Archive -LiteralPath $zip -DestinationPath $extractParent -Force
    $root = Get-ChildItem -LiteralPath $extractParent -Directory | Select-Object -First 1
    Assert ($null -ne $root) "failed to extract Leafer official docs zip: $zip"
    return $root.FullName
}

function Get-RelativePath([string]$root, [string]$path) {
    $rootUri = [System.Uri]::new((Resolve-Path -LiteralPath $root).Path.TrimEnd('\') + '\')
    $pathUri = [System.Uri]::new((Resolve-Path -LiteralPath $path).Path)
    return [Uri]::UnescapeDataString($rootUri.MakeRelativeUri($pathUri).ToString()).Replace('/', '\')
}

function Get-ImportSpecifiers([string]$source) {
    $list = @()
    $lines = $source -split "`n"
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        if ($line -notmatch '^\s*import\b') {
            continue
        }

        $block = $line
        while ($block -notmatch "['""][^'""]+['""]" -and $index + 1 -lt $lines.Count) {
            $index += 1
            $block += "`n" + [string]$lines[$index]
        }

        $match = [regex]::Match($block, "from\s+['""]([^'""]+)['""]")
        if (-not $match.Success) {
            $match = [regex]::Match($block, "^\s*import\s+['""]([^'""]+)['""]")
        }
        if ($match.Success) {
            $list += [string]$match.Groups[1].Value
        }
    }
    return $list
}

function Get-SkipReason([string]$relativePath, [string]$extension, [string]$source, [string[]]$imports) {
    $unixPath = $relativePath.Replace('\', '/')
    if ($extension -eq '.html') { return 'html-wrapper' }
    if ($unixPath -like 'nodejs/*') { return 'node' }
    if ($unixPath -like 'worker/*' -or $source -match '\bnew\s+Worker\s*\(') { return 'worker' }
    if ($unixPath -like 'performance/million*') { return 'stress:million' }
    if ($unixPath -eq 'basic/debug/custom.ts') { return 'intentional-debug-error-output' }
    if ($unixPath -eq 'event/changeName.ts') { return 'global-event-name-mutation' }
    if ($source -match '(?m)^\s*export\s+' -or $source -match '(?m)^\s*interface\s+[A-Za-z_]') { return 'ts-api-only' }

    $external = @()
    foreach ($specifier in $imports) {
        if ($specifier -eq 'leafer-ui') { continue }
        if ($specifier.StartsWith('.')) {
            return "relative-import:$specifier"
        }
        $external += $specifier
    }
    if ($external.Count -gt 0) {
        return 'external:' + (($external | Sort-Object -Unique) -join '|')
    }
    return ''
}

function ConvertTo-JsonString([string]$value) {
    if ($null -eq $value) {
        return 'null'
    }
    $builder = [System.Text.StringBuilder]::new()
    [void]$builder.Append('"')
    foreach ($ch in $value.ToCharArray()) {
        $code = [int][char]$ch
        switch ($ch) {
            '"' { [void]$builder.Append('\"'); continue }
            '\' { [void]$builder.Append('\\'); continue }
            "`b" { [void]$builder.Append('\b'); continue }
            "`f" { [void]$builder.Append('\f'); continue }
            "`n" { [void]$builder.Append('\n'); continue }
            "`r" { [void]$builder.Append('\r'); continue }
            "`t" { [void]$builder.Append('\t'); continue }
            default {
                if ($code -lt 32) {
                    [void]$builder.Append('\u')
                    [void]$builder.Append($code.ToString('x4'))
                } else {
                    [void]$builder.Append($ch)
                }
            }
        }
    }
    [void]$builder.Append('"')
    return $builder.ToString()
}

function ConvertTo-JsonStringArray([string[]]$values) {
    $parts = @()
    foreach ($value in $values) {
        $parts += ConvertTo-JsonString $value
    }
    return '[' + ($parts -join ',') + ']'
}

function ConvertTo-OfficialExamplesJson([object[]]$items) {
    $parts = @()
    foreach ($item in $items) {
        $parts += '{"path":' + (ConvertTo-JsonString ([string]$item.path)) +
            ',"source":' + (ConvertTo-JsonString ([string]$item.source)) +
            ',"imports":' + (ConvertTo-JsonStringArray ([string[]]$item.imports)) + '}'
    }
    return '[' + ($parts -join ',') + ']'
}

function ConvertTo-SkippedExamplesJson([object[]]$items) {
    $parts = @()
    foreach ($item in $items) {
        $parts += '{"path":' + (ConvertTo-JsonString ([string]$item.path)) +
            ',"reason":' + (ConvertTo-JsonString ([string]$item.reason)) + '}'
    }
    return '[' + ($parts -join ',') + ']'
}

Assert (Test-Path -LiteralPath $uiDev) "missing mblink-ui-dev: $uiDev"
Assert (Test-Path -LiteralPath $vendor) "missing vendored leafer-ui runtime: $vendor"
Assert (Test-Path -LiteralPath $fixture) "missing official examples fixture: $fixture"

$docsRoot = Get-OfficialDocsRoot
$examplesRoot = Join-Path $docsRoot 'examples'
Assert (Test-Path -LiteralPath $examplesRoot) "missing official examples directory: $examplesRoot"

$examples = @()
$skipped = @()
$files = Get-ChildItem -LiteralPath $examplesRoot -Recurse -File |
    Where-Object { $_.Extension -in @('.js', '.ts', '.html') } |
    Sort-Object FullName

foreach ($file in $files) {
    $relative = Get-RelativePath $examplesRoot $file.FullName
    $relativeUnix = $relative.Replace('\', '/')
    $source = Get-Content -Raw -Encoding UTF8 -LiteralPath $file.FullName
    $imports = @(Get-ImportSpecifiers $source)
    $reason = Get-SkipReason $relative $file.Extension $source $imports
    if ($reason) {
        $skipped += [ordered]@{ path = $relativeUnix; reason = $reason }
    } else {
        $examples += [ordered]@{ path = $relativeUnix; source = $source; imports = $imports }
    }
}

Assert ($files.Count -ge 900) "official examples scan found too few files: $($files.Count)"
Assert ($examples.Count -ge 250) "official executable example count too low: $($examples.Count)"

Stop-OfficialRuntime

if (Test-Path -LiteralPath $project) {
    Remove-Item -LiteralPath $project -Recurse -Force
}
if (Test-Path -LiteralPath $verify) {
    Remove-Item -LiteralPath $verify -Recurse -Force
}
New-Item -ItemType Directory -Force -Path (Join-Path $project 'ui/leafer-ui') | Out-Null
New-Item -ItemType Directory -Force -Path $verify | Out-Null

Copy-Item -LiteralPath $vendor -Destination (Join-Path $project 'ui/leafer-ui/web.module.min.js') -Force
Copy-Item -LiteralPath $fixture -Destination (Join-Path $project 'ui/leafer_ui_official_examples_fixture.js') -Force

$examplesJson = ConvertTo-OfficialExamplesJson ([object[]]$examples)
$skippedJson = ConvertTo-SkippedExamplesJson ([object[]]$skipped)
$appJs = @"
import { runOfficialExamples } from './leafer_ui_official_examples_fixture.js';

const examples = $examplesJson;
const skipped = $skippedJson;

runOfficialExamples({
  examples,
  skipped,
  options: {
    exampleDelayMs: 70,
    progressEvery: 20
  }
});
"@
[System.IO.File]::WriteAllText((Join-Path $project 'ui/app.js'), $appJs, [System.Text.UTF8Encoding]::new($false))

$config = @{
    name = 'leafer_official_examples'
    entry = 'ui/app.js'
    src_dir = 'ui'
    out_dir = '.dist'
    runtime = 'tool'
    purpose = 'showcase'
    window = @{
        title = 'Leafer Official Examples'
        width = 1040
        height = 720
        resizable = $true
    }
    build = @{
        builder = 'esbuild'
        minify = $false
    }
}
$configJson = $config | ConvertTo-Json -Depth 8
[System.IO.File]::WriteAllText((Join-Path $project 'mblink.config.json'), $configJson, [System.Text.UTF8Encoding]::new($false))

Write-Host "[RUN] generated Leafer official examples project: executable=$($examples.Count) skipped=$($skipped.Count) total=$($files.Count)"

$buildRaw = & $uiDev build --project $project 2>&1
$buildExit = $LASTEXITCODE
Assert ($buildExit -eq 0) "mblink-ui-dev build failed $buildExit`n$buildRaw"
$buildJson = $buildRaw | ConvertFrom-Json
Assert ($buildJson.ok -eq $true) "mblink-ui-dev build returned not ok: $($buildJson | ConvertTo-Json -Compress -Depth 8)"

Stop-OfficialRuntime
try {
    Invoke-MblinkCli @('open', '--project', $project) | Out-Null
    Capture-DevSnapshot $initialSnapshot $initialScreenshot 'initial official examples' | Out-Null

    foreach ($selector in @(
        '#official-example-runner',
        '#official-example-run-button',
        '#official-example-summary',
        '#official-example-canvas-host',
        '#official-example-proof',
        '#official-example-status',
        '#official-example-pass-badge'
    )) {
        Assert-SelectorRect $selector | Out-Null
    }
    Wait-InspectContains '#official-example-summary' "ready: executable $($examples.Count), skipped $($skipped.Count)" 5000

    $click = Invoke-MblinkCli @('click', '#official-example-run-button', '--project', $project)
    Assert ($click.result.clicked -eq $true) 'official examples run button click did not report clicked=true'

    Wait-InspectContains '#official-example-summary' "checked: $($examples.Count)" 180000
    $resultProbe = Invoke-MblinkCli @('eval', '(() => window.__LEAFER_OFFICIAL_EXAMPLE_RESULTS__ || null)()', '--project', $project)
    Assert ($null -ne $resultProbe.result) 'official example results missing from runtime'
    ($resultProbe.result | ConvertTo-Json -Depth 12) | Set-Content -Encoding UTF8 -LiteralPath $resultsFile
    Assert ([int]$resultProbe.result.executable -eq [int]$examples.Count) 'official executable count mismatch'
    Assert ([int]$resultProbe.result.skipped -eq [int]$skipped.Count) 'official skipped count mismatch'
    if ([int]$resultProbe.result.failed -ne 0) {
        $failureSummary = @($resultProbe.result.failures | Select-Object -First 12 | ForEach-Object {
            "$($_.path): $($_.name) $($_.message)"
        }) -join "`n"
        throw "official examples reported failures:`n$failureSummary`nfull results: $resultsFile"
    }

    Wait-InspectContains '#official-example-summary' 'failed: 0' 5000
    Wait-InspectContains '#official-example-pass-badge' 'official examples pass' 5000
    Capture-DevSnapshot $afterSnapshot $afterScreenshot 'after official examples run' | Out-Null

    Assert-PixelNear $afterScreenshot 930 50 22 163 74 40 'official pass badge live screenshot'
    Assert-PixelNear $afterScreenshot 802 130 22 163 74 45 'official proof Leafer canvas screenshot'
    Assert-ProofCanvasPixel

    $logs = Invoke-MblinkCli @('logs', '--project', $project)
    $logText = $logs | ConvertTo-Json -Compress -Depth 8
    Assert ($logText.Contains('[leafer-official-examples] checked')) 'runtime logs missing official examples checked marker'

    $errors = Invoke-MblinkCli @('errors', '--project', $project)
    Assert (@($errors.errors).Count -eq 0) "live runtime JS errors were reported: $($errors | ConvertTo-Json -Compress -Depth 8)"
}
finally {
    Stop-OfficialRuntime
}

Write-Host "[PASS] Leafer official core examples verified one by one with mblink-ui-dev screenshots"
