//go:build windows

package mbink

/*
#include <stdlib.h>
#include <windows.h>
#include "mbink.h"

static inline unsigned long mbink_go_current_thread_id(void) {
	return GetCurrentThreadId();
}
*/
import "C"

import (
	"runtime"
	"unsafe"
)

type controlHandle struct {
	kind   string
	handle unsafe.Pointer
}

type App struct {
	handle          C.MBinkHandle
	sharedHandles   []C.MBinkSharedHandle
	controls        []controlHandle
	bindIDs         map[string]uintptr
	watchIDs        map[int]uintptr
	eventCallbackID []uintptr
	ownerThreadID   uint32
	threadLocked    bool
}

type AppHandle struct {
	handle C.MBinkHandle
	app    *App
}

func currentThreadID() uint32 {
	return uint32(C.mbink_go_current_thread_id())
}

func Version() string {
	ensureInit()
	return C.GoString(C.mbink_version())
}

func DefaultConfig() Config {
    ensureInit()
    raw := C.mbink_default_config()
	return Config{
		Title:             C.GoString(raw.title),
		Width:             int(raw.width),
		Height:            int(raw.height),
		Headless:          bool(raw.headless),
		Borderless:        bool(raw.borderless),
		Transparent:       bool(raw.transparent),
		AlwaysOnTop:       bool(raw.always_on_top),
		Resizable:         bool(raw.resizable),
		GPU:               bool(raw.gpu),
		Fullscreen:        bool(raw.fullscreen),
		ResizeBorderWidth: int(raw.resize_border_width),
		MinWidth:          int(raw.min_width),
		MinHeight:         int(raw.min_height),
		MaxWidth:          int(raw.max_width),
		MaxHeight:         int(raw.max_height),
	}
}

func DefaultRuntimeOptions() RuntimeOptions {
	ensureInit()
	raw := C.mbink_default_runtime_options()
	runtimeEpoch := ""
	if raw.runtime_epoch != nil {
		runtimeEpoch = C.GoString(raw.runtime_epoch)
	}
	return RuntimeOptions{
		RuntimeEpoch:        runtimeEpoch,
		LoadEmbeddedRuntime: bool(raw.load_embedded_runtime),
		LoadOfficialPreact:  bool(raw.load_official_preact),
	}
}

func New(title string, width, height int) (*App, error) {
	cfg := DefaultConfig()
	cfg.Title, cfg.Width, cfg.Height = title, width, height
	return NewWithConfig(cfg)
}

func NewWithConfig(cfg Config) (*App, error) {
	runtime.LockOSThread()
	ensureInit()
	title := C.CString(cfg.Title)
	defer C.free(unsafe.Pointer(title))
	raw := C.mbink_default_config()
	raw.title = title
	raw.width, raw.height = C.int(cfg.Width), C.int(cfg.Height)
	raw.headless, raw.borderless = boolToC(cfg.Headless), boolToC(cfg.Borderless)
	raw.transparent, raw.always_on_top = boolToC(cfg.Transparent), boolToC(cfg.AlwaysOnTop)
	raw.resizable, raw.gpu = boolToC(cfg.Resizable), boolToC(cfg.GPU)
	raw.fullscreen = boolToC(cfg.Fullscreen)
	raw.resize_border_width = C.int(cfg.ResizeBorderWidth)
	raw.min_width, raw.min_height = C.int(cfg.MinWidth), C.int(cfg.MinHeight)
	raw.max_width, raw.max_height = C.int(cfg.MaxWidth), C.int(cfg.MaxHeight)
	h := C.mbink_create_ex(&raw)
	if h == nil {
		runtime.UnlockOSThread()
		return nil, newError(-1, lastErrorMessage())
	}
	runtimeOptions := C.mbink_default_runtime_options()
	if err := checkRC(C.mbink_configure_runtime(h, &runtimeOptions)); err != nil {
		C.mbink_destroy(h)
		runtime.UnlockOSThread()
		return nil, err
	}
	app := &App{
		handle:        h,
		bindIDs:       map[string]uintptr{},
		watchIDs:      map[int]uintptr{},
		ownerThreadID: currentThreadID(),
		threadLocked:  true,
	}
	if err := app.installDefaultOnCloseStop(); err != nil {
		app.Close()
		return nil, err
	}
	return app, nil
}

func (a *App) Handle() AppHandle   { return AppHandle{handle: a.handle, app: a} }
func (h AppHandle) Stop()          { C.mbink_stop(h.handle) }
func (a *App) Stop()               { C.mbink_stop(a.handle) }
func (a *App) Run()                { C.mbink_run(a.handle) }
func (a *App) Poll() (bool, error) { return bool(C.mbink_poll_events(a.handle)), nil }

func (a *App) Close() {
	if a == nil || a.handle == nil {
		return
	}
	for _, c := range a.controls {
		if c.kind == "logview" {
			C.mbink_logview_destroy((C.MBinkLogViewHandle)(c.handle))
		} else {
			C.mbink_terminal_destroy((C.MBinkTerminalHandle)(c.handle))
		}
	}
	for _, h := range a.sharedHandles {
		C.mbink_shared_destroy(h)
	}
	for watchID, callbackID := range a.watchIDs {
		C.mbink_state_unwatch(a.handle, C.int(watchID))
		releaseCallback(callbackID)
	}
	for _, id := range a.bindIDs {
		releaseCallback(id)
	}
	for _, id := range a.eventCallbackID {
		releaseCallback(id)
	}
	C.mbink_destroy(a.handle)
	a.handle = nil
	if a.threadLocked && a.ownerThreadID == currentThreadID() {
		a.threadLocked = false
		runtime.UnlockOSThread()
	}
}
