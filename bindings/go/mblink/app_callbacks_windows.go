//go:build windows && cgo

package mblink

/*
#include <stdbool.h>
#include <stdint.h>
#include "mblink.h"

extern char* go_mblink_bind_trampoline(const char* args_json, void* user_data);
extern char* go_mblink_bind_async_trampoline(const char* args_json, void* user_data);
extern void go_mblink_state_watch_trampoline(const char* name, const char* value_json, void* user_data);
extern void go_mblink_resize_trampoline(int width, int height, void* user_data);
extern void go_mblink_void_trampoline(void* user_data);
extern bool go_mblink_bool_trampoline(void* user_data);
extern void go_mblink_update_trampoline(float delta_time, void* user_data);

static inline int mblink_go_bind(MBlinkHandle handle, const char* name, void* user_data) {
	return mblink_bind(handle, name, go_mblink_bind_trampoline, user_data);
}
static inline int mblink_go_bind_async(MBlinkHandle handle, const char* name, void* user_data) {
	return mblink_bind_async(handle, name, go_mblink_bind_async_trampoline, user_data);
}
static inline int mblink_go_tray_set_left_click_callback(MBlinkHandle handle, void* user_data) {
	return mblink_tray_set_left_click_callback(handle, go_mblink_void_trampoline, user_data);
}
static inline int mblink_go_tray_set_menu_callback(MBlinkHandle handle, void* user_data) {
	return mblink_tray_set_menu_callback(handle, go_mblink_bind_trampoline, user_data);
}
static inline int mblink_go_on_resize(MBlinkHandle handle, void* user_data) {
	return mblink_on_resize(handle, go_mblink_resize_trampoline, user_data);
}
static inline int mblink_go_on_close(MBlinkHandle handle, void* user_data) {
	return mblink_on_close(handle, go_mblink_void_trampoline, user_data);
}
static inline int mblink_go_on_close_request(MBlinkHandle handle, void* user_data) {
	return mblink_on_close_request(handle, go_mblink_bool_trampoline, user_data);
}
static inline int mblink_go_on_focus(MBlinkHandle handle, void* user_data) {
	return mblink_on_focus(handle, go_mblink_void_trampoline, user_data);
}
static inline int mblink_go_on_blur(MBlinkHandle handle, void* user_data) {
	return mblink_on_blur(handle, go_mblink_void_trampoline, user_data);
}
static inline int mblink_go_on_update(MBlinkHandle handle, void* user_data) {
	return mblink_on_update(handle, go_mblink_update_trampoline, user_data);
}
static inline int mblink_go_state_watch(MBlinkHandle handle, const char* name, void* user_data) {
	return mblink_state_watch(handle, name, go_mblink_state_watch_trampoline, user_data);
}
*/
import "C"

func (a *App) Bind(name string, fn func(any) (any, error)) error {
	nameC, done := cString(name)
	defer done()
	id := registerBind(fn, false)
	err := checkRC(C.mblink_go_bind(a.handle, nameC, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.bindIDs[name] = id
	return nil
}

func (a *App) BindAsync(name string, fn func(any) (any, error)) error {
	nameC, done := cString(name)
	defer done()
	id := registerBind(fn, true)
	err := checkRC(C.mblink_go_bind_async(a.handle, nameC, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.bindIDs[name] = id
	return nil
}

func (a *App) Unbind(name string) error {
	nameC, done := cString(name)
	defer done()
	C.mblink_unbind(a.handle, nameC)
	if id, ok := a.bindIDs[name]; ok {
		releaseCallback(id)
		delete(a.bindIDs, name)
	}
	return nil
}

func (a *App) OnTrayClick(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mblink_go_tray_set_left_click_callback(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnTrayMenu(fn func(any) (any, error)) error {
	id := registerBind(fn, false)
	err := checkRC(C.mblink_go_tray_set_menu_callback(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnResize(fn func(int, int)) error {
	id := registerResize(fn)
	err := checkRC(C.mblink_go_on_resize(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnClose(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mblink_go_on_close(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnCloseRequest(fn func() bool) error {
	id := registerBool(fn)
	err := checkRC(C.mblink_go_on_close_request(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnFocus(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mblink_go_on_focus(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}
func (a *App) OnBlur(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mblink_go_on_blur(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}
func (a *App) OnUpdate(fn func(float32)) error {
	id := registerUpdate(fn)
	err := checkRC(C.mblink_go_on_update(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) installDefaultOnCloseStop() error {
	id := registerVoid(func() {
		C.mblink_stop(a.handle)
	})
	err := checkRC(C.mblink_go_on_close(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) WatchState(name string, fn func(string, any)) (int, error) {
	nameC, done := cString(name)
	defer done()
	id := registerWatcher(fn)
	watchID := C.mblink_go_state_watch(a.handle, nameC, callbackPtr(id))
	if int(watchID) < 0 {
		releaseCallback(id)
		return 0, checkRC(watchID)
	}
	a.watchIDs[int(watchID)] = id
	return int(watchID), nil
}

func (a *App) UnwatchState(watchID int) {
	C.mblink_state_unwatch(a.handle, C.int(watchID))
	if id, ok := a.watchIDs[watchID]; ok {
		releaseCallback(id)
		delete(a.watchIDs, watchID)
	}
}
