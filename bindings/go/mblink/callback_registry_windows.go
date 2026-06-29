//go:build windows && cgo

package mblink

/*
#include <stdbool.h>
#include <stdint.h>
#include "mblink.h"
*/
import "C"

import (
	"sync"
	"sync/atomic"
	"unsafe"
)

type bindFunc func(any) (any, error)
type stateWatchFunc func(string, any)
type resizeFunc func(int, int)
type voidFunc func()
type boolFunc func() bool
type updateFunc func(float32)

var callbackSeq atomic.Uint64

var callbackRegistry = struct {
	sync.RWMutex
	binds      map[uintptr]bindFunc
	asyncBinds map[uintptr]bindFunc
	watchers   map[uintptr]stateWatchFunc
	resizes    map[uintptr]resizeFunc
	voids      map[uintptr]voidFunc
	bools      map[uintptr]boolFunc
	updates    map[uintptr]updateFunc
}{
	binds:      map[uintptr]bindFunc{},
	asyncBinds: map[uintptr]bindFunc{},
	watchers:   map[uintptr]stateWatchFunc{},
	resizes:    map[uintptr]resizeFunc{},
	voids:      map[uintptr]voidFunc{},
	bools:      map[uintptr]boolFunc{},
	updates:    map[uintptr]updateFunc{},
}

func nextCallbackID() uintptr {
	return uintptr(callbackSeq.Add(1))
}

func callbackPtr(id uintptr) unsafe.Pointer {
	return unsafe.Pointer(id)
}

func registerBind(fn bindFunc, async bool) uintptr {
	id := nextCallbackID()
	callbackRegistry.Lock()
	defer callbackRegistry.Unlock()
	if async {
		callbackRegistry.asyncBinds[id] = fn
	} else {
		callbackRegistry.binds[id] = fn
	}
	return id
}

func registerWatcher(fn stateWatchFunc) uintptr {
	id := nextCallbackID()
	callbackRegistry.Lock()
	callbackRegistry.watchers[id] = fn
	callbackRegistry.Unlock()
	return id
}

func registerVoid(fn voidFunc) uintptr {
	id := nextCallbackID()
	callbackRegistry.Lock()
	callbackRegistry.voids[id] = fn
	callbackRegistry.Unlock()
	return id
}

func registerBool(fn boolFunc) uintptr {
	id := nextCallbackID()
	callbackRegistry.Lock()
	callbackRegistry.bools[id] = fn
	callbackRegistry.Unlock()
	return id
}

func registerResize(fn resizeFunc) uintptr {
	id := nextCallbackID()
	callbackRegistry.Lock()
	callbackRegistry.resizes[id] = fn
	callbackRegistry.Unlock()
	return id
}

func registerUpdate(fn updateFunc) uintptr {
	id := nextCallbackID()
	callbackRegistry.Lock()
	callbackRegistry.updates[id] = fn
	callbackRegistry.Unlock()
	return id
}
