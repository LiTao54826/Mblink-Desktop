// Regression check: mblink-ui-dev host builds must package configured Windows exe icons.
// Usage: node tests/js/mblink_ui_dev_icon_packaging_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function readRepoFile(...parts) {
  return fs.readFileSync(path.join(process.cwd(), ...parts), 'utf8');
}

function run() {
  const commonHeader = readRepoFile('tools', 'mblink_ui_dev', 'common.h');
  const commonSrc = readRepoFile('tools', 'mblink_ui_dev', 'common.cpp');
  const daemonSrc = readRepoFile('tools', 'mblink_ui_dev', 'daemon_server.cpp');

  assert(commonHeader.includes('std::string build_icon;'),
    'ProjectConfig must carry build_icon');
  assert(commonSrc.includes('{"icon", p.build_icon}') &&
         commonSrc.includes('p.build_icon = b.value("icon", p.build_icon)') &&
         commonSrc.includes('cfg.build_icon = normalize_rel(b.value("icon", cfg.build_icon))'),
    'ProjectConfig JSON and mblink.config.json loading must include build.icon');
  assert(daemonSrc.includes('a.build_icon == b.build_icon'),
    'MaterialConfigEquals must include build_icon so config changes trigger rebuilds');

  assert(commonSrc.includes('ShouldApplyEmbeddedTemplateVariables') &&
         commonSrc.includes('lower_ext != ".ico"') &&
         commonSrc.includes('lower_ext != ".lib"') &&
         commonSrc.includes('lower_ext != ".mbrp"') &&
         commonSrc.includes('EmbeddedTemplateContent(*entry, rel_path, root, project_name)'),
    'Embedded template materialization must skip binary template substitutions');

  for (const token of [
    'ApplyBuildIconToExe',
    'ValidateIcoFile',
    'InjectExeIconResource',
    'BeginUpdateResourceW',
    'UpdateResourceW',
    'RT_GROUP_ICON',
    '"status", "none"',
    '"status", "skipped"',
    '"status"] = "applied"',
    '"reason"] = "missing"',
    '"reason"] = "not_ico"',
    '"reason"] = "invalid_ico"',
    '"reason"] = "inject_failed"',
    '"reason"] = "unsupported_platform"',
  ]) {
    assert(daemonSrc.includes(token), `daemon_server.cpp must include icon packaging token: ${token}`);
  }

  for (const functionName of ['BuildRustHostArtifact', 'BuildGoHostArtifact', 'BuildPythonHostArtifact']) {
    const start = daemonSrc.indexOf(`nlohmann::json ${functionName}`);
    assert(start >= 0, `${functionName} must exist`);
    const next = daemonSrc.indexOf('\nnlohmann::json ', start + 1);
    const body = daemonSrc.slice(start, next >= 0 ? next : undefined);
    assert(body.includes('SetWindowsGuiSubsystem(final_exe, config.build_hide_console, error)'),
      `${functionName} must keep subsystem handling`);
    assert(body.includes('status["icon"] = ApplyBuildIconToExe(project_root, config, final_exe)'),
      `${functionName} must report final exe icon packaging status`);
    const iconCall = body.indexOf('status["icon"] = ApplyBuildIconToExe(project_root, config, final_exe)');
    const afterIconCall = body.slice(iconCall, iconCall + 240);
    assert(!afterIconCall.includes('status["ok"] = false') && !afterIconCall.includes('failed to set'),
      `${functionName} must not fail the build when icon packaging is skipped`);
  }

  const purposes = ['minimal', 'showcase', 'desktop-app'];
  const runtimes = ['python', 'rust', 'go'];
  for (const purpose of purposes) {
    for (const runtime of runtimes) {
      const configPath = path.join(
        process.cwd(),
        'tools',
        'mblink_ui_dev',
        'templates',
        'purpose',
        purpose,
        'runtime',
        runtime,
        'mblink.config.json'
      );
      const config = JSON.parse(fs.readFileSync(configPath, 'utf8').replace(/^\uFEFF/, ''));
      assert(config.build && config.build.icon === 'assets/app.ico',
        `${purpose}/${runtime} template config must set build.icon`);
    }
  }

  for (const runtime of runtimes) {
    const iconPath = path.join(
      process.cwd(),
      'tools',
      'mblink_ui_dev',
      'templates',
      'runtime',
      runtime,
      'assets',
      'app.ico'
    );
    const data = fs.readFileSync(iconPath);
    assert(data.length > 22, `${runtime} default app.ico must not be empty`);
    assert(data.readUInt16LE(0) === 0 && data.readUInt16LE(2) === 1 && data.readUInt16LE(4) > 0,
      `${runtime} default app.ico must have a valid ICO header`);
  }

  const purposeIconRoots = purposes.flatMap((purpose) =>
    runtimes.map((runtime) =>
      path.join(process.cwd(), 'tools', 'mblink_ui_dev', 'templates', 'purpose', purpose, 'runtime', runtime)
    )
  );
  for (const root of purposeIconRoots) {
    const duplicated = path.join(root, 'assets', 'app.ico');
    assert(!fs.existsSync(duplicated), `default app.ico must live in shared runtime layers only: ${duplicated}`);
  }

  const cliReference = readRepoFile(
    'tools', 'mblink_ui_dev', 'skills', 'mblink-ui-dev', 'references', 'cli-mcp-reference.md'
  );
  const compatibility = readRepoFile(
    'tools', 'mblink_ui_dev', 'skills', 'mblink-ui-dev', 'references', 'compatibility-guidelines.md'
  );
  assert(cliReference.includes('build.icon = "assets/app.ico"') &&
         compatibility.includes('apply it only to the final Windows `.exe` resource icon') &&
         compatibility.includes('must not fail an otherwise successful build'),
    'mblink-ui-dev docs must describe build.icon and skip semantics');

  console.log('[PASS] mblink-ui-dev icon packaging guard is present');
}

run();
