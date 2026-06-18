// Regression check: mbink_poll_events() must remain a cheap non-blocking poll.

const { spawnSync } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const dll = path.join(process.cwd(), 'build', 'bin', 'Release', 'mbink.dll');
  assert(fs.existsSync(dll), `Missing ${dll}; build Release mbink_api first`);

  const scriptPath = path.join(os.tmpdir(), `mbink-poll-latency-${process.pid}.ps1`);
  const script = String.raw`
Add-Type -TypeDefinition @"
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
public struct MBinkConfig {
    public IntPtr title;
    public int width;
    public int height;
    [MarshalAs(UnmanagedType.I1)] public bool headless;
    [MarshalAs(UnmanagedType.I1)] public bool borderless;
    [MarshalAs(UnmanagedType.I1)] public bool transparent;
    [MarshalAs(UnmanagedType.I1)] public bool always_on_top;
    [MarshalAs(UnmanagedType.I1)] public bool resizable;
    [MarshalAs(UnmanagedType.I1)] public bool gpu;
    [MarshalAs(UnmanagedType.I1)] public bool fullscreen;
    public int resize_border_width;
    public int min_width;
    public int min_height;
    public int max_width;
    public int max_height;
}

public static class Native {
    [DllImport("${dll.replace(/\\/g, '\\\\')}", CallingConvention = CallingConvention.Cdecl)]
    public static extern int mbink_init();
    [DllImport("${dll.replace(/\\/g, '\\\\')}", CallingConvention = CallingConvention.Cdecl)]
    public static extern void mbink_cleanup();
    [DllImport("${dll.replace(/\\/g, '\\\\')}", CallingConvention = CallingConvention.Cdecl)]
    public static extern MBinkConfig mbink_default_config();
    [DllImport("${dll.replace(/\\/g, '\\\\')}", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr mbink_create_ex(ref MBinkConfig config);
    [DllImport("${dll.replace(/\\/g, '\\\\')}", CallingConvention = CallingConvention.Cdecl)]
    public static extern void mbink_destroy(IntPtr handle);
    [DllImport("${dll.replace(/\\/g, '\\\\')}", CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    public static extern bool mbink_poll_events(IntPtr handle);
}
"@

$title = [Runtime.InteropServices.Marshal]::StringToHGlobalAnsi("poll latency")
$handle = [IntPtr]::Zero
try {
    [void][Native]::mbink_init()
    $cfg = [Native]::mbink_default_config()
    $cfg.title = $title
    $cfg.width = 120
    $cfg.height = 80
    $cfg.headless = $true
    $cfg.gpu = $false
    $handle = [Native]::mbink_create_ex([ref]$cfg)
    if ($handle -eq [IntPtr]::Zero) { throw "mbink_create_ex returned null" }
    [void][Native]::mbink_poll_events($handle)
    $maxMs = 0.0
    for ($i = 0; $i -lt 20; $i++) {
        $sw = [Diagnostics.Stopwatch]::StartNew()
        [void][Native]::mbink_poll_events($handle)
        $sw.Stop()
        if ($sw.Elapsed.TotalMilliseconds -gt $maxMs) {
            $maxMs = $sw.Elapsed.TotalMilliseconds
        }
    }
    Write-Output ("max_poll_ms={0:F3}" -f $maxMs)
    if ($maxMs -ge 20.0) { exit 1 }
} finally {
    if ($handle -ne [IntPtr]::Zero) { [Native]::mbink_destroy($handle) }
    [Runtime.InteropServices.Marshal]::FreeHGlobal($title)
    [Native]::mbink_cleanup()
}
`;
  fs.writeFileSync(scriptPath, script);

  const result = spawnSync('powershell.exe', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', scriptPath], {
    cwd: process.cwd(),
    encoding: 'utf8',
    timeout: 10000,
  });
  try { fs.unlinkSync(scriptPath); } catch (_) {}

  assert(result.status === 0,
    `mbink_poll_events appears blocking or test failed\nstdout:\n${result.stdout}\nstderr:\n${result.stderr}`);

  const line = result.stdout
    .split(/\r?\n/)
    .map((v) => v.trim())
    .find((v) => v.startsWith('max_poll_ms='));
  console.log(`[PASS] mbink_poll_events latency guard (${line || 'ok'})`);
}

run();
