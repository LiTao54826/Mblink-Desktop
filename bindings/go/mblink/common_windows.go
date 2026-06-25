//go:build windows

package mblink

/*
#cgo CFLAGS: -I../../../core/api -I../core/api
#cgo LDFLAGS: -L.. -L../../../build/lib/Release -L../.. -lmblink
#include <stdlib.h>
#include <stdint.h>
#include "mblink.h"

extern char* go_mblink_bind_trampoline(const char* args_json, void* user_data);
extern char* go_mblink_bind_async_trampoline(const char* args_json, void* user_data);
extern void go_mblink_state_watch_trampoline(const char* name, const char* value_json, void* user_data);
extern void go_mblink_resize_trampoline(int width, int height, void* user_data);
extern void go_mblink_void_trampoline(void* user_data);
extern bool go_mblink_bool_trampoline(void* user_data);
extern void go_mblink_update_trampoline(float delta_time, void* user_data);
*/
import "C"

import (
	"encoding/json"
	"errors"
	"fmt"
	"sync"
)

var initOnce sync.Once

func ensureInit() {
	initOnce.Do(func() {
		C.mblink_init()
	})
}

func lastErrorMessage() string {
	ptr := C.mblink_last_error()
	if ptr == nil {
		return "unknown MBlink error"
	}
	return C.GoString(ptr)
}

func checkRC(rc C.int) error {
	if rc == 0 {
		return nil
	}
	return newError(int(rc), lastErrorMessage())
}

func mustJSON(value any) string {
	if value == nil {
		return "null"
	}
	data, err := json.Marshal(value)
	if err != nil {
		fallback, _ := json.Marshal(map[string]string{"error": err.Error()})
		return string(fallback)
	}
	return string(data)
}

func boolToC(value bool) C.bool {
	if value {
		return C.bool(true)
	}
	return C.bool(false)
}

func unsupported() error {
	return errors.New("mblink Go bindings currently support Windows only")
}

func parseJSONText(text string) any {
	var out any
	if err := json.Unmarshal([]byte(text), &out); err != nil {
		return nil
	}
	return out
}

func decodeJSONText(text string, out any) error {
	if err := json.Unmarshal([]byte(text), out); err != nil {
		return fmt.Errorf("decode json: %w", err)
	}
	return nil
}
