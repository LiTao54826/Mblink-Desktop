// Regression check: mblink-ui-dev builds must preserve static media assets for
// audio/image elements that resolve paths relative to the JS entry.
// Usage: node tests/js/mblink_ui_dev_static_audio_assets_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const uiDevSrc = fs.readFileSync(
    path.join(process.cwd(), 'tools', 'mblink_ui_dev', 'daemon_server.cpp'),
    'utf8'
  );

  assert(uiDevSrc.includes('CopyUiStaticAssets') &&
         uiDevSrc.includes('project_root / "assets"') &&
         uiDevSrc.includes('std::filesystem::path("assets")'),
    'mblink-ui-dev must copy src/public/assets static resources into build outputs');

  assert(uiDevSrc.includes('IsBundledSourceFile') &&
         uiDevSrc.includes('ext == ".js"') &&
         uiDevSrc.includes('ext == ".tsx"') &&
         uiDevSrc.includes('ext == ".html"'),
    'static resource copying must skip source files that esbuild owns');

  assert(uiDevSrc.includes('CopyUiStaticAssets(project_root, config, out_dir') &&
         uiDevSrc.includes('CopyUiStaticAssets(project_root, config, input_dir') &&
         uiDevSrc.includes('"static_assets"'),
    'static resources must be copied both beside App.js and into the app.mbrp input directory');

  const apiSrc = fs.readFileSync(
    path.join(process.cwd(), 'core', 'api', 'mblink.cpp'),
    'utf8'
  );
  assert(apiSrc.includes('SetRuntimeBasePath(ctx, next_base_path)') &&
         apiSrc.includes('Utf8PathToFsPath(NormalizeResourcePath(filepath)).parent_path()') &&
         apiSrc.includes('fs::absolute(Utf8PathToFsPath(path)).parent_path()'),
    'mblink_load_js_file must set the document/fetch/image base path to the JS entry directory');

  const loaderSrc = fs.readFileSync(
    path.join(process.cwd(), 'tools', 'esm_loader', 'main.cpp'),
    'utf8'
  );
  assert(loaderSrc.includes('std::string module_base_path = NormalizeFsPath(abs_path.parent_path())') &&
         loaderSrc.includes('document->SetBasePath(module_base_path)') &&
         loaderSrc.includes('FetchBindings::SetBasePath(module_base_path)') &&
         loaderSrc.includes('ImageLoader::SetBasePath(module_base_path)'),
    'esm_loader JS mode must resolve audio/image/fetch resources relative to the entry module');

  console.log('[PASS] mblink-ui-dev static audio asset guard is present');
}

run();
