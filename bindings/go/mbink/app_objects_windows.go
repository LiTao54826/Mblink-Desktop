//go:build windows

package mbink

/*
#include "mbink.h"
*/
import "C"

import "unsafe"

type State struct{ app *App }

type Shared struct{ handle C.MBinkSharedHandle }

type LogView struct{ handle C.MBinkLogViewHandle }

type Terminal struct{ handle C.MBinkTerminalHandle }

func (a *App) State() *State { return &State{app: a} }

func (a *App) Shared(name string) (*Shared, error) {
	nameC, done := cString(name)
	defer done()
	h := C.mbink_shared_create(a.handle, nameC)
	if h == nil {
		return nil, newError(-1, lastErrorMessage())
	}
	a.sharedHandles = append(a.sharedHandles, h)
	return &Shared{handle: h}, nil
}

func (a *App) LogView(elementID string) (*LogView, error) {
	idC, done := cString(elementID)
	defer done()
	h := C.mbink_logview_get(a.handle, idC)
	if h == nil {
		return nil, newError(-1, lastErrorMessage())
	}
	a.controls = append(a.controls, controlHandle{kind: "logview", handle: unsafe.Pointer(h)})
	return &LogView{handle: h}, nil
}

func (a *App) Terminal(elementID string) (*Terminal, error) {
	idC, done := cString(elementID)
	defer done()
	h := C.mbink_terminal_get(a.handle, idC)
	if h == nil {
		return nil, newError(-1, lastErrorMessage())
	}
	a.controls = append(a.controls, controlHandle{kind: "terminal", handle: unsafe.Pointer(h)})
	return &Terminal{handle: h}, nil
}

func (a *App) DevtoolsOpen() error  { return checkRC(C.mbink_devtools_open(a.handle)) }
func (a *App) DevtoolsClose() error { return checkRC(C.mbink_devtools_close(a.handle)) }

func (l *LogView) Append(level, source, message string) error {
	lv, ld := cString(level)
	defer ld()
	sv, sd := cString(source)
	defer sd()
	mv, md := cString(message)
	defer md()
	return checkRC(C.mbink_logview_append(l.handle, lv, sv, mv))
}
func (l *LogView) Clear() { C.mbink_logview_clear(l.handle) }
func (l *LogView) Export(format string) (string, error) {
	fv, fd := cString(format)
	defer fd()
	ptr := C.mbink_logview_export(l.handle, fv)
	if ptr == nil {
		return "", nil
	}
	defer C.mbink_free(unsafe.Pointer(ptr))
	return C.GoString(ptr), nil
}

func (t *Terminal) Write(data string) error {
	v, d := cString(data)
	defer d()
	return checkRC(C.mbink_terminal_write(t.handle, v))
}
func (t *Terminal) Clear() { C.mbink_terminal_clear(t.handle) }
func (t *Terminal) Execute(command string) error {
	v, d := cString(command)
	defer d()
	return checkRC(C.mbink_terminal_execute(t.handle, v))
}
func (t *Terminal) StartShell(shell string) error {
	v, d := cString(shell)
	defer d()
	return checkRC(C.mbink_terminal_start_shell(t.handle, v))
}
func (t *Terminal) SendInput(input string) error {
	v, d := cString(input)
	defer d()
	return checkRC(C.mbink_terminal_send_input(t.handle, v))
}
func (t *Terminal) Resize(rows, cols int) {
	C.mbink_terminal_resize(t.handle, C.int(rows), C.int(cols))
}
func (t *Terminal) Serialize() (string, error) {
	ptr := C.mbink_terminal_serialize(t.handle)
	if ptr == nil {
		return "", nil
	}
	defer C.mbink_free(unsafe.Pointer(ptr))
	return C.GoString(ptr), nil
}
