$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

Set-Location (Join-Path $PSScriptRoot '..\..')

$exe = Join-Path (Get-Location) 'build/bin/Release/mbink-ui-dev.exe'
$tmpRoot = Join-Path (Get-Location) 'tmp/mbink_ui_dev_responsiveness'
$openProject = Join-Path $tmpRoot 'open_idempotent_app'
$largeProject = Join-Path $tmpRoot 'large_snapshot_app'
$largeUiDir = Join-Path $largeProject 'ui'
$largeUiApp = Join-Path $largeUiDir 'app.js'

if (!(Test-Path $exe)) { throw "missing exe: $exe" }

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Invoke-JsonCommand([string]$name, [string[]]$arguments, [scriptblock]$assert) {
    Write-Host "[RUN] $name"
    $raw = & $exe @arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        throw "[$name] failed`n$raw"
    }
    $json = $raw | ConvertFrom-Json
    & $assert $json
    Write-Host "[OK]  $name"
    return $json
}

function Invoke-JsonCommandExpectFailure([string]$name, [string[]]$arguments, [scriptblock]$assert) {
    Write-Host "[RUN] $name"
    $raw = & $exe @arguments 2>&1 | Out-String
    $json = $raw | ConvertFrom-Json
    Assert ($LASTEXITCODE -ne 0 -or $json.ok -eq $false) "[$name] unexpectedly succeeded"
    & $assert $json
    Write-Host "[OK]  $name"
    return $json
}

function Get-RuntimePid([object]$json) {
    if ($null -ne $json.daemon -and $null -ne $json.daemon.runtime_pid) {
        return [int]$json.daemon.runtime_pid
    }
    if ($null -ne $json.runtime_pid) {
        return [int]$json.runtime_pid
    }
    throw 'runtime_pid missing from response'
}

function Get-Epoch([object]$json) {
    foreach ($path in @(
        @('daemon', 'runtime_epoch'),
        @('runtime_epoch'),
        @('snapshot', 'epoch'),
        @('epoch')
    )) {
        $value = $json
        $found = $true
        foreach ($segment in $path) {
            if ($value -is [System.Collections.IDictionary]) {
                if (-not $value.Contains($segment)) {
                    $found = $false
                    break
                }
                $value = $value[$segment]
                continue
            }
            $prop = $value.PSObject.Properties[$segment]
            if ($null -eq $prop) {
                $found = $false
                break
            }
            $value = $prop.Value
        }
        if ($found -and $null -ne $value -and "$value" -ne '') {
            return "$value"
        }
    }
    return $null
}

function Write-LargeUiFixture([string]$path, [int]$count) {
    if (!(Test-Path $largeUiDir)) {
        New-Item -ItemType Directory -Path $largeUiDir -Force | Out-Null
    }
    $cells = 0..($count - 1) | ForEach-Object {
        "    h('div', { class: 'cell', 'data-index': '$($_)' }, 'Cell $($_)')"
    }
    $body = @(
        "import { h, render } from 'preact';"
        ""
        "const cells = ["
        ($cells -join ",`n")
        "];"
        ""
        "function App() {"
        "  return h('main', { id: 'grid-root', style: { padding: '12px', display: 'grid', gridTemplateColumns: 'repeat(5, minmax(0, 1fr))', gap: '4px' } }, cells);"
        "}"
        ""
        "render(h(App), document.body);"
    ) -join "`n"
    Set-Content -Path $path -Value $body -NoNewline
}

if (Test-Path $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $tmpRoot -Force | Out-Null

& $exe daemon stop --project $openProject *> $null
& $exe daemon stop --project $largeProject *> $null

Invoke-JsonCommand 'init open-idempotent fixture' @('init', $openProject, '--purpose', 'minimal', '--runtime', 'tool') {
    param($j)
    Assert ($j.ok -eq $true -and $j.canonical_key -eq 'minimal/tool') 'failed to init open-idempotent fixture'
}

Invoke-JsonCommand 'init large snapshot fixture' @('init', $largeProject, '--purpose', 'minimal', '--runtime', 'tool') {
    param($j)
    Assert ($j.ok -eq $true -and $j.canonical_key -eq 'minimal/tool') 'failed to init large snapshot fixture'
}

Write-LargeUiFixture -path $largeUiApp -count 2500

try {
    $open1 = Invoke-JsonCommand 'open same project first time' @('open', $openProject) {
        param($j)
        Assert ($j.ok -eq $true) 'initial open failed'
    }
    $info1 = Invoke-JsonCommand 'info after initial open' @('info', '--project', $openProject) {
        param($j)
        Assert ($j.ok -eq $true) 'info after initial open failed'
        Assert ($j.daemon.runtime_status -eq 'running') 'runtime not running after initial open'
    }
    $pid1 = Get-RuntimePid $info1

    $open2 = Invoke-JsonCommand 'open same project second time without force' @('open', $openProject) {
        param($j)
        Assert ($j.ok -eq $true) 'second open failed'
    }
    $pid2 = Get-RuntimePid $open2
    Assert ($pid2 -eq $pid1) 'open without --force restarted the runtime'

    $openForced = Invoke-JsonCommand 'open same project with force' @('open', $openProject, '--force') {
        param($j)
        Assert ($j.ok -eq $true) 'forced open failed'
    }
    $pidForced = Get-RuntimePid $openForced
    Assert ($pidForced -ne $pid1) 'open --force did not restart the runtime'

    $snapshotBeforeReload = Invoke-JsonCommand 'snapshot before reload' @('snapshot', '--project', $openProject, '--response', 'file') {
        param($j)
        Assert ($j.ok -eq $true) 'snapshot before reload failed'
        Assert ($j.response_mode -eq 'file') 'snapshot before reload should use file mode'
        Assert ((Test-Path -LiteralPath $j.snapshot.path) -and $j.snapshot.bytes -gt 0) 'snapshot before reload path missing'
    }
    $oldEpoch = Get-Epoch $snapshotBeforeReload
    Assert ($null -ne $oldEpoch) 'snapshot before reload did not expose an epoch'

    $reload = Invoke-JsonCommand 'reload project' @('reload', '--project', $openProject) {
        param($j)
        Assert ($j.ok -eq $true) 'reload failed'
    }

    $snapshotAfterReloadRaw = & $exe snapshot --project $openProject --response file 2>&1 | Out-String
    $snapshotAfterReload = $snapshotAfterReloadRaw | ConvertFrom-Json
    if ($snapshotAfterReload.ok -eq $true) {
        Assert ($snapshotAfterReload.response_mode -eq 'file') 'snapshot after reload should use file mode'
        $newEpoch = Get-Epoch $snapshotAfterReload
        Assert ($null -ne $newEpoch) 'snapshot after reload did not expose an epoch'
        Assert ($newEpoch -ne $oldEpoch) 'snapshot after reload reused the old epoch'
        Write-Host '[OK]  snapshot after reload returned a fresh epoch'
    } else {
        $code = $snapshotAfterReload.error.code
        Assert (@('snapshot_not_ready', 'snapshot_stale') -contains $code) "unexpected snapshot error after reload: $code"
        Write-Host "[OK]  snapshot after reload rejected stale state with $code"
    }

    $largeOpen = Invoke-JsonCommand 'open large snapshot project' @('open', $largeProject) {
        param($j)
        Assert ($j.ok -eq $true) 'open large snapshot project failed'
    }

    $largeSnapshot = Invoke-JsonCommand 'large snapshot auto response' @('snapshot', '--project', $largeProject) {
        param($j)
        Assert ($j.ok -eq $true) 'large snapshot failed'
        if ($j.response_mode -eq 'file') {
            Assert ((Test-Path -LiteralPath $j.snapshot.path) -and $j.snapshot.bytes -gt 0) 'large snapshot file response path missing'
        } else {
            Assert ($j.response_mode -eq 'inline') 'large snapshot response_mode must be inline or file'
            Assert ($null -ne $j.tree) 'large snapshot inline tree missing'
        }
        if ($j.PSObject.Properties.Name -contains 'truncated') {
            Assert (($j.PSObject.Properties.Name -contains 'node_count') -or ($j.snapshot.PSObject.Properties.Name -contains 'node_count')) 'truncated snapshot missing node_count'
        }
    }

    $largeSnapshotFile = Invoke-JsonCommand 'large snapshot explicit file response' @('snapshot', '--project', $largeProject, '--response', 'file') {
        param($j)
        Assert ($j.ok -eq $true) 'large snapshot file response failed'
        Assert ($j.response_mode -eq 'file') 'large snapshot explicit file response did not use file mode'
        Assert ((Test-Path -LiteralPath $j.snapshot.path) -and $j.snapshot.bytes -gt 0) 'large snapshot explicit file path missing'
    }
    $snapshotJson = Get-Content -Raw -LiteralPath $largeSnapshotFile.snapshot.path | ConvertFrom-Json
    Assert ($snapshotJson.ok -eq $true) 'large snapshot file payload not ok'

    $rootSnapshot = Invoke-JsonCommand 'root selector bounded snapshot' @('snapshot', '--project', $largeProject, '--response', 'inline', '--root-selector', '#grid-root', '--max-nodes', '20') {
        param($j)
        Assert ($j.ok -eq $true) 'root selector snapshot failed'
        Assert ($j.response_mode -eq 'inline') 'root selector snapshot should be inline'
        Assert ($j.root_selector -eq '#grid-root') 'root selector metadata missing'
        Assert ($j.limits.max_nodes -eq 20) 'root selector snapshot ignored max_nodes'
        Assert ($j.tree.tag -eq 'main') 'root selector snapshot did not start at main'
        Assert ($j.tree.attrs.id -eq 'grid-root') 'root selector snapshot did not start at #grid-root'
        Assert ($j.node_count -le 20) 'root selector snapshot exceeded max_nodes'
    }

    $largeQueryRaw = & $exe query --project $largeProject '.cell' 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0) {
        $largeQuery = $largeQueryRaw | ConvertFrom-Json
        Assert ($largeQuery.ok -eq $true) 'large query reported success but ok=false'
        Assert ($largeQuery.result.count -ge 2500) 'large query count did not include all cells'
        if ($largeQuery.result.PSObject.Properties.Name -contains 'matches') {
            Assert (@($largeQuery.result.matches).Count -le $largeQuery.result.count) 'large query returned more summaries than matches'
            if (@($largeQuery.result.matches).Count -lt $largeQuery.result.count) {
                Write-Host '[OK]  large query returned bounded summaries'
            } else {
                Write-Host '[OK]  large query returned all summaries'
            }
        } else {
            Write-Host '[OK]  large query returned bounded metadata without inline matches'
        }
    } else {
        $largeQuery = $largeQueryRaw | ConvertFrom-Json
        $queryErrorCode = $largeQuery.error.code
        Assert (@('runtime_command_timeout', 'daemon_response_timeout') -contains $queryErrorCode) "unexpected large query error code: $queryErrorCode"
        Write-Host "[OK]  large query failed with structured code $queryErrorCode"
    }

    Invoke-JsonCommandExpectFailure 'snapshot invalid response mode' @('snapshot', '--project', $openProject, '--response', 'bogus') {
        param($j)
        Assert ($j.ok -eq $false) 'invalid response mode should fail'
        Assert ($j.error.code -eq 'invalid_args') 'invalid response mode should return invalid_args'
    }

    Invoke-JsonCommandExpectFailure 'query missing selector' @('query', '--project', $openProject) {
        param($j)
        Assert ($j.ok -eq $false) 'query without selector should fail'
        Assert ($j.error.code -eq 'invalid_args') 'query without selector should return invalid_args'
    }
}
finally {
    & $exe daemon stop --project $openProject *> $null
    & $exe daemon stop --project $largeProject *> $null
}

Write-Host '[PASS] mbink-ui-dev responsiveness regression green'
