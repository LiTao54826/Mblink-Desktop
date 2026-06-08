// Regression check: Python host packaging must include local source roots and native packages.
// Usage: node tests/js/mbink_ui_dev_python_packaging_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const src = fs.readFileSync(
    path.join(process.cwd(), 'tools', 'mbink_ui_dev', 'daemon_server.cpp'),
    'utf8'
  );

  assert(src.includes('PythonImportSearchRoots') &&
         src.includes('project_root / "src"') &&
         src.includes('JoinProjectPath(project_root, config.src_dir)') &&
         src.includes('project_root / "vendor"'),
    'Python PyInstaller packaging must add project, src, configured src_dir, host, and vendor roots');

  assert(src.includes('IsPythonNativeBinary') &&
         src.includes('ext == ".pyd"') &&
         src.includes('ScanProjectPythonPackages') &&
         src.includes('--collect-all') &&
         src.includes('python_collect_all'),
    'Python PyInstaller packaging must collect local packages that contain native binaries');

  assert(src.includes('PythonVersionFromAbiTag') &&
         src.includes('FindPythonLauncher(python_package_scan.required_python_version)') &&
         src.includes('python_required_version'),
    'Python PyInstaller packaging must infer the launcher version from cpXY native extension tags');

  console.log('[PASS] mbink-ui-dev Python packaging guard is present');
}

run();
