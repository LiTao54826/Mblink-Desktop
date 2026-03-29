# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['examples\\borderless_demo.py'],
    pathex=['.'],
    binaries=[('mbink\\bin\\mbink_core.pyd', 'mbink\\bin')],
    datas=[
        ('mbink\\__init__.py', 'mbink'),
        ('mbink\\app.py', 'mbink'),
        ('mbink\\core.py', 'mbink'),
    ],
    hiddenimports=['mbink', 'mbink.app', 'mbink.core', 'mbink_core'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='borderless_demo',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
