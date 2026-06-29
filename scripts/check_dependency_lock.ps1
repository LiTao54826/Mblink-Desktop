# Validates bootstrapped dependency metadata across lock, bootstrap script, and notices.

[CmdletBinding()]
param(
    [string]$LockPath = "third_party.lock.json",
    [string]$BootstrapScriptPath = "scripts\download_deps.ps1",
    [string]$NoticePath = "THIRD_PARTY_NOTICES.md"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot

function Resolve-ProjectPath {
    param([string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return (Join-Path $projectRoot $Path)
}

$lockFile = Resolve-ProjectPath $LockPath
$bootstrapFile = Resolve-ProjectPath $BootstrapScriptPath
$noticeFile = Resolve-ProjectPath $NoticePath

foreach ($path in @($lockFile, $bootstrapFile, $noticeFile)) {
    if (-not (Test-Path -LiteralPath $path)) {
        Write-Error "Required dependency metadata file is missing: $path"
        exit 1
    }
}

$lock = Get-Content -LiteralPath $lockFile -Raw | ConvertFrom-Json
$bootstrap = Get-Content -LiteralPath $bootstrapFile -Raw
$notice = Get-Content -LiteralPath $noticeFile -Raw

foreach ($dependency in $lock.dependencies) {
    foreach ($field in @("name", "target", "url", "license", "ref")) {
        if (-not $dependency.PSObject.Properties.Name.Contains($field)) {
            Write-Error "Dependency lock entry is missing field '$field'."
            exit 1
        }
        if ([string]::IsNullOrWhiteSpace([string]$dependency.$field)) {
            Write-Error "Dependency lock entry has empty field '$field'."
            exit 1
        }
    }

    foreach ($value in @($dependency.url, $dependency.ref)) {
        if ($bootstrap -notmatch [regex]::Escape([string]$value)) {
            Write-Error "$BootstrapScriptPath is missing $($dependency.name) value: $value"
            exit 1
        }
    }

    foreach ($value in @($dependency.url, $dependency.ref, $dependency.license)) {
        if ($notice -notmatch [regex]::Escape([string]$value)) {
            Write-Error "$NoticePath is missing $($dependency.name) value: $value"
            exit 1
        }
    }
}

$lockedUrls = @($lock.dependencies | ForEach-Object { [string]$_.url })
$lockedRefs = @($lock.dependencies | ForEach-Object { [string]$_.ref })

$scriptUrls = [regex]::Matches($bootstrap, 'https://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+\.git') |
    ForEach-Object { $_.Value } |
    Sort-Object -Unique
foreach ($url in $scriptUrls) {
    if ($lockedUrls -notcontains $url) {
        Write-Error "$BootstrapScriptPath contains URL not represented in ${LockPath}: $url"
        exit 1
    }
}

$noticeUrls = [regex]::Matches($notice, 'https://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+\.git') |
    ForEach-Object { $_.Value } |
    Sort-Object -Unique
foreach ($url in $noticeUrls) {
    if ($lockedUrls -notcontains $url) {
        Write-Error "$NoticePath contains URL not represented in ${LockPath}: $url"
        exit 1
    }
}

$noticeShaValues = [regex]::Matches($notice, '\b[0-9a-f]{40}\b') |
    ForEach-Object { $_.Value } |
    Sort-Object -Unique
foreach ($ref in $noticeShaValues) {
    if ($lockedRefs -notcontains $ref) {
        Write-Error "$NoticePath contains 40-character ref not represented in ${LockPath}: $ref"
        exit 1
    }
}

Write-Host "Dependency lock metadata OK."
