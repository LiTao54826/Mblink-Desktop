$ErrorActionPreference = 'Stop'

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Read-JsonFile([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing json file: $path"
    return Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
}

function Write-Utf8File([string]$path, [string]$content) {
    $parent = Split-Path -Parent $path
    if ($parent -and !(Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding UTF8
}

function Wait-ForFile([string]$path, [int]$timeoutMs = 6000) {
    $deadline = [DateTime]::UtcNow.AddMilliseconds($timeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        if (Test-Path -LiteralPath $path) { return }
        Start-Sleep -Milliseconds 25
    }
    throw "timed out waiting for $path"
}

function Quote-ProcessArgument([string]$value) {
    if ($null -eq $value -or $value.Length -eq 0) { return '""' }
    if ($value -notmatch '[\s"]') { return $value }
    return '"' + ($value -replace '\\(?=("|$))', '\\' -replace '"', '\"') + '"'
}

function Assert-CommandOk($result, [string]$label) {
    Assert ($result.ok -eq $true) "$label failed"
}

function Assert-ContainsOuterHtml($result, [string]$needle, [string]$label) {
    Assert-CommandOk $result $label
    Assert ($result.result.outer_html -like "*$needle*") "$label missing text: $needle"
}

function Get-PngEvidence([string]$path) {
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::FromFile($path)
    try {
        $colors = [System.Collections.Generic.HashSet[string]]::new()
        $sampleCount = 0
        $nonTransparent = 0
        $xSteps = 12
        $ySteps = 8
        for ($yi = 0; $yi -lt $ySteps; ++$yi) {
            $y = [Math]::Min($bitmap.Height - 1, [int][Math]::Round(($bitmap.Height - 1) * $yi / [Math]::Max(1, $ySteps - 1)))
            for ($xi = 0; $xi -lt $xSteps; ++$xi) {
                $x = [Math]::Min($bitmap.Width - 1, [int][Math]::Round(($bitmap.Width - 1) * $xi / [Math]::Max(1, $xSteps - 1)))
                $color = $bitmap.GetPixel($x, $y)
                ++$sampleCount
                if ($color.A -gt 0) { ++$nonTransparent }
                [void]$colors.Add("$($color.A),$($color.R),$($color.G),$($color.B)")
            }
        }
        return [ordered]@{
            width = $bitmap.Width
            height = $bitmap.Height
            sampled_pixels = $sampleCount
            nontransparent_samples = $nonTransparent
            unique_sampled_colors = $colors.Count
        }
    }
    finally {
        $bitmap.Dispose()
    }
}

function Get-PositiveMatchRect($queryResult, [string]$label) {
    Assert-CommandOk $queryResult $label
    Assert ($queryResult.result.count -ge 1) "$label selector not found"
    $match = @($queryResult.result.matches)[0]
    Assert ($null -ne $match -and $null -ne $match.rect) "$label missing rect"
    $rect = $match.rect
    Assert ([double]$rect.w -gt 0 -and [double]$rect.h -gt 0) "$label rect is empty: $($rect | ConvertTo-Json -Compress)"
    return [ordered]@{
        x = [double]$rect.x
        y = [double]$rect.y
        w = [double]$rect.w
        h = [double]$rect.h
    }
}
