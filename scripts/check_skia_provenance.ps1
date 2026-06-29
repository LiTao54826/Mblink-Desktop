# Validates the Skia provenance record required before publishing binaries.

[CmdletBinding()]
param(
    [string]$Path = "third_party\skia\SKIA_PROVENANCE.json",
    [switch]$RequireLibraries
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$provenancePath = if ([System.IO.Path]::IsPathRooted($Path)) {
    $Path
} else {
    Join-Path $projectRoot $Path
}

function Fail {
    param([string]$Message)
    Write-Error $Message
    exit 1
}

if (-not (Test-Path -LiteralPath $provenancePath)) {
    Fail "Missing Skia provenance file: $provenancePath"
}

$raw = Get-Content -LiteralPath $provenancePath -Raw
try {
    $provenance = $raw | ConvertFrom-Json
} catch {
    Fail "Skia provenance file is not valid JSON: $provenancePath"
}

if ($raw -notmatch '"licenseFiles"\s*:\s*\[') {
    Fail "Skia provenance licenseFiles must be a JSON array."
}

$requiredFields = @(
    "component",
    "sourceUrl",
    "sourceRevision",
    "buildConfiguration",
    "artifactOrigin",
    "licenseFiles",
    "debugLibrary",
    "releaseLibrary",
    "debugLibrarySha256",
    "releaseLibrarySha256"
)

foreach ($field in $requiredFields) {
    if (-not $provenance.PSObject.Properties.Name.Contains($field)) {
        Fail "Skia provenance is missing required field: $field"
    }
    if ($null -eq $provenance.$field -or [string]::IsNullOrWhiteSpace([string]$provenance.$field)) {
        Fail "Skia provenance field is empty: $field"
    }
}

if ($provenance.component -ne "skia") {
    Fail "Skia provenance component must be 'skia'."
}

if ($provenance.sourceUrl -notmatch '^https://') {
    Fail "Skia provenance sourceUrl must be an HTTPS URL."
}

if ($provenance.sourceRevision -notmatch '^[0-9a-fA-F]{40}$') {
    Fail "Skia provenance sourceRevision must be a 40-character Git commit SHA."
}

foreach ($hashField in @("debugLibrarySha256", "releaseLibrarySha256")) {
    if ($provenance.$hashField -notmatch '^[0-9a-fA-F]{64}$') {
        Fail "Skia provenance $hashField must be a 64-character SHA256 hex digest."
    }
}

$licenseFiles = @($provenance.licenseFiles)
if ($licenseFiles.Count -eq 0) {
    Fail "Skia provenance licenseFiles must contain at least one path."
}

$hasSkiaLicensePath = $false
foreach ($licenseFile in $licenseFiles) {
    if ([string]::IsNullOrWhiteSpace([string]$licenseFile)) {
        Fail "Skia provenance licenseFiles contains an empty path."
    }
    if ([string]$licenseFile -match '(^|[\\/])third_party[\\/]skia[\\/]') {
        $hasSkiaLicensePath = $true
    }
}
if (-not $hasSkiaLicensePath) {
    Fail "Skia provenance licenseFiles must include at least one third_party/skia path."
}

if ($RequireLibraries) {
    $libraries = @(
        @{ Path = $provenance.debugLibrary; Sha256 = $provenance.debugLibrarySha256 },
        @{ Path = $provenance.releaseLibrary; Sha256 = $provenance.releaseLibrarySha256 }
    )
    foreach ($library in $libraries) {
        $libraryValue = [string]$library.Path
        $libraryPath = if ([System.IO.Path]::IsPathRooted($libraryValue)) {
            $libraryValue
        } else {
            Join-Path $projectRoot $libraryValue
        }
        if (-not (Test-Path -LiteralPath $libraryPath)) {
            Fail "Skia library listed in provenance was not found: $libraryPath"
        }

        $actualHash = (Get-FileHash -LiteralPath $libraryPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $expectedHash = ([string]$library.Sha256).ToLowerInvariant()
        if ($actualHash -ne $expectedHash) {
            Fail "Skia library SHA256 mismatch for ${libraryPath}: expected $expectedHash, got $actualHash"
        }
    }

    foreach ($licenseFile in $licenseFiles) {
        $licensePath = if ([System.IO.Path]::IsPathRooted([string]$licenseFile)) {
            [string]$licenseFile
        } else {
            Join-Path $projectRoot ([string]$licenseFile)
        }
        if (-not (Test-Path -LiteralPath $licensePath)) {
            Fail "Skia license file listed in provenance was not found: $licensePath"
        }
    }
}

Write-Host "Skia provenance OK: $provenancePath"
