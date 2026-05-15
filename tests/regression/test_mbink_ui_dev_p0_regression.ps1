$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
Set-Location (Join-Path $PSScriptRoot '..\..')
$exe = Join-Path (Get-Location) 'build/bin/Release/mbink-ui-dev.exe'
$todo = Join-Path (Get-Location) 'examples/todo_app_js'
$canonicalDev = Join-Path (Get-Location) 'tmp/mbink_ui_dev_p0_canonical_template'
$projA = Join-Path (Get-Location) 'tmp/mbink_ui_dev_multi_verify_a'
$projB = Join-Path (Get-Location) 'tmp/mbink_ui_dev_multi_verify_b'
$tmpJs = Join-Path (Get-Location) 'tmp/mbink_ui_dev_eval_test.js'
if (!(Test-Path $exe)) { throw "missing exe: $exe" }
if (!(Test-Path $projA) -or !(Test-Path $projB)) { throw 'missing multi-project verify fixtures' }
if (Test-Path $canonicalDev) { Remove-Item -LiteralPath $canonicalDev -Recurse -Force }
Set-Content -Path $tmpJs -Value '1+2' -NoNewline

function Invoke-JsonCommand([string]$name, [string[]]$arguments, [scriptblock]$assert) {
    Write-Host "[RUN] $name"
    $raw = & $exe @arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw "[$name] failed`n$raw" }
    $json = $raw | ConvertFrom-Json
    & $assert $json
    Write-Host "[OK]  $name"
}

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Start-ServeProcess() {
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $exe
    $psi.Arguments = "serve --project `"$todo`""
    $psi.UseShellExecute = $false
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    return $proc
}

function Invoke-Rpc([System.Diagnostics.Process]$proc, [hashtable]$payload) {
    $id = $payload.id
    $proc.StandardInput.WriteLine(($payload | ConvertTo-Json -Compress -Depth 10))
    $deadline = (Get-Date).AddSeconds(15)
    while ((Get-Date) -lt $deadline) {
        $line = $proc.StandardOutput.ReadLine()
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $msg = $line | ConvertFrom-Json
        if ($msg.id -eq $id) { return $msg }
    }
    throw "rpc timeout for id=$id"
}

& $exe daemon stop --project $projA *> $null
& $exe daemon stop --project $projB *> $null
& $exe daemon stop --project $canonicalDev *> $null

Invoke-JsonCommand 'open todo' @('open', $todo) {
    param($j)
    Assert ($j.ok -eq $true) 'open todo not ok'
}
Invoke-JsonCommand 'snapshot cold start' @('snapshot', '--project', $todo) {
    param($j)
    Assert ($j.ok -eq $true) 'snapshot not ok'
    Assert ($j.tree.tag -ne 'stub-root') 'snapshot returned stub-root'
    Assert ($j.viewport.width -eq 800 -and $j.viewport.height -eq 600) 'viewport size mismatch'
    Assert ($j.response_mode -eq 'inline') 'snapshot default response was not inline for small DOM'
    Assert ((Test-Path -LiteralPath $j.snapshot.path) -and $j.snapshot.bytes -gt 0) 'snapshot metadata path missing'
}
Invoke-JsonCommand 'snapshot file response' @('snapshot', '--project', $todo, '--response', 'file') {
    param($j)
    Assert ($j.ok -eq $true) 'snapshot file response not ok'
    Assert ($j.response_mode -eq 'file') 'snapshot file response did not use file mode'
    Assert ((Test-Path -LiteralPath $j.snapshot.path) -and $j.snapshot.bytes -gt 0) 'snapshot file response path missing'
    Assert (-not ($j.PSObject.Properties.Name -contains 'tree')) 'snapshot file response should not inline tree'
}
Invoke-JsonCommand 'query with --project first' @('query', '--project', $todo, '#todo-input') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.count -ge 1) 'query did not find #todo-input'
}
Invoke-JsonCommand 'inspect with --project first' @('inspect', '--project', $todo, 'button[type="submit"]') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.found -eq $true) 'inspect failed'
}
Invoke-JsonCommand 'input-text' @('input-text', '--project', $todo, '#todo-input', 'task from regression') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.value -eq 'task from regression') 'input-text failed'
}
Invoke-JsonCommand 'click submit' @('click', '--project', $todo, 'button[type="submit"]') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.clicked -eq $true) 'click failed'
}
Invoke-JsonCommand 'query updated result' @('query', '--project', $todo, 'span') {
    param($j)
    $texts = @($j.result.matches | ForEach-Object { $_.text })
    Assert ($texts -contains 'task from regression') 'post-click query result is stale'
}

Invoke-JsonCommand 'init canonical desktop template' @('init', $canonicalDev, '--purpose', 'desktop-app', '--runtime', 'tool') {
    param($j)
    Assert ($j.ok -eq $true -and $j.canonical_key -eq 'desktop-app/tool') 'init canonical desktop template not ok'
    Assert (($j.files_created -contains 'ui/app.js') -and ($j.files_created -contains 'ui/bridge.js')) 'canonical template missing UI bridge files'
}
Invoke-JsonCommand 'open canonical desktop template' @('open', '--project', $canonicalDev) {
    param($j)
    Assert ($j.ok -eq $true) 'open canonical desktop template not ok'
}
Invoke-JsonCommand 'snapshot canonical desktop template' @('snapshot', '--project', $canonicalDev) {
    param($j)
    Assert ($j.ok -eq $true) 'snapshot canonical desktop template not ok'
    Assert ($j.tree.tag -ne 'stub-root') 'canonical desktop template snapshot returned stub-root'
    Assert ($j.tree.rect.h -eq $j.viewport.height) 'canonical body height differs from viewport'
}
Invoke-JsonCommand 'query canonical validation input' @('query', '--project', $canonicalDev, '#validation-input') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.count -eq 1) 'canonical validation input missing'
}
Invoke-JsonCommand 'click canonical counter' @('click', '--project', $canonicalDev, '#counter-button') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.clicked -eq $true -and $j.result.element.text -like '*1*') 'canonical counter click failed'
}
Invoke-JsonCommand 'input-text canonical validation' @('input-text', '--project', $canonicalDev, '#validation-input', 'task from canonical regression') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.value -eq 'task from canonical regression') 'canonical input-text failed'
}
Invoke-JsonCommand 'submit canonical validation' @('click', '--project', $canonicalDev, '#submit-button') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.clicked -eq $true) 'canonical submit click failed'
}
Invoke-JsonCommand 'inspect canonical result' @('inspect', '--project', $canonicalDev, '#result-text') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.outer_html -like '*task from canonical regression*') 'canonical result did not update'
}
Invoke-JsonCommand 'eval canonical body height' @('eval', '--project', $canonicalDev, "JSON.stringify({bodyScrollHeight:document.body.scrollHeight,innerHeight:window.innerHeight,input:document.getElementById('validation-input').value})") {
    param($j)
    $state = $j.result | ConvertFrom-Json
    Assert ([double]$state.bodyScrollHeight -le ([double]$state.innerHeight + 1)) 'canonical body overflows viewport'
    Assert ($state.input -eq 'task from canonical regression') 'canonical input value not retained'
}

Invoke-JsonCommand 'highlight' @('highlight', '--project', $todo, '#todo-input', '--color', '#ff4d4f') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.highlighted -eq $true) 'highlight failed'
}
Invoke-JsonCommand 'scroll body' @('scroll', '--project', $todo, 'body', '--y', '50') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result.selector -eq 'body') 'scroll failed'
}
Invoke-JsonCommand 'eval base64' @('eval', '--project', $todo, '--code-base64', 'MSsx') {
    param($j)
    Assert ($j.ok -eq $true -and $j.result -eq 2) 'eval --code-base64 failed'
}
Invoke-JsonCommand 'eval from file' @('eval', '--project', $todo, '--from', $tmpJs) {
    param($j)
    Assert ($j.ok -eq $true -and $j.result -eq 3) 'eval --from failed'
}

Write-Host '[RUN] serve open_project cold switch A/B'
$serve = Start-ServeProcess
try {
    $init = Invoke-Rpc $serve @{ jsonrpc = '2.0'; id = 1; method = 'initialize'; params = @{} }
    Assert ($init.result.serverInfo.name -eq 'mbink-ui-dev') 'initialize failed'
    $openA = Invoke-Rpc $serve @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'open_project'; arguments = @{ path = $projA } } }
    Assert ($openA.result.isError -eq $false) 'open_project A failed'
    Assert ($openA.result.structuredContent.ok -eq $true) 'open_project A not ok'
    $infoA = Invoke-Rpc $serve @{ jsonrpc = '2.0'; id = 3; method = 'tools/call'; params = @{ name = 'get_project_info'; arguments = @{} } }
    Assert ($infoA.result.structuredContent.project.root -eq $projA) 'active project A mismatch'
    $openB = Invoke-Rpc $serve @{ jsonrpc = '2.0'; id = 4; method = 'tools/call'; params = @{ name = 'open_project'; arguments = @{ path = $projB } } }
    Assert ($openB.result.isError -eq $false) 'open_project B failed'
    Assert ($openB.result.structuredContent.ok -eq $true) 'open_project B not ok'
    $infoB = Invoke-Rpc $serve @{ jsonrpc = '2.0'; id = 5; method = 'tools/call'; params = @{ name = 'get_project_info'; arguments = @{} } }
    Assert ($infoB.result.structuredContent.project.root -eq $projB) 'active project B mismatch'
    Write-Host '[OK]  serve open_project cold switch A/B'
}
finally {
    if ($serve -and -not $serve.HasExited) { $serve.Kill() }
}

& $exe daemon stop --project $canonicalDev *> $null

Write-Host '[PASS] mbink-ui-dev P0 regression green'
