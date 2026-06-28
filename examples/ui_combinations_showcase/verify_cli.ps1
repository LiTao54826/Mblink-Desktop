$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)
. (Join-Path $PSScriptRoot 'verify_common.ps1')

$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$exe = Join-Path $repo 'build\bin\Release\mblink-ui-dev.exe'
$project = Resolve-Path $PSScriptRoot
$tmp = Join-Path $repo 'tmp\ui_combinations_showcase'
$snapshotCopy = Join-Path $tmp 'mblink-ui-dev-cli-snapshot.json'
$screenshotCopy = Join-Path $tmp 'mblink-ui-dev-cli-screenshot.png'
$evidence = Join-Path $tmp 'mblink-ui-dev-cli-evidence.json'

function Invoke-MblinkCli([string[]]$arguments) {
    $raw = & $exe @arguments 2>&1 | Out-String
    $exit = $LASTEXITCODE
    Assert ($exit -eq 0) "mblink-ui-dev exited $exit for: $($arguments -join ' ')`n$raw"
    Assert ($raw.Trim().Length -gt 0) "mblink-ui-dev returned empty output for: $($arguments -join ' ')"
    $json = $raw | ConvertFrom-Json
    Assert ($json.ok -eq $true) "mblink-ui-dev returned ok=false for: $($arguments -join ' ')`n$raw"
    return $json
}

Assert (Test-Path -LiteralPath $exe) "missing mblink-ui-dev: $exe"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

$timer = [System.Diagnostics.Stopwatch]::StartNew()

Invoke-MblinkCli @('stop', '--project', $project.Path) | Out-Null
$open = Invoke-MblinkCli @('open', '--project', $project.Path)
$info = Invoke-MblinkCli @('info', '--project', $project.Path)
$snapshot = Invoke-MblinkCli @('snapshot', '--project', $project.Path, '--response', 'file', '--include-screenshot')

Assert ($snapshot.response_mode -eq 'file') 'snapshot response_mode is not file'
Assert ($snapshot.snapshot.path -and (Test-Path -LiteralPath $snapshot.snapshot.path)) 'snapshot JSON file missing'
Assert ($snapshot.screenshot.included -eq $true) 'snapshot screenshot metadata missing'
Assert ($snapshot.screenshot.path -and (Test-Path -LiteralPath $snapshot.screenshot.path)) 'snapshot screenshot PNG missing'
Assert ([int64]$snapshot.screenshot.bytes -gt 1024) "snapshot screenshot too small: $($snapshot.screenshot.bytes)"
Copy-Item -LiteralPath $snapshot.snapshot.path -Destination $snapshotCopy -Force
Copy-Item -LiteralPath $snapshot.screenshot.path -Destination $screenshotCopy -Force
$screenshotRender = Get-PngEvidence $screenshotCopy
Assert ($screenshotRender.width -ge 100 -and $screenshotRender.height -ge 100) "screenshot dimensions too small: $($screenshotRender.width)x$($screenshotRender.height)"
Assert ($screenshotRender.unique_sampled_colors -gt 3) "screenshot appears visually blank; sampled colors: $($screenshotRender.unique_sampled_colors)"
Assert ($screenshotRender.nontransparent_samples -gt 0) 'screenshot has no nontransparent sampled pixels'

$layoutRects = [ordered]@{}
$root = Invoke-MblinkCli @('query', '#combo-root', '--project', $project.Path)
Assert ($root.result.count -eq 1) 'root query count mismatch'
$layoutRects['combo_root'] = Get-PositiveMatchRect $root 'root query'
$table = Invoke-MblinkCli @('query', '#queue-table', '--project', $project.Path)
Assert ($table.result.count -eq 1) 'table query count mismatch'
$layoutRects['queue_table'] = Get-PositiveMatchRect $table 'table query'
$statePreviewQuery = Invoke-MblinkCli @('query', '#state-preview', '--project', $project.Path)
Assert ($statePreviewQuery.result.count -eq 1) 'state preview query count mismatch'
$layoutRects['state_preview'] = Get-PositiveMatchRect $statePreviewQuery 'state preview query'
$globalSearchQuery = Invoke-MblinkCli @('query', '#global-search', '--project', $project.Path)
$layoutRects['global_search'] = Get-PositiveMatchRect $globalSearchQuery 'global search query'
$activityFeedQuery = Invoke-MblinkCli @('query', '#activity-feed', '--project', $project.Path)
$layoutRects['activity_feed'] = Get-PositiveMatchRect $activityFeedQuery 'activity feed query'
$statePreview = Invoke-MblinkCli @('inspect', '#state-preview', '--project', $project.Path)
Assert ($statePreview.result.outer_html -like '*Normal content state*') 'normal state preview text missing'

$search = Invoke-MblinkCli @('input-text', '#global-search', 'Auth', '--project', $project.Path)
Assert ($search.result.value -eq 'Auth') 'search input value mismatch'
$filtered = Invoke-MblinkCli @('inspect', '#queue-row-auth', '--project', $project.Path)
Assert ($filtered.result.outer_html -like '*Auth Policy*') 'filtered row text missing'
$clearSearch = Invoke-MblinkCli @('input-text', '#global-search', '--clear', '--project', $project.Path)
Assert ($clearSearch.result.value -eq '') 'input-text --clear did not clear search'

$compact = Invoke-MblinkCli @('click', '#density-compact', '--project', $project.Path)
Assert ($compact.result.clicked -eq $true) 'compact click failed'
$compactState = Invoke-MblinkCli @('inspect', '#density-compact', '--project', $project.Path)
Assert ($compactState.result.outer_html -like '*active*') 'compact density state missing'

foreach ($mode in @(
    @{ selector = '#status-loading'; text = 'Loading queue snapshot' },
    @{ selector = '#status-empty'; text = 'No matching queue items' },
    @{ selector = '#status-error'; text = 'Snapshot failed' },
    @{ selector = '#status-normal'; text = 'Normal content state' }
)) {
    $click = Invoke-MblinkCli @('click', $mode.selector, '--project', $project.Path)
    Assert ($click.result.clicked -eq $true) "state click failed: $($mode.selector)"
    $state = Invoke-MblinkCli @('inspect', '#state-preview', '--project', $project.Path)
    Assert ($state.result.outer_html -like "*$($mode.text)*") "state preview missing text: $($mode.text)"
}

$name = Invoke-MblinkCli @('input-text', '#request-name', '--text', 'CLI Dev Evidence', '--project', $project.Path)
Assert ($name.result.value -eq 'CLI Dev Evidence') 'request name value mismatch'
$type = Invoke-MblinkCli @('input-text', '#request-type', '--text', 'incident', '--project', $project.Path)
Assert ($type.result.value -eq 'incident') 'request type select value mismatch'
$priority = Invoke-MblinkCli @('input-text', '#request-priority', '--text', 'high', '--project', $project.Path)
Assert ($priority.result.value -eq 'high') 'request priority select value mismatch'
$notes = Invoke-MblinkCli @('input-text', '#request-notes', '--text', "CLI line one`nCLI line two", '--project', $project.Path)
Assert ($notes.result.value -like "*CLI line two*") 'textarea multiline value mismatch'
$create = Invoke-MblinkCli @('click', '#request-create', '--project', $project.Path)
Assert ($create.result.clicked -eq $true) 'create click failed'
$result = Invoke-MblinkCli @('inspect', '#form-result', '--project', $project.Path)
Assert ($result.result.outer_html -like '*CLI Dev Evidence*') 'form result missing request name'
Assert ($result.result.outer_html -like '*incident / high*') 'form result missing select values'

$modalOpen = Invoke-MblinkCli @('click', '#open-settings-modal', '--project', $project.Path)
Assert ($modalOpen.result.clicked -eq $true) 'modal open click failed'
$modal = Invoke-MblinkCli @('query', '#settings-dialog', '--project', $project.Path)
Assert ($modal.result.count -eq 1) 'modal query count mismatch'
$modalApply = Invoke-MblinkCli @('click', '#modal-apply', '--project', $project.Path)
Assert ($modalApply.result.clicked -eq $true) 'modal apply click failed'
$modalClosed = Invoke-MblinkCli @('query', '#settings-dialog', '--project', $project.Path)
Assert ($modalClosed.result.count -eq 0) 'modal did not close'

$scroll = Invoke-MblinkCli @('scroll', '#activity-feed', '--y', '80', '--project', $project.Path)
Assert ($scroll.result.selector -eq '#activity-feed') 'activity feed scroll failed'
$missing = Invoke-MblinkCli @('query', '#does-not-exist', '--project', $project.Path)
Assert ($missing.result.count -eq 0) 'missing selector should return zero matches'
$logs = Invoke-MblinkCli @('logs', '--project', $project.Path)
Assert ($logs.ok -eq $true) 'logs command failed'
$errors = Invoke-MblinkCli @('errors', '--project', $project.Path)
Assert (@($errors.errors).Count -eq 0) 'runtime JS errors were reported'
$stop = Invoke-MblinkCli @('stop', '--project', $project.Path)
Assert ($stop.ok -eq $true) 'stop command failed'

$timer.Stop()
Assert ($timer.Elapsed.TotalSeconds -lt 30) "CLI verification exceeded 30s: $($timer.Elapsed.TotalSeconds)"
$screenshotInfo = Get-Item -LiteralPath $screenshotCopy

$evidenceObject = [ordered]@{
    ok = $true
    elapsed_ms = [int]$timer.Elapsed.TotalMilliseconds
    exe = $exe
    project = $project.Path
    runtime_epoch = $snapshot.runtime_epoch
    snapshot = $snapshotCopy
    screenshot = $screenshotCopy
    screenshot_bytes = $screenshotInfo.Length
    screenshot_render = $screenshotRender
    source_snapshot = $snapshot.snapshot.path
    source_screenshot = $snapshot.screenshot.path
    node_snapshot_bytes = [int64]$snapshot.snapshot.bytes
    viewport = $snapshot.viewport
    layout_rects = $layoutRects
    open_runtime_status = $open.runtime_status
    info_runtime_status = $info.daemon.runtime_status
    checks = @(
        'mblink-ui-dev open/info/snapshot --response file --include-screenshot',
        'screenshot decoded and visually nonblank',
        'positive layout rects for root/table/state preview/search/feed',
        'query root/table/missing selector',
        'inspect state preview normal/loading/empty/error modes',
        'input-text positional, --text, --clear, textarea, select values',
        'click density/state/create/modal',
        'scroll activity feed',
        'logs ok and errors empty',
        'elapsed under 30s'
    )
}
Set-Content -LiteralPath $evidence -Value ($evidenceObject | ConvertTo-Json -Depth 8) -Encoding UTF8
Write-Host '[OK] ui_combinations_showcase mblink-ui-dev CLI checks passed'
Write-Host "[OK] screenshot: $screenshotCopy"
Write-Host "[OK] evidence: $evidence"
