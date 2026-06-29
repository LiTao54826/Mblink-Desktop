//go:build windows && cgo

package mblink

/*
#include <stdlib.h>
#include <stdbool.h>
#include "mblink.h"

typedef struct {
    const char* runtime_epoch;
    size_t max_nodes;
    int max_depth;
    const char* root_selector;
    bool include_screenshot;
    bool inline_screenshot;
    const char* screenshot_file;
} MBlinkUiDevSnapshotOptions;

extern MBlinkUiDevSnapshotOptions go_mblink_ui_dev_default_snapshot_options_call(void);
extern int go_mblink_ui_dev_snapshot_json_call(MBlinkHandle h, const MBlinkUiDevSnapshotOptions* o, char** out);
extern int go_mblink_ui_dev_snapshot_file_call(MBlinkHandle h, const char* p, const MBlinkUiDevSnapshotOptions* o);
extern int go_mblink_ui_dev_command_json_call(MBlinkHandle h, const char* c, char** out);
*/
import "C"

import (
	"encoding/json"
	"unsafe"
)

func cString(value string) (*C.char, func()) {
	ptr := C.CString(value)
	return ptr, func() { C.free(unsafe.Pointer(ptr)) }
}

func (a *App) LoadHTML(html string) error {
	v, done := cString(html)
	defer done()
	return checkRC(C.mblink_load_html(a.handle, v))
}
func (a *App) LoadHTMLFile(path string) error {
	v, done := cString(path)
	defer done()
	return checkRC(C.mblink_load_html_file(a.handle, v))
}
func (a *App) LoadEntryFile(path string, executeHTMLScripts bool) error {
	v, done := cString(path)
	defer done()
	return checkRC(C.mblink_load_entry_file(a.handle, v, boolToC(executeHTMLScripts)))
}
func (a *App) EvalJS(code string) error {
	v, done := cString(code)
	defer done()
	return checkRC(C.mblink_eval_js(a.handle, v))
}
func (a *App) EvalModule(code, filename string) error {
	cv, cd := cString(code)
	defer cd()
	fv, fd := cString(filename)
	defer fd()
	return checkRC(C.mblink_eval_module(a.handle, cv, fv))
}
func (a *App) LoadJSFile(path string) error {
	v, done := cString(path)
	defer done()
	return checkRC(C.mblink_load_js_file(a.handle, v))
}
func (a *App) LoadModuleFile(path string) error {
	v, done := cString(path)
	defer done()
	return checkRC(C.mblink_load_module_file(a.handle, v))
}
func (a *App) LoadBytecode(data []byte) error {
	if len(data) == 0 {
		return checkRC(C.mblink_load_bytecode(a.handle, nil, 0))
	}
	return checkRC(C.mblink_load_bytecode(a.handle, unsafe.Pointer(&data[0]), C.size_t(len(data))))
}
func (a *App) MountResourcePackage(file, key, mount string) error {
	fv, fd := cString(file)
	defer fd()
	kv, kd := cString(key)
	defer kd()
	mv, md := cString(mount)
	defer md()
	return checkRC(C.mblink_mount_resource_package(a.handle, fv, kv, mv))
}
func (a *App) ConfigureRuntime(options RuntimeOptions) error {
	raw := C.mblink_default_runtime_options()
	var epoch *C.char
	if options.RuntimeEpoch != "" {
		epoch, _ = cString(options.RuntimeEpoch)
		defer C.free(unsafe.Pointer(epoch))
		raw.runtime_epoch = epoch
	}
	raw.load_embedded_runtime = boolToC(options.LoadEmbeddedRuntime)
	raw.load_official_preact = boolToC(options.LoadOfficialPreact)
	return checkRC(C.mblink_configure_runtime(a.handle, &raw))
}
func (a *App) LoadEmbeddedRuntime(includeOfficialPreact bool) error {
	return checkRC(C.mblink_load_embedded_runtime(a.handle, boolToC(includeOfficialPreact)))
}
func (a *App) RenderFrame(passes int) error {
	return checkRC(C.mblink_render_frame(a.handle, C.int(passes)))
}
func (a *App) RuntimeEpoch() (string, error) {
	var out *C.char
	if err := checkRC(C.mblink_runtime_epoch(a.handle, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) LifecycleState() LifecycleState {
	return LifecycleState(C.mblink_lifecycle_state(a.handle))
}
func (a *App) LifecycleReason() (string, error) {
	var out *C.char
	if err := checkRC(C.mblink_lifecycle_reason(a.handle, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) ObserveConsoleJSON() (string, error) {
	var out *C.char
	if err := checkRC(C.mblink_observe_console_json(a.handle, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) ObserveErrorsJSON() (string, error) {
	var out *C.char
	if err := checkRC(C.mblink_observe_errors_json(a.handle, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) ObserveLifecycleJSON() (string, error) {
	var out *C.char
	if err := checkRC(C.mblink_observe_lifecycle_json(a.handle, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) ObserveConsole() (any, error) {
	text, err := a.ObserveConsoleJSON()
	if err != nil {
		return nil, err
	}
	return parseJSONText(text), nil
}
func (a *App) ObserveErrors() (any, error) {
	text, err := a.ObserveErrorsJSON()
	if err != nil {
		return nil, err
	}
	return parseJSONText(text), nil
}
func (a *App) ObserveLifecycle() (any, error) {
	text, err := a.ObserveLifecycleJSON()
	if err != nil {
		return nil, err
	}
	return parseJSONText(text), nil
}
func (a *App) ObserveClear(kind ObserveKind) error {
	return checkRC(C.mblink_observe_clear(a.handle, C.MBlinkObserveKind(kind)))
}
func (a *App) UiDevSnapshotJSON(options UiDevSnapshotOptions) (string, error) {
	if err := ensureDevtoolsRuntime(); err != nil {
		return "", err
	}
	raw := C.go_mblink_ui_dev_default_snapshot_options_call()
	keepalive := make([]func(), 0, 3)
	if options.RuntimeEpoch != "" {
		v, done := cString(options.RuntimeEpoch)
		keepalive = append(keepalive, done)
		raw.runtime_epoch = v
	}
	if options.MaxNodes != 0 {
		raw.max_nodes = C.size_t(options.MaxNodes)
	}
	if options.MaxDepth != 0 {
		raw.max_depth = C.int(options.MaxDepth)
	}
	if options.RootSelector != "" {
		v, done := cString(options.RootSelector)
		keepalive = append(keepalive, done)
		raw.root_selector = v
	}
	raw.include_screenshot = boolToC(options.IncludeScreenshot)
	raw.inline_screenshot = boolToC(options.InlineScreenshot)
	if options.ScreenshotFile != "" {
		v, done := cString(options.ScreenshotFile)
		keepalive = append(keepalive, done)
		raw.screenshot_file = v
	}
	defer func() {
		for i := len(keepalive) - 1; i >= 0; i-- {
			keepalive[i]()
		}
	}()
	var out *C.char
	if err := checkRC(C.go_mblink_ui_dev_snapshot_json_call(a.handle, &raw, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) UiDevSnapshot(options UiDevSnapshotOptions) (any, error) {
	text, err := a.UiDevSnapshotJSON(options)
	if err != nil {
		return nil, err
	}
	return parseJSONText(text), nil
}
func (a *App) UiDevSnapshotFile(path string, options UiDevSnapshotOptions) error {
	if err := ensureDevtoolsRuntime(); err != nil {
		return err
	}
	raw := C.go_mblink_ui_dev_default_snapshot_options_call()
	var runtimeEpoch *C.char
	if options.RuntimeEpoch != "" {
		runtimeEpoch, _ = cString(options.RuntimeEpoch)
		defer C.free(unsafe.Pointer(runtimeEpoch))
		raw.runtime_epoch = runtimeEpoch
	}
	if options.MaxNodes != 0 {
		raw.max_nodes = C.size_t(options.MaxNodes)
	}
	if options.MaxDepth != 0 {
		raw.max_depth = C.int(options.MaxDepth)
	}
	var rootSelector *C.char
	if options.RootSelector != "" {
		rootSelector, _ = cString(options.RootSelector)
		defer C.free(unsafe.Pointer(rootSelector))
		raw.root_selector = rootSelector
	}
	raw.include_screenshot = boolToC(options.IncludeScreenshot)
	raw.inline_screenshot = boolToC(options.InlineScreenshot)
	var screenshotFile *C.char
	if options.ScreenshotFile != "" {
		screenshotFile, _ = cString(options.ScreenshotFile)
		defer C.free(unsafe.Pointer(screenshotFile))
		raw.screenshot_file = screenshotFile
	}
	pv, pd := cString(path)
	defer pd()
	return checkRC(C.go_mblink_ui_dev_snapshot_file_call(a.handle, pv, &raw))
}
func (a *App) UiDevCommandJSON(commandJSON string) (string, error) {
	if err := ensureDevtoolsRuntime(); err != nil {
		return "", err
	}
	cmd, done := cString(commandJSON)
	defer done()
	var out *C.char
	if err := checkRC(C.go_mblink_ui_dev_command_json_call(a.handle, cmd, &out)); err != nil {
		return "", err
	}
	return takeOwnedString(out), nil
}
func (a *App) UiDevCommand(command any) (any, error) {
	var payload string
	switch v := command.(type) {
	case string:
		payload = v
	default:
		data, err := json.Marshal(command)
		if err != nil {
			return nil, err
		}
		payload = string(data)
	}
	text, err := a.UiDevCommandJSON(payload)
	if err != nil {
		return nil, err
	}
	return parseJSONText(text), nil
}
func (a *App) SetTitle(title string) error {
	v, done := cString(title)
	defer done()
	return checkRC(C.mblink_set_title(a.handle, v))
}
func (a *App) CreateTray(tooltip string) error {
	v, done := cString(tooltip)
	defer done()
	return checkRC(C.mblink_tray_create(a.handle, v))
}
func (a *App) DestroyTray() error { return checkRC(C.mblink_tray_destroy(a.handle)) }
func (a *App) SetTrayTooltip(tooltip string) error {
	v, done := cString(tooltip)
	defer done()
	return checkRC(C.mblink_tray_set_tooltip(a.handle, v))
}
func (a *App) SetTrayMenuJSON(menuJSON string) error {
	v, done := cString(menuJSON)
	defer done()
	return checkRC(C.mblink_tray_set_menu(a.handle, v))
}
func (a *App) SetTrayMenu(menu any) error { return a.SetTrayMenuJSON(mustJSON(menu)) }
func (a *App) Emit(event string, value any) error {
	ev, ed := cString(event)
	defer ed()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mblink_emit(a.handle, ev, jv))
}
func (a *App) SetSize(width, height int) error {
	return checkRC(C.mblink_set_size(a.handle, C.int(width), C.int(height)))
}
func (a *App) Size() (int, int, error) {
	var w, h C.int
	if err := checkRC(C.mblink_get_size(a.handle, &w, &h)); err != nil {
		return 0, 0, err
	}
	return int(w), int(h), nil
}
func (a *App) SetPosition(x, y int) error {
	return checkRC(C.mblink_set_position(a.handle, C.int(x), C.int(y)))
}
func (a *App) Position() (int, int, error) {
	var x, y C.int
	if err := checkRC(C.mblink_get_position(a.handle, &x, &y)); err != nil {
		return 0, 0, err
	}
	return int(x), int(y), nil
}
func (a *App) SetMinSize(width, height int) error {
	return checkRC(C.mblink_set_min_size(a.handle, C.int(width), C.int(height)))
}
func (a *App) SetMaxSize(width, height int) error {
	return checkRC(C.mblink_set_max_size(a.handle, C.int(width), C.int(height)))
}
func (a *App) Show() error     { return checkRC(C.mblink_show(a.handle)) }
func (a *App) Hide() error     { return checkRC(C.mblink_hide(a.handle)) }
func (a *App) Minimize() error { return checkRC(C.mblink_minimize(a.handle)) }
func (a *App) Maximize() error { return checkRC(C.mblink_maximize(a.handle)) }
func (a *App) Restore() error  { return checkRC(C.mblink_restore(a.handle)) }
func (a *App) SetFullscreen(v bool) error {
	return checkRC(C.mblink_set_fullscreen(a.handle, boolToC(v)))
}
func (a *App) SetResizable(v bool) error { return checkRC(C.mblink_set_resizable(a.handle, boolToC(v))) }
func (a *App) SetBorderless(v bool) error {
	return checkRC(C.mblink_set_borderless(a.handle, boolToC(v)))
}
func (a *App) SetAlwaysOnTop(v bool) error {
	return checkRC(C.mblink_set_always_on_top(a.handle, boolToC(v)))
}
func (h AppHandle) Show() error    { return checkRC(C.mblink_show(h.handle)) }
func (h AppHandle) Hide() error    { return checkRC(C.mblink_hide(h.handle)) }
func (h AppHandle) Restore() error { return checkRC(C.mblink_restore(h.handle)) }
func (h AppHandle) SetTitle(title string) error {
	v, d := cString(title)
	defer d()
	return checkRC(C.mblink_set_title(h.handle, v))
}
func (h AppHandle) SetAlwaysOnTop(v bool) error {
	return checkRC(C.mblink_set_always_on_top(h.handle, boolToC(v)))
}
