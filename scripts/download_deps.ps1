# MBlink third-party dependency bootstrap for Windows.
#
# This script prepares the public fresh-clone dependencies that can be fetched
# safely from source. It intentionally does not fabricate the Skia layout,
# because the current CMake files expect prepared Debug/Release Skia libraries.

[CmdletBinding()]
param(
    [switch]$IncludeSkia,
    [switch]$NonInteractive,
    [switch]$ConfigureGlobalGitProxy,
    [switch]$Force,
    [string]$Proxy,
    [string]$QuickJsRef = "ca0d50dc2991c433f1c265239790e37b38c836b8",
    [string]$SdlRef = "b9c790949e4ff6bc26813b437d477da694269ef5"
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$ThirdPartyDir = Join-Path $ProjectRoot "third_party"

function Write-Step {
    param([string]$Message)
    Write-Host "[deps] $Message" -ForegroundColor Cyan
}

function Require-Command {
    param([string]$Command)
    if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) {
        throw "Required command not found: $Command"
    }
}

function Use-ProxyIfConfigured {
    $selectedProxy = $Proxy
    if (-not $selectedProxy) {
        $selectedProxy = $env:HTTPS_PROXY
    }
    if (-not $selectedProxy) {
        $selectedProxy = $env:HTTP_PROXY
    }

    if (-not $selectedProxy) {
        Write-Step "No proxy configured; using the current process environment."
        return
    }

    $env:HTTP_PROXY = $selectedProxy
    $env:HTTPS_PROXY = $selectedProxy
    $env:http_proxy = $selectedProxy
    $env:https_proxy = $selectedProxy
    Write-Step "Using proxy from parameter/environment: $selectedProxy"

    if ($ConfigureGlobalGitProxy) {
        git config --global http.proxy "$selectedProxy"
        git config --global https.proxy "$selectedProxy"
        Write-Step "Configured global Git proxy because -ConfigureGlobalGitProxy was passed."
    } else {
        Write-Step "Leaving global Git proxy untouched."
    }
}

function Assert-UnderThirdParty {
    param([string]$Path)

    $fullThirdParty = [System.IO.Path]::GetFullPath($ThirdPartyDir)
    $fullPath = [System.IO.Path]::GetFullPath($Path)

    if (-not $fullPath.StartsWith($fullThirdParty, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify path outside third_party: $fullPath"
    }
}

function Remove-DependencyDirectory {
    param([string]$Path)

    Assert-UnderThirdParty -Path $Path
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

function Invoke-Git {
    param([string[]]$Arguments)

    & git @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') failed with exit code $LASTEXITCODE"
    }
}

function Get-CurrentGitRef {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath (Join-Path $Path ".git"))) {
        return $null
    }

    $current = & git -C $Path rev-parse HEAD 2>$null
    if ($LASTEXITCODE -ne 0) {
        return $null
    }
    return $current.Trim()
}

function Clone-OrVerifyDependency {
    param(
        [string]$Name,
        [string]$Url,
        [string]$TargetName,
        [string]$Ref
    )

    $targetPath = Join-Path $ThirdPartyDir $TargetName
    Write-Step "$Name -> third_party\$TargetName"

    if (Test-Path -LiteralPath $targetPath) {
        $currentRef = Get-CurrentGitRef -Path $targetPath
        if ($currentRef -and $currentRef.Equals($Ref, [System.StringComparison]::OrdinalIgnoreCase)) {
            Write-Host "  OK: already at $Ref" -ForegroundColor Green
            return
        }

        if (-not $Force) {
            Write-Host "  SKIP: directory already exists." -ForegroundColor Yellow
            if ($currentRef) {
                Write-Host "        current ref: $currentRef" -ForegroundColor Yellow
                Write-Host "        expected ref: $Ref" -ForegroundColor Yellow
            }
            Write-Host "        pass -Force to replace it." -ForegroundColor Yellow
            return
        }

        Write-Host "  replacing existing directory because -Force was passed" -ForegroundColor Yellow
        Remove-DependencyDirectory -Path $targetPath
    }

    Invoke-Git -Arguments @("clone", $Url, $targetPath)
    Invoke-Git -Arguments @("-C", $targetPath, "checkout", "--detach", $Ref)
    Write-Host "  OK: checked out $Ref" -ForegroundColor Green
}

function Test-SkiaLayout {
    $debugLib = Join-Path $ThirdPartyDir "skia\Debug\out\Debug-windows-x64\skia.lib"
    $releaseLib = Join-Path $ThirdPartyDir "skia\Release\out\Release-windows-x64\skia.lib"
    return (Test-Path -LiteralPath $debugLib) -and (Test-Path -LiteralPath $releaseLib)
}

function Report-SkiaState {
    if (Test-SkiaLayout) {
        Write-Host "  OK: prepared Skia Debug/Release libraries were found." -ForegroundColor Green
        return
    }

    $message = @"
  Skia is still required by the default MBlink build.
  Current CMake expects prepared libraries under:
    third_party\skia\Debug\out\Debug-windows-x64\skia.lib
    third_party\skia\Release\out\Release-windows-x64\skia.lib

  This script does not download a random Skia source/prebuilt tree, because that
  would not match the layout CMake consumes today. Prepare Skia separately and
  record provenance before publishing release artifacts. See docs\BUILD.md,
  docs\RELEASE.md, and THIRD_PARTY_NOTICES.md.
"@

    if ($IncludeSkia) {
        throw $message
    }

    Write-Host $message -ForegroundColor Yellow
}

Write-Host "MBlink dependency bootstrap" -ForegroundColor Cyan
Write-Host "Project root: $ProjectRoot"
Write-Host "Third-party:  $ThirdPartyDir"
if ($NonInteractive) {
    Write-Host "Mode:         non-interactive"
}
Write-Host ""

Require-Command "git"
Use-ProxyIfConfigured

if (-not (Test-Path -LiteralPath $ThirdPartyDir)) {
    New-Item -ItemType Directory -Path $ThirdPartyDir | Out-Null
}

Clone-OrVerifyDependency `
    -Name "QuickJS-ng" `
    -Url "https://github.com/quickjs-ng/quickjs.git" `
    -TargetName "quickjs" `
    -Ref $QuickJsRef

Clone-OrVerifyDependency `
    -Name "SDL3" `
    -Url "https://github.com/libsdl-org/SDL.git" `
    -TargetName "SDL3" `
    -Ref $SdlRef

Write-Step "Skia"
Report-SkiaState

Write-Host ""
Write-Host "Dependency bootstrap finished." -ForegroundColor Green
Write-Host "Next: prepare Skia if needed, then run cmake from the repository root." -ForegroundColor Cyan
