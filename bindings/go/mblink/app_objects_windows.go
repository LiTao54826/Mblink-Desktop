//go:build windows

package mblink

/*
#include <stdlib.h>
#include <stdbool.h>
#include "mblink.h"

typedef struct {
    const char* bind_host;
    unsigned short port;
    const char* auth_token;
    bool require_auth;
} MBlinkDevToolsHttpOptions;

typedef struct {
    unsigned short port;
    char* url;
    char* auth_token;
} MBlinkDevToolsHttpInfo;

extern int go_mblink_devtools_open_call(MBlinkHandle h);
extern int go_mblink_devtools_close_call(MBlinkHandle h);
extern MBlinkDevToolsHttpOptions go_mblink_devtools_default_http_options_call(void);
extern int go_mblink_devtools_http_start_call(MBlinkHandle h, const MBlinkDevToolsHttpOptions* o, MBlinkDevToolsHttpInfo* i);
extern int go_mblink_devtools_http_stop_call(MBlinkHandle h);
extern void go_mblink_devtools_http_info_free_call(MBlinkDevToolsHttpInfo* i);
*/
import "C"

import (
	"bytes"
	"encoding/json"
	"net/http"
	"sync/atomic"
	"time"
	"unsafe"
)

type State struct{ app *App }

type Shared struct{ handle C.MBlinkSharedHandle }

type LogView struct{ handle C.MBlinkLogViewHandle }

type Terminal struct{ handle C.MBlinkTerminalHandle }

func (a *App) State() *State { return &State{app: a} }

func (a *App) Shared(name string) (*Shared, error) {
	nameC, done := cString(name)
	defer done()
	h := C.mblink_shared_create(a.handle, nameC)
	if h == nil {
		return nil, newError(-1, lastErrorMessage())
	}
	a.sharedHandles = append(a.sharedHandles, h)
	return &Shared{handle: h}, nil
}

func (a *App) LogView(elementID string) (*LogView, error) {
	idC, done := cString(elementID)
	defer done()
	h := C.mblink_logview_get(a.handle, idC)
	if h == nil {
		return nil, newError(-1, lastErrorMessage())
	}
	a.controls = append(a.controls, controlHandle{kind: "logview", handle: unsafe.Pointer(h)})
	return &LogView{handle: h}, nil
}

func (a *App) Terminal(elementID string) (*Terminal, error) {
	idC, done := cString(elementID)
	defer done()
	h := C.mblink_terminal_get(a.handle, idC)
	if h == nil {
		return nil, newError(-1, lastErrorMessage())
	}
	a.controls = append(a.controls, controlHandle{kind: "terminal", handle: unsafe.Pointer(h)})
	return &Terminal{handle: h}, nil
}

func (a *App) DevtoolsOpen() error {
	if err := ensureDevtoolsRuntime(); err != nil {
		return err
	}
	return checkRC(C.go_mblink_devtools_open_call(a.handle))
}

func (a *App) DevtoolsClose() error {
	if err := ensureDevtoolsRuntime(); err != nil {
		return err
	}
	return checkRC(C.go_mblink_devtools_close_call(a.handle))
}

func (a *App) EnableDevtools() error { return a.DevtoolsOpen() }

func (a *App) EnableDevtoolsHttp(options DevToolsHttpOptions) (*DevToolsHttpSession, error) {
	return a.DevtoolsHttpSession(options)
}

func (a *App) DevtoolsHttpSession(options DevToolsHttpOptions) (*DevToolsHttpSession, error) {
	if err := ensureDevtoolsRuntime(); err != nil {
		return nil, err
	}
	raw := C.go_mblink_devtools_default_http_options_call()
	var bindHost *C.char
	if options.BindHost != "" {
		bindHost, _ = cString(options.BindHost)
		defer C.free(unsafe.Pointer(bindHost))
		raw.bind_host = bindHost
	}
	if options.Port != 0 {
		raw.port = C.ushort(options.Port)
	}
	var authToken *C.char
	if options.AuthToken != "" {
		authToken, _ = cString(options.AuthToken)
		defer C.free(unsafe.Pointer(authToken))
		raw.auth_token = authToken
	}
	raw.require_auth = boolToC(!options.NoAuth)

	var info C.MBlinkDevToolsHttpInfo
	if err := checkRC(C.go_mblink_devtools_http_start_call(a.handle, &raw, &info)); err != nil {
		return nil, err
	}
	defer C.go_mblink_devtools_http_info_free_call(&info)
	return &DevToolsHttpSession{
		URL:         C.GoString(info.url),
		Port:        uint16(info.port),
		AuthToken:   C.GoString(info.auth_token),
		RequireAuth: !options.NoAuth,
		app:         a,
		stop:        a.DevtoolsHttpStop,
	}, nil
}

func (a *App) DevtoolsHttpStop() error {
	if err := ensureDevtoolsRuntime(); err != nil {
		return err
	}
	return checkRC(C.go_mblink_devtools_http_stop_call(a.handle))
}

func (s *DevToolsHttpSession) Request(method string, params any) (map[string]any, error) {
	id := atomic.AddInt64(&s.nextID, 1)
	payload := map[string]any{
		"jsonrpc": "2.0",
		"id":      id,
		"method":  method,
	}
	if params != nil {
		payload["params"] = params
	}
	data, err := json.Marshal(payload)
	if err != nil {
		return nil, err
	}
	req, err := http.NewRequest(http.MethodPost, s.URL, bytes.NewReader(data))
	if err != nil {
		return nil, err
	}
	req.Header.Set("Content-Type", "application/json")
	if s.AuthToken != "" {
		req.Header.Set("X-MBLINK-DevTools-Token", s.AuthToken)
	}

	type requestResult struct {
		value map[string]any
		err   error
	}

	done := make(chan requestResult, 1)
	go func() {
		resp, err := http.DefaultClient.Do(req)
		if err != nil {
			done <- requestResult{err: err}
			return
		}
		defer resp.Body.Close()
		var out map[string]any
		if err := json.NewDecoder(resp.Body).Decode(&out); err != nil {
			done <- requestResult{err: err}
			return
		}
		done <- requestResult{value: out}
	}()

	for {
		select {
		case result := <-done:
			return result.value, result.err
		default:
			if s.app != nil {
				_, _ = s.app.Poll()
			}
			time.Sleep(2 * time.Millisecond)
		}
	}
}

func (s *DevToolsHttpSession) Stop() error {
	if s == nil || s.stop == nil {
		return nil
	}
	return s.stop()
}

func (l *LogView) Append(level, source, message string) error {
	lv, ld := cString(level)
	defer ld()
	sv, sd := cString(source)
	defer sd()
	mv, md := cString(message)
	defer md()
	return checkRC(C.mblink_logview_append(l.handle, lv, sv, mv))
}
func (l *LogView) Clear() { C.mblink_logview_clear(l.handle) }
func (l *LogView) Export(format string) (string, error) {
	fv, fd := cString(format)
	defer fd()
	ptr := C.mblink_logview_export(l.handle, fv)
	if ptr == nil {
		return "", nil
	}
	defer C.mblink_free(unsafe.Pointer(ptr))
	return C.GoString(ptr), nil
}

func (t *Terminal) Write(data string) error {
	v, d := cString(data)
	defer d()
	return checkRC(C.mblink_terminal_write(t.handle, v))
}
func (t *Terminal) Clear() { C.mblink_terminal_clear(t.handle) }
func (t *Terminal) Execute(command string) error {
	v, d := cString(command)
	defer d()
	return checkRC(C.mblink_terminal_execute(t.handle, v))
}
func (t *Terminal) StartShell(shell string) error {
	v, d := cString(shell)
	defer d()
	return checkRC(C.mblink_terminal_start_shell(t.handle, v))
}
func (t *Terminal) SendInput(input string) error {
	v, d := cString(input)
	defer d()
	return checkRC(C.mblink_terminal_send_input(t.handle, v))
}
func (t *Terminal) Resize(rows, cols int) {
	C.mblink_terminal_resize(t.handle, C.int(rows), C.int(cols))
}
func (t *Terminal) Serialize() (string, error) {
	ptr := C.mblink_terminal_serialize(t.handle)
	if ptr == nil {
		return "", nil
	}
	defer C.mblink_free(unsafe.Pointer(ptr))
	return C.GoString(ptr), nil
}
