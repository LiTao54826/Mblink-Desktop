$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$verify = Join-Path $repoRoot 'examples\complex_layout_dynamics\verify.ps1'

powershell -NoProfile -ExecutionPolicy Bypass -File $verify @args
if ($LASTEXITCODE -ne 0) {
    throw "complex layout dynamics verification failed with exit code $LASTEXITCODE"
}
