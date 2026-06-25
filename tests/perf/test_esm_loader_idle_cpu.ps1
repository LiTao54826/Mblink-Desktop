param(
    [int]$SampleSeconds = 60,
    [int]$WarmupSeconds = 5,
    [double]$MaxRuntimeCpuPercentOneCore = 2.0,
    [double]$MaxDaemonCpuPercentOneCore = 1.0
)

$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Set-Location $repoRoot

$exe = Join-Path $repoRoot 'build/bin/Release/mblink-ui-dev.exe'
$project = Join-Path $repoRoot 'tmp/mblink_idle_cpu_probe/large_snapshot_app'
$fallbackProject = Join-Path $repoRoot 'tmp/mblink_ui_dev_responsiveness/large_snapshot_app'
$esbuildBin = Join-Path $repoRoot 'tmp/esbuild_tools/node_modules/.bin'
$resultDir = Join-Path $repoRoot 'tmp/perf'
$resultPath = Join-Path $resultDir 'esm_loader_idle_cpu.json'

if (!(Test-Path -LiteralPath $exe)) {
    throw "missing exe: $exe"
}

if (Test-Path -LiteralPath $esbuildBin) {
    $env:Path = "$esbuildBin;$env:Path"
}

if (!(Test-Path -LiteralPath $project)) {
    if (Test-Path -LiteralPath $fallbackProject) {
        $project = $fallbackProject
    } else {
        powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repoRoot 'tests/regression/test_mblink_ui_dev_responsiveness.ps1')
        if (Test-Path -LiteralPath $fallbackProject) {
            $project = $fallbackProject
        }
    }
}

if (!(Test-Path -LiteralPath $project)) {
    throw "missing large snapshot project: $project"
}

New-Item -ItemType Directory -Path $resultDir -Force | Out-Null

function Invoke-Json([string[]]$Arguments) {
    $raw = & $exe @Arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        throw "mblink-ui-dev $($Arguments -join ' ') failed`n$raw"
    }
    return $raw | ConvertFrom-Json
}

& $exe daemon stop --project $project *> $null

try {
    $open = Invoke-Json @('open', '--project', $project)
    if ($open.ok -ne $true) {
        throw 'open did not return ok=true'
    }

    Start-Sleep -Seconds $WarmupSeconds

    $info = Invoke-Json @('info', '--project', $project)
    if ($info.daemon.runtime_status -ne 'running') {
        throw "runtime is not running: $($info.daemon.runtime_status)"
    }

    $snapshot = Invoke-Json @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    if ($snapshot.ok -ne $true) {
        throw 'snapshot did not return ok=true'
    }
    if (!(Test-Path -LiteralPath $snapshot.snapshot.path)) {
        throw "snapshot file missing: $($snapshot.snapshot.path)"
    }
    if (!(Test-Path -LiteralPath $snapshot.screenshot.path)) {
        throw "screenshot file missing: $($snapshot.screenshot.path)"
    }

    Start-Sleep -Seconds $WarmupSeconds

    $ids = @([int]$info.daemon.pid, [int]$info.daemon.runtime_pid) | Where-Object { $_ -gt 0 }
    $before = Get-Process -Id $ids -ErrorAction Stop | Select-Object Id, ProcessName, CPU
    $startedAt = Get-Date
    Start-Sleep -Seconds $SampleSeconds
    $elapsed = ((Get-Date) - $startedAt).TotalSeconds
    $after = Get-Process -Id $ids -ErrorAction Stop | Select-Object Id, ProcessName, CPU

    $processes = @()
    foreach ($proc in $after) {
        $startProc = $before | Where-Object { $_.Id -eq $proc.Id } | Select-Object -First 1
        if ($null -eq $startProc) {
            continue
        }
        $delta = [double]($proc.CPU - $startProc.CPU)
        $role = if ($proc.Id -eq [int]$info.daemon.pid) { 'daemon' } elseif ($proc.Id -eq [int]$info.daemon.runtime_pid) { 'runtime' } else { 'unknown' }
        $processes += [pscustomobject]@{
            id = $proc.Id
            role = $role
            process_name = $proc.ProcessName
            cpu_seconds_delta = [Math]::Round($delta, 4)
            cpu_percent_one_core = [Math]::Round((100.0 * $delta / $elapsed), 3)
        }
    }

    $runtime = $processes | Where-Object { $_.role -eq 'runtime' } | Select-Object -First 1
    $daemon = $processes | Where-Object { $_.role -eq 'daemon' } | Select-Object -First 1
    if ($null -eq $runtime) {
        throw 'runtime process sample missing'
    }
    if ($null -eq $daemon) {
        throw 'daemon process sample missing'
    }

    $ok = ($runtime.cpu_percent_one_core -le $MaxRuntimeCpuPercentOneCore) -and ($daemon.cpu_percent_one_core -le $MaxDaemonCpuPercentOneCore)
    $result = [pscustomobject]@{
        ok = $ok
        project = $project
        sample_seconds = $SampleSeconds
        elapsed_seconds = [Math]::Round($elapsed, 3)
        max_runtime_cpu_percent_one_core = $MaxRuntimeCpuPercentOneCore
        max_daemon_cpu_percent_one_core = $MaxDaemonCpuPercentOneCore
        runtime_epoch = $info.daemon.runtime_epoch
        snapshot_path = $snapshot.snapshot.path
        screenshot_path = $snapshot.screenshot.path
        processes = $processes
    }

    $json = $result | ConvertTo-Json -Depth 8
    Set-Content -LiteralPath $resultPath -Value $json -Encoding UTF8
    Write-Host $json

    if (-not $ok) {
        throw "idle CPU evaluator failed; see $resultPath"
    }
}
finally {
    & $exe daemon stop --project $project *> $null
}
