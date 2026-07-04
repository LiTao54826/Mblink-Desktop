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
    '--height', '480',
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

$errorsJson = Read-JsonFile $errors
Assert (@($errorsJson.errors).Count -eq 0) "unexpected JS errors: $($errorsJson | ConvertTo-Json -Compress -Depth 8)"

$snapshotText = Get-Content -Raw -LiteralPath $snapshot
Assert ($snapshotText.Contains('leafer-api-compat')) 'snapshot missing compat root'
Assert ($snapshotText.Contains('leafer-api-status')) 'snapshot missing status node'

Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::new($screenshot)
try {
    $pixel = $bitmap.GetPixel(16, 16)
    Assert ($pixel.B -gt 180 -and $pixel.R -lt 80 -and $pixel.G -gt 80) "canvas draw pixel was not blue: R=$($pixel.R) G=$($pixel.G) B=$($pixel.B)"
} finally {
    $bitmap.Dispose()
}

Write-Host '[PASS] leafer-ui API compatibility runtime guard green'
