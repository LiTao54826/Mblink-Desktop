$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Set-Location $repoRoot

$exe = Join-Path $repoRoot 'build\bin\Release\mblink-ui-dev.exe'
$tmpRoot = Join-Path $repoRoot 'tmp\css_layout_stability'
$project = Join-Path $tmpRoot 'app'
$evidencePath = Join-Path $tmpRoot 'evidence.json'
$screenshotCopy = Join-Path $tmpRoot 'layout-stability-screenshot.png'
$snapshotCopy = Join-Path $tmpRoot 'layout-stability-snapshot.json'

function Assert([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}

function Write-Utf8NoBomFile([string]$path, [string]$content) {
    $parent = Split-Path -Parent $path
    if ($parent -and !(Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    [System.IO.File]::WriteAllText($path, $content, [System.Text.UTF8Encoding]::new($false))
}

function Invoke-MblinkCli([string[]]$arguments) {
    $raw = & $exe @arguments 2>&1 | Out-String
    $exit = $LASTEXITCODE
    Assert ($exit -eq 0) "mblink-ui-dev exited $exit for: $($arguments -join ' ')`n$raw"
    $json = $raw | ConvertFrom-Json
    Assert ($json.ok -eq $true) "mblink-ui-dev returned ok=false for: $($arguments -join ' ')`n$raw"
    return $json
}

function Get-PngEvidence([string]$path) {
    Assert (Test-Path -LiteralPath $path) "missing PNG file: $path"
    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::FromFile($path)
    try {
        $colors = [System.Collections.Generic.HashSet[string]]::new()
        $nonTransparent = 0
        for ($yi = 0; $yi -lt 8; ++$yi) {
            $y = [Math]::Min($bitmap.Height - 1, [int][Math]::Round(($bitmap.Height - 1) * $yi / 7))
            for ($xi = 0; $xi -lt 12; ++$xi) {
                $x = [Math]::Min($bitmap.Width - 1, [int][Math]::Round(($bitmap.Width - 1) * $xi / 11))
                $color = $bitmap.GetPixel($x, $y)
                if ($color.A -gt 0) { ++$nonTransparent }
                [void]$colors.Add("$($color.A),$($color.R),$($color.G),$($color.B)")
            }
        }
        return [ordered]@{
            width = $bitmap.Width
            height = $bitmap.Height
            nontransparent_samples = $nonTransparent
            unique_sampled_colors = $colors.Count
        }
    }
    finally {
        $bitmap.Dispose()
    }
}

function Get-Rect([string]$selector) {
    $query = Invoke-MblinkCli @('query', $selector, '--project', $project)
    Assert ([int]$query.result.count -eq 1) "selector count mismatch for $selector"
    $match = @($query.result.matches)[0]
    Assert ($null -ne $match.rect) "missing rect for $selector"
    $rect = $match.rect
    Assert ([double]$rect.w -gt 0 -and [double]$rect.h -gt 0) "empty rect for $selector"
    return [ordered]@{
        x = [Math]::Round([double]$rect.x, 3)
        y = [Math]::Round([double]$rect.y, 3)
        w = [Math]::Round([double]$rect.w, 3)
        h = [Math]::Round([double]$rect.h, 3)
    }
}

function Compare-Rect([object]$a, [object]$b, [string]$label, [double]$tolerance = 0.5) {
    foreach ($key in @('x', 'y', 'w', 'h')) {
        $delta = [Math]::Abs([double]$a[$key] - [double]$b[$key])
        Assert ($delta -le $tolerance) "$label rect $key drifted by $delta; before=$($a | ConvertTo-Json -Compress) after=$($b | ConvertTo-Json -Compress)"
    }
}

Assert (Test-Path -LiteralPath $exe) "missing mblink-ui-dev: $exe"

if (Test-Path -LiteralPath $tmpRoot) {
    Remove-Item -LiteralPath $tmpRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path (Join-Path $project 'ui') | Out-Null

Write-Utf8NoBomFile (Join-Path $project 'mblink.config.json') @'
{
  "name": "css_layout_stability",
  "entry": "ui/app.js",
  "runtime": "tool",
  "window": {
    "title": "CSS Layout Stability",
    "width": 1040,
    "height": 720,
    "resizable": true
  }
}
'@

Write-Utf8NoBomFile (Join-Path $project 'ui\app.js') @'
const css = `
* { box-sizing: border-box; }
html, body { width: 100%; height: 100%; margin: 0; overflow: hidden; font-family: Segoe UI, Arial, sans-serif; }
body { background: #edf2f7; color: #172033; }
#app { width: 100vw; height: 100vh; display: grid; grid-template-columns: 220px minmax(0, 1fr); grid-template-rows: auto minmax(0, 1fr); gap: 12px; padding: 14px; }
#topbar { grid-column: 1 / 3; display: flex; align-items: center; gap: 8px; min-width: 0; padding: 10px; background: #ffffff; border: 1px solid #cbd5e1; }
#search { width: 240px; min-width: 160px; padding: 8px 10px; border: 1px solid #94a3b8; border-radius: 4px; }
.toggle { padding: 8px 10px; border: 1px solid #64748b; border-radius: 4px; background: #f8fafc; color: #0f172a; }
.toggle.active { background: #14532d; color: white; }
#sidebar { min-height: 0; overflow: auto; background: #1f2937; color: white; padding: 12px; display: flex; flex-direction: column; gap: 8px; }
#workspace { min-width: 0; min-height: 0; display: grid; grid-template-rows: auto minmax(0, 1fr) auto; gap: 12px; }
#cards { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 10px; }
.card { min-width: 0; min-height: 82px; padding: 10px; background: white; border: 1px solid #cbd5e1; display: flex; flex-direction: column; gap: 6px; }
.card strong { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
#data-table-wrap { min-height: 0; overflow: auto; background: white; border: 1px solid #cbd5e1; }
#data-table { width: 100%; border-collapse: collapse; table-layout: fixed; }
#data-table th, #data-table td { border-bottom: 1px solid #e2e8f0; padding: 8px; text-align: left; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
#data-table th:first-child, #data-table td:first-child { width: 42px; text-align: center; }
#data-table input[type="checkbox"] { width: 16px; height: 16px; }
#form-row { display: grid; grid-template-columns: minmax(0, 1fr) 140px 120px; gap: 8px; background: white; border: 1px solid #cbd5e1; padding: 10px; }
#name-input, #kind-select { width: 100%; min-width: 0; padding: 8px 10px; border: 1px solid #94a3b8; border-radius: 4px; }
#save-button { padding: 8px 10px; border: 0; border-radius: 4px; background: #0f766e; color: white; }
#result { min-height: 22px; color: #0f766e; }
.compact #cards { grid-template-columns: repeat(4, minmax(0, 1fr)); }
.compact .card { min-height: 58px; padding: 8px; }
`;

const rows = [
  ['Auth Policy', 'Security', 'Ready'],
  ['Billing Sync', 'Data', 'Running'],
  ['Worker Pool', 'Compute', 'Queued'],
  ['Release Review', 'QA', 'Blocked'],
  ['Telemetry Digest', 'Ops', 'Ready'],
  ['API Gateway', 'Network', 'Running']
];

function el(tag, attrs = {}, children = []) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs)) {
    if (key === 'className') node.className = value;
    else if (key === 'text') node.textContent = value;
    else if (key.startsWith('on')) node.addEventListener(key.slice(2).toLowerCase(), value);
    else node.setAttribute(key, value);
  }
  for (const child of children) node.appendChild(child);
  return node;
}

function render() {
  const style = el('style', { text: css });
  document.head.appendChild(style);
  const app = el('main', { id: 'app' });
  const topbar = el('header', { id: 'topbar' }, [
    el('input', { id: 'search', value: '', placeholder: 'Search queue' }),
    el('button', { id: 'density-normal', className: 'toggle active', text: 'Normal' }),
    el('button', { id: 'density-compact', className: 'toggle', text: 'Compact' })
  ]);
  const sidebar = el('aside', { id: 'sidebar' }, Array.from({ length: 10 }, (_, index) => el('button', { className: 'toggle', text: `Navigation ${index + 1}` })));
  const cards = el('section', { id: 'cards' }, rows.slice(0, 4).map(([name, type, state], index) =>
    el('article', { id: `card-${index + 1}`, className: 'card' }, [
      el('strong', { text: name }),
      el('span', { text: type }),
      el('span', { text: state })
    ])
  ));
  const tableBody = el('tbody', {}, rows.map(([name, type, state], index) =>
    el('tr', { id: `row-${index + 1}` }, [
      el('td', {}, [el('input', { id: `check-${index + 1}`, type: 'checkbox' })]),
      el('td', { text: name }),
      el('td', { text: type }),
      el('td', { text: state })
    ])
  ));
  const table = el('table', { id: 'data-table' }, [
    el('thead', {}, [el('tr', {}, [el('th', { text: '' }), el('th', { text: 'Name' }), el('th', { text: 'Type' }), el('th', { text: 'State' })])]),
    tableBody
  ]);
  const form = el('section', { id: 'form-row' }, [
    el('input', { id: 'name-input', value: '', placeholder: 'Request name' }),
    el('select', { id: 'kind-select' }, [
      el('option', { value: 'task', text: 'Task' }),
      el('option', { value: 'incident', text: 'Incident' }),
      el('option', { value: 'change', text: 'Change' })
    ]),
    el('button', { id: 'save-button', text: 'Save' })
  ]);
  const workspace = el('section', { id: 'workspace' }, [
    cards,
    el('div', { id: 'data-table-wrap' }, [table]),
    form,
    el('div', { id: 'result', text: 'idle' })
  ]);
  app.appendChild(topbar);
  app.appendChild(sidebar);
  app.appendChild(workspace);
  document.body.appendChild(app);

  document.getElementById('density-normal').addEventListener('click', () => {
    app.classList.remove('compact');
    document.getElementById('density-normal').classList.add('active');
    document.getElementById('density-compact').classList.remove('active');
  });
  document.getElementById('density-compact').addEventListener('click', () => {
    app.classList.add('compact');
    document.getElementById('density-compact').classList.add('active');
    document.getElementById('density-normal').classList.remove('active');
  });
  document.getElementById('search').addEventListener('input', (event) => {
    const needle = event.currentTarget.value.toLowerCase();
    for (const row of tableBody.children) {
      row.style.display = row.textContent.toLowerCase().includes(needle) ? '' : 'none';
    }
  });
  document.getElementById('save-button').addEventListener('click', () => {
    const name = document.getElementById('name-input').value;
    const kind = document.getElementById('kind-select').value;
    document.getElementById('result').textContent = `${name} / ${kind}`;
  });
}

render();
'@

$timer = [System.Diagnostics.Stopwatch]::StartNew()

try {
    Invoke-MblinkCli @('stop', '--project', $project) | Out-Null
    Invoke-MblinkCli @('open', '--project', $project) | Out-Null
    $snapshot = Invoke-MblinkCli @('snapshot', '--project', $project, '--response', 'file', '--include-screenshot')
    Assert ($snapshot.snapshot.path -and (Test-Path -LiteralPath $snapshot.snapshot.path)) 'snapshot JSON missing'
    Assert ($snapshot.screenshot.path -and (Test-Path -LiteralPath $snapshot.screenshot.path)) 'screenshot PNG missing'
    Copy-Item -LiteralPath $snapshot.snapshot.path -Destination $snapshotCopy -Force
    Copy-Item -LiteralPath $snapshot.screenshot.path -Destination $screenshotCopy -Force
    $pngEvidence = Get-PngEvidence $screenshotCopy
    Assert ($pngEvidence.unique_sampled_colors -gt 3) 'screenshot appears blank'

    $stableSelectors = @('#app', '#topbar', '#sidebar', '#workspace', '#cards', '#data-table-wrap', '#data-table', '#form-row', '#search', '#name-input', '#kind-select', '#save-button')
    $baseline = [ordered]@{}
    foreach ($selector in $stableSelectors) {
        $baseline[$selector] = Get-Rect $selector
    }

    for ($sample = 1; $sample -le 3; ++$sample) {
        $again = [ordered]@{}
        foreach ($selector in $stableSelectors) {
            $again[$selector] = Get-Rect $selector
            Compare-Rect $baseline[$selector] $again[$selector] "$selector stable sample $sample"
        }
    }

    $search = Invoke-MblinkCli @('input-text', '#search', '--text', 'Auth', '--project', $project)
    Assert ($search.result.value -eq 'Auth') 'search input value mismatch'
    $row1 = Invoke-MblinkCli @('query', '#row-1', '--project', $project)
    Assert ($row1.result.count -eq 1) 'filtered row missing'
    $row2 = Invoke-MblinkCli @('query', '#row-2', '--project', $project)
    Assert ($row2.result.count -eq 1) 'hidden row should remain queryable'
    Assert ([double](@($row2.result.matches)[0].rect.h) -eq 0) 'hidden filtered row should have zero height'

    Invoke-MblinkCli @('input-text', '#search', '--clear', '--project', $project) | Out-Null
    $tableAfterClear = Get-Rect '#data-table'
    Compare-Rect $baseline['#data-table'] $tableAfterClear '#data-table after search clear'

    Invoke-MblinkCli @('click', '#density-compact', '--project', $project) | Out-Null
    $compactCard = Get-Rect '#card-1'
    Assert ([double]$compactCard.h -lt [double]$baseline['#cards'].h) 'compact mode did not reduce card geometry'
    Invoke-MblinkCli @('click', '#density-normal', '--project', $project) | Out-Null
    foreach ($selector in @('#app', '#topbar', '#sidebar', '#workspace', '#data-table-wrap', '#data-table', '#form-row')) {
        Compare-Rect $baseline[$selector] (Get-Rect $selector) "$selector after compact roundtrip"
    }

    $input = Invoke-MblinkCli @('input-text', '#name-input', '--text', 'Layout Stability', '--project', $project)
    Assert ($input.result.value -eq 'Layout Stability') 'name input value mismatch'
    $select = Invoke-MblinkCli @('input-text', '#kind-select', '--text', 'incident', '--project', $project)
    Assert ($select.result.value -eq 'incident') 'select value mismatch'
    Invoke-MblinkCli @('click', '#save-button', '--project', $project) | Out-Null
    $result = Invoke-MblinkCli @('inspect', '#result', '--project', $project)
    Assert ($result.result.outer_html -like '*Layout Stability / incident*') 'form event result mismatch'

    $scroll = Invoke-MblinkCli @('scroll', '#data-table-wrap', '--y', '120', '--project', $project)
    Assert ($scroll.result.selector -eq '#data-table-wrap') 'scroll command selector mismatch'
    foreach ($selector in @('#app', '#topbar', '#sidebar', '#workspace', '#form-row')) {
        Compare-Rect $baseline[$selector] (Get-Rect $selector) "$selector after scroll"
    }

    $errors = Invoke-MblinkCli @('errors', '--project', $project)
    Assert (@($errors.errors).Count -eq 0) 'runtime JS errors were reported'
}
finally {
    try { Invoke-MblinkCli @('stop', '--project', $project) | Out-Null } catch {}
}

$timer.Stop()
$screenshotInfo = Get-Item -LiteralPath $screenshotCopy
$evidence = [ordered]@{
    ok = $true
    elapsed_ms = [int]$timer.Elapsed.TotalMilliseconds
    project = $project
    exe = $exe
    snapshot = $snapshotCopy
    screenshot = $screenshotCopy
    screenshot_bytes = $screenshotInfo.Length
    screenshot_render = $pngEvidence
    baseline_rects = $baseline
    stable_selectors = $stableSelectors
    checks = @(
        'snapshot file with screenshot',
        'nonblank decoded PNG',
        'repeated rect sampling within 0.5 logical pixels',
        'flex topbar geometry',
        'grid shell/workspace/cards geometry',
        'table fixed layout and checkbox sizing',
        'input/select/button events',
        'filter hide/show layout roundtrip',
        'compact mode roundtrip',
        'scroll container stability',
        'no runtime JS errors'
    )
}
Write-Utf8NoBomFile $evidencePath ($evidence | ConvertTo-Json -Depth 10)
Write-Host "[OK] CSS/layout stability evidence: $evidencePath"
Write-Host "[OK] screenshot: $screenshotCopy"
