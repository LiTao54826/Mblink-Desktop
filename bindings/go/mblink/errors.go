package mblink

import "fmt"

type Error struct {
	Code    int
	Message string
}

func (e *Error) Error() string {
	if e == nil {
		return "<nil>"
	}
	if e.Code == 0 {
		return e.Message
	}
	return fmt.Sprintf("MBlink error %d: %s", e.Code, e.Message)
}

func newError(code int, message string) error {
	return &Error{Code: code, Message: message}
}
