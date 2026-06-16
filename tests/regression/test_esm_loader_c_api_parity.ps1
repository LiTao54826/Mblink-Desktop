$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

Set-Location (Join-Path $PSScriptRoot '..\..')

$exe = Join-Path (Get-Location) 'build/bin/Release/esm_loader.exe'
$dll = Join-Path (Get-Location) 'build/bin/Release/mbink.dll'
$tmpRoot = Join-Path (Get-Location) 'tmp/esm_loader_c_api_parity'
$mainRoot = Join-Path $tmpRoot 'main'
$noScriptRoot = Join-Path $tmpRoot 'no_scripts'

if (!(Test-Path $exe)) { throw "missing exe: $exe" }
if (!(Test-Path $dll)) { throw "missing dll: $dll" }

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Write-Utf8File([string]$path, [string]$content) {
    $parent = Split-Path -Parent $path
    if ($parent -and !(Test-Path $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding UTF8
}

function Read-JsonFile([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing json file: $path"
    return Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
}

function Find-Dumpbin {
    $cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $roots = @(
        (Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio'),
        (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio')
    ) | Where-Object { $_ -and (Test-Path $_) }

    $matches = foreach ($root in $roots) {
        Get-ChildItem -Path $root -Recurse -Filter dumpbin.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -like '*\HostX64\x64\dumpbin.exe' }
    }

    return @($matches | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName)
}

function Start-Loader([string]$entry, [string]$root, [double]$quitSeconds, [switch]$NoScripts) {
    $snapshot = Join-Path $root 'snapshot.json'
    $console = Join-Path $root 'console.json'
    $errors = Join-Path $root 'errors.json'
    $lifecycle = Join-Path $root 'lifecycle.json'
    $command = Join-Path $root 'command.json'
    $response = Join-Path $root 'response.json'
    $epoch = "epoch-$([System.IO.Path]::GetFileName($root))"

    $args = @(
        $entry,
        '--width', '640',
        '--height', '480',
        '--title', "esm-loader-$([System.IO.Path]::GetFileName($root))",
        '--ui-dev-runtime-epoch', $epoch,
        '--ui-dev-snapshot-file', $snapshot,
        '--ui-dev-command-file', $command,
        '--ui-dev-response-file', $response,
        '--ui-dev-console-file', $console,
        '--ui-dev-errors-file', $errors,
        '--ui-dev-lifecycle-file', $lifecycle,
        '--quit', ([string]$quitSeconds)
    )
    if ($NoScripts) {
        $args = @($entry, '--no-scripts') + $args[1..($args.Length - 1)]
    }

    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $exe
    $psi.Arguments = ($args | ForEach-Object { Quote-ProcessArgument $_ }) -join ' '
    $psi.WorkingDirectory = (Get-Location).Path
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    return [pscustomobject]@{
        Process = $proc
        Snapshot = $snapshot
        Console = $console
        Errors = $errors
        Lifecycle = $lifecycle
        Command = $command
        Response = $response
        Epoch = $epoch
    }
}

function Wait-ForFile([string]$path, [int]$timeoutMs = 5000) {
    $deadline = [DateTime]::UtcNow.AddMilliseconds($timeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        if (Test-Path -LiteralPath $path) { return }
        Start-Sleep -Milliseconds 25
    }
    throw "timed out waiting for $path"
}

function Quote-ProcessArgument([string]$value) {
    if ($null -eq $value) { return '""' }
    if ($value.Length -eq 0) { return '""' }
    if ($value -notmatch '[\s"]') { return $value }
    return '"' + ($value -replace '\\(?=("|$))', '\\' -replace '"', '\"') + '"'
}

function Send-UiDevCommand([object]$runtime, [hashtable]$payload, [int]$timeoutMs = 5000) {
    if (Test-Path -LiteralPath $runtime.Response) {
        Remove-Item -LiteralPath $runtime.Response -Force
    }
    Write-Utf8File $runtime.Command (($payload | ConvertTo-Json -Compress -Depth 10))
    Wait-ForFile $runtime.Response $timeoutMs
    return Read-JsonFile $runtime.Response
}

if (Test-Path $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $mainRoot, $noScriptRoot | Out-Null

Write-Utf8File (Join-Path $mainRoot 'index.html') @'
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>C API parity fixture</title>
  <style>
    body { margin: 0; font-family: Segoe UI, sans-serif; background: #f7fafc; color: #102033; }
    #root { padding: 20px; }
    button, input { padding: 8px 12px; }
  </style>
</head>
<body>
  <main id="root">
    <h1>C API parity fixture</h1>
    <input id="field" value="before">
    <button id="action" onclick="document.getElementById('status').textContent = document.getElementById('field').value">Ready</button>
    <p id="status">idle</p>
  </main>
  <script>
    console.log('esm-loader-c-api-parity');
    document.getElementById('action').textContent = 'Loaded via script';
  </script>
</body>
</html>
'@

Write-Utf8File (Join-Path $noScriptRoot 'index.html') @'
<!doctype html>
<html>
<body>
  <h1 id="title">No scripts fixture</h1>
  <p id="status">script did not run</p>
  <script>
    console.log('no-scripts-user-log');
    document.getElementById('status').textContent = 'script ran';
  </script>
</body>
</html>
'@

$dumpbin = Find-Dumpbin
if ($dumpbin) {
    $dependents = & $dumpbin /DEPENDENTS $exe 2>&1 | Out-String
    Assert ($dependents -like '*mbink.dll*') 'esm_loader does not depend on mbink.dll'
    Assert ($dependents -notlike '*SDL3*') 'esm_loader unexpectedly links SDL directly'
    Assert ($dependents -notlike '*skia*') 'esm_loader unexpectedly links Skia directly'
    Write-Host '[OK]  esm_loader depends on mbink.dll'
} else {
    Write-Host '[SKIP] dumpbin not found; dependency check skipped'
}

$loaderSize = (Get-Item -LiteralPath $exe).Length
Assert ($loaderSize -lt 1048576) "esm_loader.exe is expected to stay thin, size=$loaderSize"
Write-Host "[OK]  esm_loader thin size: $loaderSize bytes"

$main = Start-Loader -entry (Join-Path $mainRoot 'index.html') -root $mainRoot -quitSeconds 5.0
$mainCompleted = $false
try {
    Wait-ForFile $main.Snapshot
    $snapshot = Read-JsonFile $main.Snapshot
    Assert ($snapshot.ok -eq $true) 'snapshot not ok'
    Assert ($snapshot.runtime_epoch -eq $main.Epoch) 'snapshot runtime_epoch mismatch'
    Assert ($snapshot.tree.tag -eq 'body') 'snapshot root should be body'
    Assert ($snapshot.node_count -gt 1) 'snapshot should contain real DOM nodes'
    Assert ((Get-Content -Raw -LiteralPath $main.Snapshot) -like '*C API parity fixture*') 'snapshot missing fixture text'

    $query = Send-UiDevCommand $main @{ id = 'query-field'; type = 'query_element'; selector = '#field' }
    Assert ($query.ok -eq $true) 'query command not ok'
    Assert ($query.result.count -eq 1) 'query command did not find #field'

    $input = Send-UiDevCommand $main @{ id = 'input-field'; type = 'input_text'; selector = '#field'; text = 'from c api command' }
    Assert ($input.ok -eq $true) 'input_text command not ok'
    Assert ($input.result.value -eq 'from c api command') 'input_text did not update value'

    $click = Send-UiDevCommand $main @{ id = 'click-action'; type = 'click'; selector = '#action' }
    Assert ($click.ok -eq $true) 'click command not ok'
    Assert ($click.result.clicked -eq $true) 'click command did not click'

    $inspect = Send-UiDevCommand $main @{ id = 'inspect-status'; type = 'inspect'; selector = '#status' }
    Assert ($inspect.ok -eq $true) 'inspect command not ok'
    Assert ($inspect.result.outer_html -like '*from c api command*') 'click result did not update DOM'

    $console = Read-JsonFile $main.Console
    Assert ((Get-Content -Raw -LiteralPath $main.Console) -like '*esm-loader-c-api-parity*') 'console log missing'

    $errors = Read-JsonFile $main.Errors
    Assert (@($errors.errors).Count -eq 0) 'unexpected JS errors'

    $lifecycle = Read-JsonFile $main.Lifecycle
    Assert ($lifecycle.ok -eq $true) 'lifecycle not ok'
    Assert ($lifecycle.runtime_epoch -eq $main.Epoch) 'lifecycle runtime_epoch mismatch'
    Write-Host '[OK]  esm_loader C API snapshot/observe/command path'
    $mainCompleted = $true
}
finally {
    if (-not $mainCompleted -and $main.Process -and -not $main.Process.HasExited) {
        $main.Process.Kill()
    }
}

if (-not $main.Process.WaitForExit(10000)) {
    $main.Process.Kill()
    throw 'esm_loader main fixture did not exit before timeout'
}
$stdout = $main.Process.StandardOutput.ReadToEnd()
$stderr = $main.Process.StandardError.ReadToEnd()
Assert ($main.Process.ExitCode -eq 0) "esm_loader main fixture failed exit=$($main.Process.ExitCode)`nstdout=$stdout`nstderr=$stderr"

$noScripts = Start-Loader -entry (Join-Path $noScriptRoot 'index.html') -root $noScriptRoot -quitSeconds 0.2 -NoScripts
if (-not $noScripts.Process.WaitForExit(5000)) {
    $noScripts.Process.Kill()
    throw 'esm_loader --no-scripts fixture did not exit before timeout'
}
$noScriptsStdout = $noScripts.Process.StandardOutput.ReadToEnd()
$noScriptsStderr = $noScripts.Process.StandardError.ReadToEnd()
Assert ($noScripts.Process.ExitCode -eq 0) "esm_loader --no-scripts failed exit=$($noScripts.Process.ExitCode)`nstdout=$noScriptsStdout`nstderr=$noScriptsStderr"
$noScriptsSnapshot = Read-JsonFile $noScripts.Snapshot
Assert ($noScriptsSnapshot.ok -eq $true) '--no-scripts snapshot not ok'
Assert ((Get-Content -Raw -LiteralPath $noScripts.Snapshot) -like '*script did not run*') '--no-scripts snapshot changed scripted text'
Assert ((Get-Content -Raw -LiteralPath $noScripts.Snapshot) -notlike '*script ran*') '--no-scripts executed inline script'
Assert ((Get-Content -Raw -LiteralPath $noScripts.Console) -notlike '*no-scripts-user-log*') '--no-scripts emitted user script log'
Write-Host '[OK]  esm_loader --no-scripts blocks page scripts'

Write-Host '[PASS] esm_loader C API parity regression green'
