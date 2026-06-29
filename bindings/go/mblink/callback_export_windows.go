//go:build windows && cgo

package mblink

/*
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "mblink.h"
*/
import "C"

import "unsafe"

func releaseCallback(id uintptr) {
	callbackRegistry.Lock()
	delete(callbackRegistry.binds, id)
	delete(callbackRegistry.asyncBinds, id)
	delete(callbackRegistry.watchers, id)
	delete(callbackRegistry.resizes, id)
	delete(callbackRegistry.voids, id)
	delete(callbackRegistry.bools, id)
	delete(callbackRegistry.updates, id)
	callbackRegistry.Unlock()
}

func callbackJSONResult(value any, err error) *C.char {
	payload := value
	if err != nil {
		payload = map[string]string{"error": err.Error()}
	}
	text := mustJSON(payload)
	ctext := C.CString(text)
	defer C.free(unsafe.Pointer(ctext))
	return C.mblink_copy_string(ctext)
}

//export go_mblink_bind_trampoline
func go_mblink_bind_trampoline(argsJSON *C.char, userData unsafe.Pointer) *C.char {
	id := uintptr(userData)
	callbackRegistry.RLock()
	fn := callbackRegistry.binds[id]
	callbackRegistry.RUnlock()
	if fn == nil {
		return callbackJSONResult(map[string]string{"error": "missing callback"}, nil)
	}
	return callbackJSONResult(fn(parseJSONText(C.GoString(argsJSON))))
}

//export go_mblink_bind_async_trampoline
func go_mblink_bind_async_trampoline(argsJSON *C.char, userData unsafe.Pointer) *C.char {
	id := uintptr(userData)
	callbackRegistry.RLock()
	fn := callbackRegistry.asyncBinds[id]
	callbackRegistry.RUnlock()
	if fn == nil {
		return callbackJSONResult(map[string]string{"error": "missing async callback"}, nil)
	}
	return callbackJSONResult(fn(parseJSONText(C.GoString(argsJSON))))
}

//export go_mblink_state_watch_trampoline
func go_mblink_state_watch_trampoline(name *C.char, valueJSON *C.char, userData unsafe.Pointer) {
	callbackRegistry.RLock()
	fn := callbackRegistry.watchers[uintptr(userData)]
	callbackRegistry.RUnlock()
	if fn != nil {
		fn(C.GoString(name), parseJSONText(C.GoString(valueJSON)))
	}
}

//export go_mblink_resize_trampoline
func go_mblink_resize_trampoline(width C.int, height C.int, userData unsafe.Pointer) {
	callbackRegistry.RLock()
	fn := callbackRegistry.resizes[uintptr(userData)]
	callbackRegistry.RUnlock()
	if fn != nil {
		fn(int(width), int(height))
	}
}

//export go_mblink_void_trampoline
func go_mblink_void_trampoline(userData unsafe.Pointer) {
	callbackRegistry.RLock()
	fn := callbackRegistry.voids[uintptr(userData)]
	callbackRegistry.RUnlock()
	if fn != nil {
		fn()
	}
}

//export go_mblink_bool_trampoline
func go_mblink_bool_trampoline(userData unsafe.Pointer) C.bool {
	callbackRegistry.RLock()
	fn := callbackRegistry.bools[uintptr(userData)]
	callbackRegistry.RUnlock()
	if fn != nil && fn() {
		return C.bool(true)
	}
	return C.bool(false)
}

//export go_mblink_update_trampoline
func go_mblink_update_trampoline(deltaTime C.float, userData unsafe.Pointer) {
	callbackRegistry.RLock()
	fn := callbackRegistry.updates[uintptr(userData)]
	callbackRegistry.RUnlock()
	if fn != nil {
		fn(float32(deltaTime))
	}
}
