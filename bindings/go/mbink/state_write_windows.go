//go:build windows

package mbink

/*
#include "mbink.h"
*/
import "C"

func (s *State) CreateNull(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_create_null(s.app.handle, v))
}
func (s *State) CreateBool(name string, value bool) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_create_bool(s.app.handle, v, boolToC(value)))
}
func (s *State) CreateInt(name string, value int64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_create_int(s.app.handle, v, C.int64_t(value)))
}
func (s *State) CreateDouble(name string, value float64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_create_double(s.app.handle, v, C.double(value)))
}
func (s *State) CreateString(name, value string) error {
	nv, nd := cString(name)
	defer nd()
	vv, vd := cString(value)
	defer vd()
	return checkRC(C.mbink_state_create_string(s.app.handle, nv, vv))
}
func (s *State) CreateArray(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_create_array(s.app.handle, v))
}
func (s *State) CreateObject(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_create_object(s.app.handle, v))
}
func (s *State) CreateJSON(name string, value any) error {
	nv, nd := cString(name)
	defer nd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mbink_state_create_json(s.app.handle, nv, jv))
}
func (s *State) SetNull(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_set_null(s.app.handle, v))
}
func (s *State) SetBool(name string, value bool) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_set_bool(s.app.handle, v, boolToC(value)))
}
func (s *State) SetInt(name string, value int64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_set_int(s.app.handle, v, C.int64_t(value)))
}
func (s *State) SetDouble(name string, value float64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_set_double(s.app.handle, v, C.double(value)))
}
func (s *State) SetString(name, value string) error {
	nv, nd := cString(name)
	defer nd()
	vv, vd := cString(value)
	defer vd()
	return checkRC(C.mbink_state_set_string(s.app.handle, nv, vv))
}
func (s *State) SetJSON(name string, value any) error {
	nv, nd := cString(name)
	defer nd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mbink_state_set_json(s.app.handle, nv, jv))
}
func (s *State) ArrayPush(name string, value any) error {
	nv, nd := cString(name)
	defer nd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mbink_state_array_push(s.app.handle, nv, jv))
}
func (s *State) ArrayPushInt(name string, value int64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_push_int(s.app.handle, v, C.int64_t(value)))
}
func (s *State) ArrayPushDouble(name string, value float64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_push_double(s.app.handle, v, C.double(value)))
}
func (s *State) ArrayPushString(name, value string) error {
	nv, nd := cString(name)
	defer nd()
	vv, vd := cString(value)
	defer vd()
	return checkRC(C.mbink_state_array_push_string(s.app.handle, nv, vv))
}
func (s *State) ArrayPushBool(name string, value bool) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_push_bool(s.app.handle, v, boolToC(value)))
}
func (s *State) ArrayPop(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_pop(s.app.handle, v))
}
func (s *State) ArrayShift(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_shift(s.app.handle, v))
}
func (s *State) ArrayUnshift(name string, value any) error {
	nv, nd := cString(name)
	defer nd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mbink_state_array_unshift(s.app.handle, nv, jv))
}
func (s *State) ArrayRemove(name string, index int) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_remove(s.app.handle, v, C.int(index)))
}
func (s *State) ArrayClear(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mbink_state_array_clear(s.app.handle, v))
}
