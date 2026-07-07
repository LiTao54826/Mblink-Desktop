param(
    [switch]$StopAfterVerify
)

$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

function Assert($condition, [string]$message) {
    if (-not $condition) {
        throw $message
    }
}

$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$exe = Join-Path $repo 'build\bin\Release\mblink-ui-dev.exe'
$project = Resolve-Path $PSScriptRoot
$tmp = Join-Path $repo 'tmp\markdown_preview_compat'
$snapshotCopy = Join-Path $tmp 'snapshot.json'
$screenshotCopy = Join-Path $tmp 'screenshot.png'
$evidence = Join-Path $tmp 'evidence.json'

function Invoke-MblinkCli([string[]]$arguments) {
    $raw = & $exe @arguments 2>&1 | Out-String
    $exit = $LASTEXITCODE
    Assert ($exit -eq 0) "mblink-ui-dev exited $exit for: $($arguments -join ' ')`n$raw"
    Assert ($raw.Trim().Length -gt 0) "mblink-ui-dev returned empty output for: $($arguments -join ' ')"
    $json = $raw | ConvertFrom-Json
    Assert ($json.ok -eq $true) "mblink-ui-dev returned ok=false for: $($arguments -join ' ')`n$raw"
    return $json
}

function Require-Query($selector) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $project.Path)
    Assert ($query.result.count -eq 1) "selector count mismatch for $selector"
    $match = $query.result.matches[0]
    $width = if ($null -ne $match.rect.width) { [double]$match.rect.width } else { [double]$match.rect.w }
    $height = if ($null -ne $match.rect.height) { [double]$match.rect.height } else { [double]$match.rect.h }
    Assert ($width -gt 0) "selector has zero width: $selector"
    Assert ($height -gt 0) "selector has zero height: $selector"
    return $query
}

function Get-RenderedCount($text) {
    if ($text -notmatch 'Rendering\s+(\d+)\s+of\s+(\d+)') {
        throw "virtualization text did not include a rendered count: $text"
    }
    return @{
        Visible = [int]$Matches[1]
        Total = [int]$Matches[2]
    }
}

Assert (Test-Path -LiteralPath $exe) "missing mblink-ui-dev: $exe"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

$rendererPath = (Join-Path $PSScriptRoot 'ui\markdown_renderer.js').Replace('\', '/')
$rendererGuard = @"
import { createMarkdownRenderer } from 'file:///$rendererPath';

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function h(type, props, ...children) {
  return { type, props: props || {}, children: children.flat(Infinity) };
}

function findNode(nodes, type) {
  for (const node of nodes) {
    if (node && node.type === type) return node;
    if (node && Array.isArray(node.children)) {
      const nested = findNode(node.children, type);
      if (nested) return nested;
    }
  }
  return null;
}

const renderer = createMarkdownRenderer({ h });
const unsafeImage = findNode(renderer.render('![x](data:image/svg+xml;base64,PHN2Zy8+)').nodes, 'img');
const safeImage = findNode(renderer.render('![x](data:image/png;base64,iVBORw0KGgo=)').nodes, 'img');
const unsafeLink = findNode(renderer.render('[bad](javascript:alert(1))').nodes, 'a');
assert(unsafeImage && unsafeImage.props.src === '', 'SVG data image was not rejected');
assert(safeImage && safeImage.props.src.startsWith('data:image/png;base64,'), 'PNG data image was not retained');
assert(unsafeLink && unsafeLink.props.href === '#', 'javascript: link was not clamped');
"@
$rendererGuard | node --input-type=module
Assert ($LASTEXITCODE -eq 0) 'renderer safety guard failed'

$timer = [System.Diagnostics.Stopwatch]::StartNew()

$build = Invoke-MblinkCli @('build', '--project', $project.Path)
Invoke-MblinkCli @('stop', '--project', $project.Path) | Out-Null
$open = Invoke-MblinkCli @('open', '--project', $project.Path)
$snapshot = Invoke-MblinkCli @('snapshot', '--project', $project.Path, '--response', 'file', '--include-screenshot')

Assert ($snapshot.response_mode -eq 'file') 'snapshot response_mode is not file'
Assert ($snapshot.snapshot.path -and (Test-Path -LiteralPath $snapshot.snapshot.path)) 'snapshot JSON file missing'
Assert ($snapshot.screenshot.included -eq $true) 'snapshot screenshot metadata missing'
Assert ($snapshot.screenshot.path -and (Test-Path -LiteralPath $snapshot.screenshot.path)) 'snapshot screenshot PNG missing'
Assert ([int64]$snapshot.screenshot.bytes -gt 1024) "snapshot screenshot too small: $($snapshot.screenshot.bytes)"
Copy-Item -LiteralPath $snapshot.snapshot.path -Destination $snapshotCopy -Force
Copy-Item -LiteralPath $snapshot.screenshot.path -Destination $screenshotCopy -Force

Require-Query '#markdown-preview-root' | Out-Null
Require-Query '#markdown-source' | Out-Null
Require-Query '#markdown-preview' | Out-Null
Require-Query '#markdown-heading-fixture' | Out-Null
Require-Query '#markdown-link-fixture' | Out-Null
Require-Query '#markdown-image-fixture' | Out-Null
Require-Query '#markdown-table-fixture' | Out-Null
Require-Query '#markdown-code-fixture' | Out-Null
Require-Query '#markdown-safe-html-fixture' | Out-Null
Require-Query '#markdown-stats' | Out-Null

$image = Invoke-MblinkCli @('inspect', '#markdown-image-fixture', '--project', $project.Path)
Assert ($image.result.outer_html -like '*data:image/png;base64,*') 'image fixture did not use the inline PNG fixture'
$preview = Invoke-MblinkCli @('inspect', '#markdown-preview', '--project', $project.Path)
Assert ($preview.result.outer_html -like '*Mainstream Markdown Preview*') 'preview heading missing'
Assert ($preview.result.outer_html -like '*Raw HTML is displayed as text*') 'safe HTML text missing'
Assert ($preview.result.outer_html -like '*renderMarkdown*') 'code fence text missing'

$edited = @'
# Edited Preview

This edit proves live preview updates from the source textarea.

| Column | Value |
| --- | --- |
| updated | yes |
'@
$input = Invoke-MblinkCli @('input-text', '#markdown-source', '--text', $edited, '--project', $project.Path)
Assert ($input.result.value -like '*Edited Preview*') 'textarea edit did not apply'
$editedPreview = Invoke-MblinkCli @('inspect', '#markdown-preview', '--project', $project.Path)
Assert ($editedPreview.result.outer_html -like '*Edited Preview*') 'edited preview did not render'
Assert ($editedPreview.result.outer_html -like '*updated*') 'edited table did not render'

$reset = Invoke-MblinkCli @('click', '#markdown-reset', '--project', $project.Path)
Assert ($reset.result.clicked -eq $true) 'reset click failed'
$chat = Invoke-MblinkCli @('click', '#chat-mode', '--project', $project.Path)
Assert ($chat.result.clicked -eq $true) 'chat mode click failed'
Require-Query '#markdown-message-list' | Out-Null
Require-Query '#markdown-virtualization-state' | Out-Null
$messageStateBefore = Invoke-MblinkCli @('inspect', '#markdown-virtualization-state', '--project', $project.Path)
$messageCountsBefore = Get-RenderedCount $messageStateBefore.result.outer_html
Assert ($messageCountsBefore.Visible -lt $messageCountsBefore.Total) 'message virtualization did not start with a bounded window'
$stream = Invoke-MblinkCli @('click', '#markdown-stream-append', '--project', $project.Path)
Assert ($stream.result.clicked -eq $true) 'stream append click failed'
$streamState = Invoke-MblinkCli @('inspect', '#markdown-stream-state', '--project', $project.Path)
Assert ($streamState.result.outer_html -like '*stream chunks*') 'stream state missing'
$showMore = Invoke-MblinkCli @('click', '#markdown-show-more-messages', '--project', $project.Path)
Assert ($showMore.result.clicked -eq $true) 'show more messages click failed'
$messageStateAfter = Invoke-MblinkCli @('inspect', '#markdown-virtualization-state', '--project', $project.Path)
$messageCountsAfter = Get-RenderedCount $messageStateAfter.result.outer_html
Assert ($messageCountsAfter.Visible -gt $messageCountsBefore.Visible) 'show more messages did not increase the rendered message window'
$document = Invoke-MblinkCli @('click', '#document-mode', '--project', $project.Path)
Assert ($document.result.clicked -eq $true) 'document mode click failed'
$longDoc = Invoke-MblinkCli @('click', '#markdown-load-long-doc', '--project', $project.Path)
Assert ($longDoc.result.clicked -eq $true) 'long document click failed'
Require-Query '#markdown-block-window-state' | Out-Null
$blockStateBefore = Invoke-MblinkCli @('inspect', '#markdown-block-window-state', '--project', $project.Path)
$blockCountsBefore = Get-RenderedCount $blockStateBefore.result.outer_html
Assert ($blockCountsBefore.Visible -lt $blockCountsBefore.Total) 'block virtualization did not start with a bounded window'
$moreBlocks = Invoke-MblinkCli @('click', '#markdown-show-more-blocks', '--project', $project.Path)
Assert ($moreBlocks.result.clicked -eq $true) 'show more blocks click failed'
$blockStateAfter = Invoke-MblinkCli @('inspect', '#markdown-block-window-state', '--project', $project.Path)
$blockCountsAfter = Get-RenderedCount $blockStateAfter.result.outer_html
Assert ($blockCountsAfter.Visible -gt $blockCountsBefore.Visible) 'show more blocks did not increase the rendered block window'

$scroll = Invoke-MblinkCli @('scroll', '#markdown-preview-scroll', '--y', '120', '--project', $project.Path)
Assert ($scroll.result.selector -eq '#markdown-preview-scroll') 'preview scroll failed'
$logs = Invoke-MblinkCli @('logs', '--project', $project.Path)
Assert ($logs.ok -eq $true) 'logs command failed'
$errors = Invoke-MblinkCli @('errors', '--project', $project.Path)
Assert (@($errors.errors).Count -eq 0) 'runtime JS errors were reported'
if ($StopAfterVerify) {
    $stop = Invoke-MblinkCli @('stop', '--project', $project.Path)
    Assert ($stop.ok -eq $true) 'stop command failed'
}

$timer.Stop()
$evidenceObject = [ordered]@{
    ok = $true
    elapsed_ms = [int]$timer.Elapsed.TotalMilliseconds
    exe = $exe
    project = $project.Path
    build_status = $build.status
    open_runtime_status = $open.runtime_status
    stop_after_verify = [bool]$StopAfterVerify
    runtime_epoch = $snapshot.runtime_epoch
    snapshot = $snapshotCopy
    screenshot = $screenshotCopy
    screenshot_bytes = [int64](Get-Item -LiteralPath $screenshotCopy).Length
    checks = @(
        'renderer safety guard rejects SVG data images and javascript links',
        'build/open/snapshot with screenshot',
        'mainstream Markdown fixture selectors',
        'inline PNG image fixture asserted',
        'live source edit updates preview',
        'chat mode message-level virtualization count increase',
        'stream append interaction',
        'long document block-window count increase',
        'preview scroll',
        'logs ok and errors empty'
    )
}
Set-Content -LiteralPath $evidence -Value ($evidenceObject | ConvertTo-Json -Depth 6) -Encoding UTF8
Write-Host '[OK] markdown_preview_compat checks passed'
Write-Host "[OK] screenshot: $screenshotCopy"
Write-Host "[OK] evidence: $evidence"
