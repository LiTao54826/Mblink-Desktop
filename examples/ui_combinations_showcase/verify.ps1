$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
. (Join-Path $PSScriptRoot 'verify_common.ps1')

$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$exeCandidates = @(
    (Join-Path $repo 'build-tests\bin\Debug\esm_loader.exe'),
    (Join-Path $repo 'build\bin\Release\esm_loader.exe'),
    (Join-Path $repo 'build\bin\Debug\esm_loader.exe')
)
$exe = $exeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
$entry = Join-Path $PSScriptRoot 'app.js'
$tmp = Join-Path $repo 'tmp\ui_combinations_showcase'
$snapshot = Join-Path $tmp 'snapshot.json'
$screenshot = Join-Path $tmp 'screenshot.png'
$evidence = Join-Path $tmp 'evidence.json'
$command = Join-Path $tmp 'command.json'
$response = Join-Path $tmp 'response.json'
$consoleFile = Join-Path $tmp 'console.json'
$errorsFile = Join-Path $tmp 'errors.json'
$lifecycleFile = Join-Path $tmp 'lifecycle.json'

function Send-UiDevCommand([hashtable]$payload, [int]$timeoutMs = 6000) {
    if (Test-Path -LiteralPath $response) {
        Remove-Item -LiteralPath $response -Force
    }
    Write-Utf8File $command (($payload | ConvertTo-Json -Compress -Depth 10))
    Wait-ForFile $response $timeoutMs
    return Read-JsonFile $response
}

Assert ($null -ne $exe -and (Test-Path -LiteralPath $exe)) "missing esm_loader in expected build output directories"
if (Test-Path -LiteralPath $tmp) {
    Remove-Item -LiteralPath $tmp -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$timer = [System.Diagnostics.Stopwatch]::StartNew()

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
    '--quit', '30'
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
    $screenshotInfo = Get-Item -LiteralPath $screenshot
    Assert ($screenshotInfo.Length -gt 1024) "screenshot too small: $($screenshotInfo.Length)"
    $screenshotRender = Get-PngEvidence $screenshot
    Assert ($screenshotRender.width -ge 100 -and $screenshotRender.height -ge 100) "screenshot dimensions too small: $($screenshotRender.width)x$($screenshotRender.height)"
    Assert ($screenshotRender.unique_sampled_colors -gt 3) "screenshot appears visually blank; sampled colors: $($screenshotRender.unique_sampled_colors)"
    Assert ($screenshotRender.nontransparent_samples -gt 0) 'screenshot has no nontransparent sampled pixels'

    $layoutRects = [ordered]@{}

    $root = Send-UiDevCommand @{ id = 'query-root'; type = 'query_element'; selector = '#combo-root' }
    Assert-CommandOk $root 'query root'
    Assert ($root.result.count -eq 1) 'root selector count mismatch'
    $layoutRects['combo_root'] = Get-PositiveMatchRect $root 'query root'

    $table = Send-UiDevCommand @{ id = 'query-table'; type = 'query_element'; selector = '#queue-table' }
    Assert-CommandOk $table 'query table'
    Assert ($table.result.count -eq 1) 'table selector count mismatch'
    $layoutRects['queue_table'] = Get-PositiveMatchRect $table 'query table'

    $statePreview = Send-UiDevCommand @{ id = 'query-state-preview'; type = 'query_element'; selector = '#state-preview' }
    Assert-CommandOk $statePreview 'query state preview'
    Assert ($statePreview.result.count -eq 1) 'state preview selector count mismatch'
    $layoutRects['state_preview'] = Get-PositiveMatchRect $statePreview 'query state preview'
    $globalSearch = Send-UiDevCommand @{ id = 'query-global-search'; type = 'query_element'; selector = '#global-search' }
    $layoutRects['global_search'] = Get-PositiveMatchRect $globalSearch 'query global search'
    $activityFeed = Send-UiDevCommand @{ id = 'query-activity-feed'; type = 'query_element'; selector = '#activity-feed' }
    $layoutRects['activity_feed'] = Get-PositiveMatchRect $activityFeed 'query activity feed'
    $normalState = Send-UiDevCommand @{ id = 'inspect-state-normal'; type = 'inspect'; selector = '#state-preview' }
    Assert-ContainsOuterHtml $normalState 'Normal content state' 'normal state preview inspect'

    $search = Send-UiDevCommand @{ id = 'input-search'; type = 'input_text'; selector = '#global-search'; text = 'Auth' }
    Assert-CommandOk $search 'search input'
    $filtered = Send-UiDevCommand @{ id = 'inspect-filtered-row'; type = 'inspect'; selector = '#queue-row-auth' }
    Assert-ContainsOuterHtml $filtered 'Auth Policy' 'filtered row inspect'

    $compact = Send-UiDevCommand @{ id = 'click-compact'; type = 'click'; selector = '#density-compact' }
    Assert-CommandOk $compact 'compact click'
    $compactState = Send-UiDevCommand @{ id = 'inspect-density'; type = 'inspect'; selector = '#density-compact' }
    Assert-ContainsOuterHtml $compactState 'active' 'compact state inspect'

    $allTab = Send-UiDevCommand @{ id = 'click-tab-all'; type = 'click'; selector = '#tab-all' }
    Assert-CommandOk $allTab 'all tab click'
    $checkbox = Send-UiDevCommand @{ id = 'click-auth-checkbox'; type = 'click'; selector = '#select-auth' }
    Assert-CommandOk $checkbox 'checkbox click'
    $selectedCount = Send-UiDevCommand @{ id = 'inspect-selected-count'; type = 'inspect'; selector = '#selected-count' }
    Assert-ContainsOuterHtml $selectedCount '1 selected' 'selected count inspect'

    $clearSearch = Send-UiDevCommand @{ id = 'clear-search'; type = 'input_text'; selector = '#global-search'; text = '' }
    Assert-CommandOk $clearSearch 'clear search input'

    $loadingStateClick = Send-UiDevCommand @{ id = 'click-status-loading'; type = 'click'; selector = '#status-loading' }
    Assert-CommandOk $loadingStateClick 'loading state click'
    $loadingState = Send-UiDevCommand @{ id = 'inspect-state-loading'; type = 'inspect'; selector = '#state-preview' }
    Assert-ContainsOuterHtml $loadingState 'Loading queue snapshot' 'loading state preview inspect'
    $emptyStateClick = Send-UiDevCommand @{ id = 'click-status-empty'; type = 'click'; selector = '#status-empty' }
    Assert-CommandOk $emptyStateClick 'empty state click'
    $emptyState = Send-UiDevCommand @{ id = 'inspect-state-empty'; type = 'inspect'; selector = '#state-preview' }
    Assert-ContainsOuterHtml $emptyState 'No matching queue items' 'empty state preview inspect'
    $errorStateClick = Send-UiDevCommand @{ id = 'click-status-error'; type = 'click'; selector = '#status-error' }
    Assert-CommandOk $errorStateClick 'error state click'
    $errorState = Send-UiDevCommand @{ id = 'inspect-state-error'; type = 'inspect'; selector = '#state-preview' }
    Assert-ContainsOuterHtml $errorState 'Snapshot failed' 'error state preview inspect'
    $normalStateClick = Send-UiDevCommand @{ id = 'click-status-normal'; type = 'click'; selector = '#status-normal' }
    Assert-CommandOk $normalStateClick 'normal state click'

    $input = Send-UiDevCommand @{ id = 'input-request'; type = 'input_text'; selector = '#request-name'; text = 'API Gateway Review' }
    Assert-CommandOk $input 'request input_text'
    Assert ($input.result.value -eq 'API Gateway Review') 'input value mismatch'

    $typeSelect = Send-UiDevCommand @{ id = 'select-request-type'; type = 'input_text'; selector = '#request-type'; text = 'incident' }
    Assert-CommandOk $typeSelect 'request type select change'
    $prioritySelect = Send-UiDevCommand @{ id = 'select-request-priority'; type = 'input_text'; selector = '#request-priority'; text = 'high' }
    Assert-CommandOk $prioritySelect 'request priority select change'
    $notes = Send-UiDevCommand @{ id = 'input-request-notes'; type = 'input_text'; selector = '#request-notes'; text = "Line one`nLine two" }
    Assert-CommandOk $notes 'textarea input'

    $create = Send-UiDevCommand @{ id = 'click-create'; type = 'click'; selector = '#request-create' }
    Assert-CommandOk $create 'create click'

    $result = Send-UiDevCommand @{ id = 'inspect-result'; type = 'inspect'; selector = '#form-result' }
    Assert-ContainsOuterHtml $result 'API Gateway Review' 'form result inspect'
    Assert ($result.result.outer_html -like '*incident / high*') 'select changes did not affect form result'

    $toastStack = Send-UiDevCommand @{ id = 'inspect-toast-stack'; type = 'inspect'; selector = '#toast-stack' }
    Assert-ContainsOuterHtml $toastStack 'API Gateway Review' 'toast stack inspect'

    $ackOff = Send-UiDevCommand @{ id = 'click-ack-off'; type = 'click'; selector = '#request-ack' }
    Assert-CommandOk $ackOff 'ack checkbox off'
    $disabledCreate = Send-UiDevCommand @{ id = 'inspect-disabled-create'; type = 'inspect'; selector = '#request-create' }
    Assert-ContainsOuterHtml $disabledCreate 'disabled' 'disabled create inspect'

    $modalOpen = Send-UiDevCommand @{ id = 'click-modal-open'; type = 'click'; selector = '#open-settings-modal' }
    Assert-CommandOk $modalOpen 'modal open'
    $modal = Send-UiDevCommand @{ id = 'query-modal'; type = 'query_element'; selector = '#settings-dialog' }
    Assert-CommandOk $modal 'modal query'
    Assert ($modal.result.count -eq 1) 'modal not found'
    $modalApply = Send-UiDevCommand @{ id = 'click-modal-apply'; type = 'click'; selector = '#modal-apply' }
    Assert-CommandOk $modalApply 'modal apply click'
    $modalClosed = Send-UiDevCommand @{ id = 'query-modal-closed'; type = 'query_element'; selector = '#settings-dialog' }
    Assert-CommandOk $modalClosed 'modal closed query'
    Assert ($modalClosed.result.count -eq 0) 'modal should be closed'

    $scroll = Send-UiDevCommand @{ id = 'scroll-feed'; type = 'scroll'; selector = '#activity-feed'; y = 80 }
    Assert-CommandOk $scroll 'feed scroll'

    $invalidSelector = Send-UiDevCommand @{ id = 'query-missing'; type = 'query_element'; selector = '#does-not-exist' }
    Assert-CommandOk $invalidSelector 'missing selector query'
    Assert ($invalidSelector.result.count -eq 0) 'missing selector should return zero count'

    $errors = Read-JsonFile $errorsFile
    Assert (@($errors.errors).Count -eq 0) 'unexpected JS errors'

    $timer.Stop()
    Assert ($timer.Elapsed.TotalSeconds -lt 30) "verification exceeded 30s: $($timer.Elapsed.TotalSeconds)"
    $evidenceObject = [ordered]@{
        ok = $true
        elapsed_ms = [int]$timer.Elapsed.TotalMilliseconds
        exe = $exe
        entry = $entry
        snapshot = $snapshot
        screenshot = $screenshot
        screenshot_bytes = $screenshotInfo.Length
        screenshot_render = $screenshotRender
        node_count = $snap.node_count
        layout_rects = $layoutRects
        checks = @(
            'snapshot',
            'screenshot decoded and visually nonblank',
            'positive layout rects for root/table/state preview/search/feed',
            'query root/table/state preview/modal/missing selector',
            'inspect filtered row/density/selected count/state preview/form result/toast/disabled button',
            'input text/search/textarea',
            'select value changes through UI-dev input_text path',
            'click checkbox/tab/segmented/button/modal',
            'state preview normal/loading/empty/error modes',
            'scroll activity feed',
            'js errors empty',
            'elapsed under 30s'
        )
    }
    Write-Utf8File $evidence (($evidenceObject | ConvertTo-Json -Depth 6))
    Write-Host '[OK] ui_combinations_showcase snapshot/query/inspect/input/select/click/scroll checks passed'
    Write-Host "[OK] screenshot: $screenshot"
    Write-Host "[OK] evidence: $evidence"
    $completed = $true
}
finally {
    if (-not $completed -and $proc -and -not $proc.HasExited) {
        $proc.Kill()
    }
}

if (-not $proc.WaitForExit(35000)) {
    $proc.Kill()
    throw 'esm_loader did not exit before timeout'
}
$stdout = $proc.StandardOutput.ReadToEnd()
$stderr = $proc.StandardError.ReadToEnd()
Assert ($proc.ExitCode -eq 0) "esm_loader failed exit=$($proc.ExitCode)`nstdout=$stdout`nstderr=$stderr"
