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

$exe = Join-Path (Get-Location) 'build/bin/Release/esm_loader.exe'
$entry = Join-Path (Get-Location) 'tests/js/leafer_ui_api_compat_fixture.js'
$tmpRoot = Join-Path (Get-Location) 'tmp/leafer_ui_api_compat'
$snapshot = Join-Path $tmpRoot 'snapshot.json'
$screenshot = Join-Path $tmpRoot 'snapshot.png'
$console = Join-Path $tmpRoot 'console.json'
$errors = Join-Path $tmpRoot 'errors.json'
$lifecycle = Join-Path $tmpRoot 'lifecycle.json'

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) {
        throw $message
    }
}

function Read-JsonFile([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing json file: $path"
    return Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
}

Assert (Test-Path -LiteralPath $exe) "missing esm_loader: $exe"
Assert (Test-Path -LiteralPath $entry) "missing fixture: $entry"

if (Test-Path -LiteralPath $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

$arguments = @(
    $entry,
    '--width', '640',
    '--height', '560',
    '--title', 'leafer-api-compat',
    '--ui-dev-runtime-epoch', 'leafer-api-compat',
    '--ui-dev-snapshot-file', $snapshot,
    '--ui-dev-snapshot-include-screenshot',
    '--ui-dev-screenshot-file', $screenshot,
    '--ui-dev-console-file', $console,
    '--ui-dev-errors-file', $errors,
    '--ui-dev-lifecycle-file', $lifecycle,
    '--quit', '1'
)

$raw = & $exe @arguments 2>&1
$exit = $LASTEXITCODE
Assert ($exit -eq 0) "esm_loader exited $exit`n$raw"

$consoleJson = Read-JsonFile $console
$messages = @($consoleJson.entries | ForEach-Object { $_.message })
Assert ($messages -contains '[leafer-api-compat] ready') "missing leafer API ready marker`n$($messages -join "`n")"
Assert ($messages -contains '[leafer-api-compat] docs coverage ready') "missing Leafer docs coverage marker`n$($messages -join "`n")"

$errorsJson = Read-JsonFile $errors
Assert (@($errorsJson.errors).Count -eq 0) "unexpected JS errors: $($errorsJson | ConvertTo-Json -Compress -Depth 8)"

$snapshotText = Get-Content -Raw -LiteralPath $snapshot
Assert ($snapshotText.Contains('leafer-api-compat')) 'snapshot missing compat root'
Assert ($snapshotText.Contains('leafer-api-status')) 'snapshot missing status node'
Assert ($snapshotText.Contains('leafer docs api compat')) 'snapshot missing docs compat status text'
Assert ($snapshotText.Contains('leafer-api-canvas')) 'snapshot missing direct canvas'
Assert ($snapshotText.Contains('leafer-canvas-view-host')) 'snapshot missing canvas view host'
Assert ($snapshotText.Contains('leafer-id-view-host')) 'snapshot missing id view host'

Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::new($screenshot)
try {
    $pixel = $bitmap.GetPixel(16, 16)
    Assert ($pixel.B -gt 180 -and $pixel.R -lt 80 -and $pixel.G -gt 80) "direct canvas draw pixel was not blue: R=$($pixel.R) G=$($pixel.G) B=$($pixel.B)"

    $polygonPixel = $bitmap.GetPixel(316, 198)
    Assert ($polygonPixel.G -gt 130 -and $polygonPixel.B -gt 80 -and $polygonPixel.B -lt 180 -and $polygonPixel.R -lt 80) "Leafer polygon pixel was not green: R=$($polygonPixel.R) G=$($polygonPixel.G) B=$($polygonPixel.B)"

    $pathPixel = $bitmap.GetPixel(254, 198)
    Assert ($pathPixel.R -gt 180 -and $pathPixel.G -gt 50 -and $pathPixel.G -lt 150 -and $pathPixel.B -gt 80 -and $pathPixel.B -lt 170) "Leafer path pixel was not magenta: R=$($pathPixel.R) G=$($pathPixel.G) B=$($pathPixel.B)"

    $canvasPixel = $bitmap.GetPixel(337, 247)
    Assert ($canvasPixel.R -gt 220 -and $canvasPixel.G -gt 170 -and $canvasPixel.B -lt 80) "Leafer Canvas fixture pixel was not yellow: R=$($canvasPixel.R) G=$($canvasPixel.G) B=$($canvasPixel.B)"
} finally {
    $bitmap.Dispose()
}

Write-Host '[PASS] leafer-ui API compatibility runtime guard green'
