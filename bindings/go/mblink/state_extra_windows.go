//go:build windows && cgo

package mblink

/*
#include "mblink.h"
*/
import "C"

func (s *State) ArraySet(name string, index int, value any) error {
	nv, nd := cString(name)
	defer nd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mblink_state_array_set(s.app.handle, nv, C.int(index), jv))
}
func (s *State) ArraySetInt(name string, index int, value int64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mblink_state_array_set_int(s.app.handle, v, C.int(index), C.int64_t(value)))
}
func (s *State) ArraySetDouble(name string, index int, value float64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mblink_state_array_set_double(s.app.handle, v, C.int(index), C.double(value)))
}
func (s *State) ArraySetString(name string, index int, value string) error {
	nv, nd := cString(name)
	defer nd()
	vv, vd := cString(value)
	defer vd()
	return checkRC(C.mblink_state_array_set_string(s.app.handle, nv, C.int(index), vv))
}
func (s *State) ObjectSet(name, key string, value any) error {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mblink_state_object_set(s.app.handle, nv, kv, jv))
}
func (s *State) ObjectSetInt(name, key string, value int64) error {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	return checkRC(C.mblink_state_object_set_int(s.app.handle, nv, kv, C.int64_t(value)))
}
func (s *State) ObjectSetDouble(name, key string, value float64) error {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	return checkRC(C.mblink_state_object_set_double(s.app.handle, nv, kv, C.double(value)))
}
func (s *State) ObjectSetString(name, key, value string) error {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	vv, vd := cString(value)
	defer vd()
	return checkRC(C.mblink_state_object_set_string(s.app.handle, nv, kv, vv))
}
func (s *State) ObjectSetBool(name, key string, value bool) error {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	return checkRC(C.mblink_state_object_set_bool(s.app.handle, nv, kv, boolToC(value)))
}
func (s *State) ObjectRemove(name, key string) error {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	return checkRC(C.mblink_state_object_remove(s.app.handle, nv, kv))
}
func (s *State) ObjectClear(name string) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mblink_state_object_clear(s.app.handle, v))
}
func (s *State) Increment(name string, delta float64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mblink_state_increment(s.app.handle, v, C.double(delta)))
}
func (s *State) Multiply(name string, factor float64) error {
	v, d := cString(name)
	defer d()
	return checkRC(C.mblink_state_multiply(s.app.handle, v, C.double(factor)))
}
func (s *State) StringAppend(name, suffix string) error {
	nv, nd := cString(name)
	defer nd()
	sv, sd := cString(suffix)
	defer sd()
	return checkRC(C.mblink_state_string_append(s.app.handle, nv, sv))
}
func (s *State) StringPrepend(name, prefix string) error {
	nv, nd := cString(name)
	defer nd()
	pv, pd := cString(prefix)
	defer pd()
	return checkRC(C.mblink_state_string_prepend(s.app.handle, nv, pv))
}
