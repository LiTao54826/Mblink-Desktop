$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$exe = Join-Path $repo 'build-tests\bin\Debug\esm_loader.exe'
$entry = Join-Path $PSScriptRoot 'app.js'
$tmp = Join-Path $repo 'tmp\ui_combinations_showcase'
$snapshot = Join-Path $tmp 'snapshot.json'
$screenshot = Join-Path $tmp 'screenshot.png'
$command = Join-Path $tmp 'command.json'
$response = Join-Path $tmp 'response.json'
$consoleFile = Join-Path $tmp 'console.json'
$errorsFile = Join-Path $tmp 'errors.json'
$lifecycleFile = Join-Path $tmp 'lifecycle.json'

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

function Send-UiDevCommand([hashtable]$payload, [int]$timeoutMs = 6000) {
    if (Test-Path -LiteralPath $response) {
        Remove-Item -LiteralPath $response -Force
    }
    Write-Utf8File $command (($payload | ConvertTo-Json -Compress -Depth 10))
    Wait-ForFile $response $timeoutMs
    return Read-JsonFile $response
}

Assert (Test-Path -LiteralPath $exe) "missing esm_loader: $exe"
if (Test-Path -LiteralPath $tmp) {
    Remove-Item -LiteralPath $tmp -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

$args = @(
    $entry,
    '--width', '1180',
    '--height', '760',
    '--title', 'ui_combinations_showcase_verify',
    '--ui-dev-runtime-epoch', 'ui-combinations-verify',
    '--ui-dev-snapshot-file', $snapshot,
    '--ui-dev-snapshot-include-screenshot',
    '--ui-dev-screenshot-file', $screenshot,
    '--ui-dev-command-file', $command,
    '--ui-dev-response-file', $response,
    '--ui-dev-console-file', $consoleFile,
    '--ui-dev-errors-file', $errorsFile,
    '--ui-dev-lifecycle-file', $lifecycleFile,
    '--quit', '6'
)

$psi = [System.Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $exe
$psi.Arguments = ($args | ForEach-Object { Quote-ProcessArgument $_ }) -join ' '
$psi.WorkingDirectory = $repo.Path
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true
$proc = [System.Diagnostics.Process]::new()
$proc.StartInfo = $psi
[void]$proc.Start()

$completed = $false
try {
    Wait-ForFile $snapshot
    $snap = Read-JsonFile $snapshot
    Assert ($snap.ok -eq $true) 'snapshot failed'
    Assert ($snap.node_count -gt 40) "snapshot too small: $($snap.node_count)"
    $snapshotText = Get-Content -Raw -LiteralPath $snapshot
    Assert ($snapshotText -like '*Operations console*') 'missing app title'
    Assert ($snapshotText -like '*Request editor*') 'missing request panel'
    Assert (Test-Path -LiteralPath $screenshot) 'screenshot not written'

    $root = Send-UiDevCommand @{ id = 'query-root'; type = 'query_element'; selector = '#combo-root' }
    Assert ($root.ok -eq $true) 'query root failed'
    Assert ($root.result.count -eq 1) 'root selector count mismatch'

    $table = Send-UiDevCommand @{ id = 'query-table'; type = 'query_element'; selector = '#queue-table' }
    Assert ($table.ok -eq $true) 'query table failed'
    Assert ($table.result.count -eq 1) 'table selector count mismatch'

    $compact = Send-UiDevCommand @{ id = 'click-compact'; type = 'click'; selector = '#density-compact' }
    Assert ($compact.ok -eq $true) 'compact click failed'

    $input = Send-UiDevCommand @{ id = 'input-request'; type = 'input_text'; selector = '#request-name'; text = 'API Gateway Review' }
    Assert ($input.ok -eq $true) 'input_text failed'
    Assert ($input.result.value -eq 'API Gateway Review') 'input value mismatch'

    $create = Send-UiDevCommand @{ id = 'click-create'; type = 'click'; selector = '#request-create' }
    Assert ($create.ok -eq $true) 'create click failed'

    $result = Send-UiDevCommand @{ id = 'inspect-result'; type = 'inspect'; selector = '#form-result' }
    Assert ($result.ok -eq $true) 'inspect result failed'
    Assert ($result.result.outer_html -like '*API Gateway Review*') 'form result did not update'

    $modalOpen = Send-UiDevCommand @{ id = 'click-modal-open'; type = 'click'; selector = '#open-settings-modal' }
    Assert ($modalOpen.ok -eq $true) 'modal open failed'
    $modal = Send-UiDevCommand @{ id = 'query-modal'; type = 'query_element'; selector = '#settings-dialog' }
    Assert ($modal.ok -eq $true) 'modal query failed'
    Assert ($modal.result.count -eq 1) 'modal not found'

    $scroll = Send-UiDevCommand @{ id = 'scroll-feed'; type = 'scroll'; selector = '#activity-feed'; y = 80 }
    Assert ($scroll.ok -eq $true) 'feed scroll failed'

    $errors = Read-JsonFile $errorsFile
    Assert (@($errors.errors).Count -eq 0) 'unexpected JS errors'

    Write-Host '[OK] ui_combinations_showcase snapshot/query/input/click/scroll checks passed'
    Write-Host "[OK] screenshot: $screenshot"
    $completed = $true
}
finally {
    if (-not $completed -and $proc -and -not $proc.HasExited) {
        $proc.Kill()
    }
}

if (-not $proc.WaitForExit(10000)) {
    $proc.Kill()
    throw 'esm_loader did not exit before timeout'
}
$stdout = $proc.StandardOutput.ReadToEnd()
$stderr = $proc.StandardError.ReadToEnd()
Assert ($proc.ExitCode -eq 0) "esm_loader failed exit=$($proc.ExitCode)`nstdout=$stdout`nstderr=$stderr"
