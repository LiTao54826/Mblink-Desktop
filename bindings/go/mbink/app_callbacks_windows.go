//go:build windows

package mbink

/*
#include <stdbool.h>
#include <stdint.h>
#include "mbink.h"

extern char* go_mbink_bind_trampoline(const char* args_json, void* user_data);
extern char* go_mbink_bind_async_trampoline(const char* args_json, void* user_data);
extern void go_mbink_state_watch_trampoline(const char* name, const char* value_json, void* user_data);
extern void go_mbink_resize_trampoline(int width, int height, void* user_data);
extern void go_mbink_void_trampoline(void* user_data);
extern bool go_mbink_bool_trampoline(void* user_data);
extern void go_mbink_update_trampoline(float delta_time, void* user_data);

static inline int mbink_go_bind(MBinkHandle handle, const char* name, void* user_data) {
	return mbink_bind(handle, name, go_mbink_bind_trampoline, user_data);
}
static inline int mbink_go_bind_async(MBinkHandle handle, const char* name, void* user_data) {
	return mbink_bind_async(handle, name, go_mbink_bind_async_trampoline, user_data);
}
static inline int mbink_go_tray_set_left_click_callback(MBinkHandle handle, void* user_data) {
	return mbink_tray_set_left_click_callback(handle, go_mbink_void_trampoline, user_data);
}
static inline int mbink_go_tray_set_menu_callback(MBinkHandle handle, void* user_data) {
	return mbink_tray_set_menu_callback(handle, go_mbink_bind_trampoline, user_data);
}
static inline int mbink_go_on_resize(MBinkHandle handle, void* user_data) {
	return mbink_on_resize(handle, go_mbink_resize_trampoline, user_data);
}
static inline int mbink_go_on_close(MBinkHandle handle, void* user_data) {
	return mbink_on_close(handle, go_mbink_void_trampoline, user_data);
}
static inline int mbink_go_on_close_request(MBinkHandle handle, void* user_data) {
	return mbink_on_close_request(handle, go_mbink_bool_trampoline, user_data);
}
static inline int mbink_go_on_focus(MBinkHandle handle, void* user_data) {
	return mbink_on_focus(handle, go_mbink_void_trampoline, user_data);
}
static inline int mbink_go_on_blur(MBinkHandle handle, void* user_data) {
	return mbink_on_blur(handle, go_mbink_void_trampoline, user_data);
}
static inline int mbink_go_on_update(MBinkHandle handle, void* user_data) {
	return mbink_on_update(handle, go_mbink_update_trampoline, user_data);
}
static inline int mbink_go_state_watch(MBinkHandle handle, const char* name, void* user_data) {
	return mbink_state_watch(handle, name, go_mbink_state_watch_trampoline, user_data);
}
*/
import "C"

func (a *App) Bind(name string, fn func(any) (any, error)) error {
	nameC, done := cString(name)
	defer done()
	id := registerBind(fn, false)
	err := checkRC(C.mbink_go_bind(a.handle, nameC, callbackPtr(id)))
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
	err := checkRC(C.mbink_go_bind_async(a.handle, nameC, callbackPtr(id)))
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
	C.mbink_unbind(a.handle, nameC)
	if id, ok := a.bindIDs[name]; ok {
		releaseCallback(id)
		delete(a.bindIDs, name)
	}
	return nil
}

func (a *App) OnTrayClick(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mbink_go_tray_set_left_click_callback(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnTrayMenu(fn func(any) (any, error)) error {
	id := registerBind(fn, false)
	err := checkRC(C.mbink_go_tray_set_menu_callback(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnResize(fn func(int, int)) error {
	id := registerResize(fn)
	err := checkRC(C.mbink_go_on_resize(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnClose(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mbink_go_on_close(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnCloseRequest(fn func() bool) error {
	id := registerBool(fn)
	err := checkRC(C.mbink_go_on_close_request(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) OnFocus(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mbink_go_on_focus(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}
func (a *App) OnBlur(fn func()) error {
	id := registerVoid(fn)
	err := checkRC(C.mbink_go_on_blur(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}
func (a *App) OnUpdate(fn func(float32)) error {
	id := registerUpdate(fn)
	err := checkRC(C.mbink_go_on_update(a.handle, callbackPtr(id)))
	if err != nil {
		releaseCallback(id)
		return err
	}
	a.eventCallbackID = append(a.eventCallbackID, id)
	return nil
}

func (a *App) installDefaultOnCloseStop() error {
	id := registerVoid(func() {
		C.mbink_stop(a.handle)
	})
	err := checkRC(C.mbink_go_on_close(a.handle, callbackPtr(id)))
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
	watchID := C.mbink_go_state_watch(a.handle, nameC, callbackPtr(id))
	if int(watchID) < 0 {
		releaseCallback(id)
		return 0, checkRC(watchID)
	}
	a.watchIDs[int(watchID)] = id
	return int(watchID), nil
}

func (a *App) UnwatchState(watchID int) {
	C.mbink_state_unwatch(a.handle, C.int(watchID))
	if id, ok := a.watchIDs[watchID]; ok {
		releaseCallback(id)
		delete(a.watchIDs, watchID)
	}
}
