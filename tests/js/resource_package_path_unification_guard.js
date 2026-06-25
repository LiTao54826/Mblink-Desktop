// Regression check: compiled resource names must match the caller-visible input path.
// Usage: node tests/js/resource_package_path_unification_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const resourcePackagePath = path.join(process.cwd(), 'core', 'api', 'resource_package.cpp');
  const resourcePackageSrc = fs.readFileSync(resourcePackagePath, 'utf8');

  assert(resourcePackageSrc.includes('std::string NormalizeResourceName(const fs::path& path)') &&
         resourcePackageSrc.includes('return NormalizeResourceName(Utf8PathToFsPath(path));'),
    'Resource package paths must normalize through UTF-8 filesystem conversion');
  assert(!resourcePackageSrc.includes('fs::path(resource_path).generic_string()'),
    'Resource package loading must not use narrow filesystem path conversion for UTF-8 resource paths');
  assert(resourcePackageSrc.includes('fs::path DirectoryResourceRootName(const fs::path& input)') &&
         resourcePackageSrc.includes('resource_name = resource_root / fs::relative(file, base);'),
    'Directory resource packages must preserve the compiled input directory as the virtual root');
  assert(!resourcePackageSrc.includes('IsBoundarySuffixMatch') &&
         !resourcePackageSrc.includes('IsResourceNameMatch') &&
         !resourcePackageSrc.includes('ResourceNameDistance') &&
         !resourcePackageSrc.includes('PreferSuffixMatch') &&
         !resourcePackageSrc.includes('best_suffix'),
    'Resource package loading must not keep suffix compatibility; callers must use the compiled virtual path exactly');

  const apiPath = path.join(process.cwd(), 'core', 'api', 'mblink.cpp');
  const apiSrc = fs.readFileSync(apiPath, 'utf8');
  assert(apiSrc.includes('Utf8PathToFsPath(NormalizeResourcePath(filepath)).parent_path()'),
    'Mounted HTML base path normalization must remain UTF-8 safe on Windows');

  const uiDevPath = path.join(process.cwd(), 'tools', 'mblink_ui_dev', 'daemon_server.cpp');
  const uiDevSrc = fs.readFileSync(uiDevPath, 'utf8');
  assert(uiDevSrc.includes('JoinProjectPath(project_root, config.out_dir) / "app"'),
    'mblink-ui-dev resource input directory must match the /app.js runtime virtual root');

  const pythonTemplate = fs.readFileSync(
    path.join(process.cwd(), 'tools', 'mblink_ui_dev', 'templates', 'runtime', 'python', 'host', 'main.py'),
    'utf8');
  assert(pythonTemplate.includes('RESOURCE_APP_PATH = "/app/app.js"') &&
         pythonTemplate.includes('RESOURCE_CONFIG_PATH = "app/mblink.config.json"'),
    'Python host template must load resources through the compiled /app virtual root');

  const goTemplate = fs.readFileSync(
    path.join(process.cwd(), 'tools', 'mblink_ui_dev', 'templates', 'runtime', 'go', 'host', 'main.go'),
    'utf8');
  assert(goTemplate.includes('const resourceAppPath = "/app/app.js"') &&
         goTemplate.includes('const resourceConfigPath = "app/mblink.config.json"'),
    'Go host template must load resources through the compiled /app virtual root');

  const rustTemplate = fs.readFileSync(
    path.join(process.cwd(), 'tools', 'mblink_ui_dev', 'templates', 'runtime', 'rust', 'rust_host', 'src', 'main.rs'),
    'utf8');
  assert(rustTemplate.includes('const RESOURCE_APP_PATH: &str = "/app/app.js"') &&
         rustTemplate.includes('app.load_js_file(RESOURCE_APP_PATH)'),
    'Rust host template must load resources through the compiled /app virtual root');

  console.log('[PASS] resource package path unification guard is present');
}

run();
