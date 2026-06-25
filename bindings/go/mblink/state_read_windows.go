//go:build windows

package mblink

/*
#include "mblink.h"
*/
import "C"

func (s *State) Exists(name string) bool {
	v, d := cString(name)
	defer d()
	return bool(C.mblink_state_exists(s.app.handle, v))
}
func (s *State) Type(name string) ValueType {
	v, d := cString(name)
	defer d()
	return ValueType(C.mblink_state_type(s.app.handle, v))
}
func (s *State) Delete(name string) {
	v, d := cString(name)
	defer d()
	C.mblink_state_delete(s.app.handle, v)
}
func (s *State) GetBool(name string) bool {
	v, d := cString(name)
	defer d()
	return bool(C.mblink_state_get_bool(s.app.handle, v))
}
func (s *State) GetInt(name string) int64 {
	v, d := cString(name)
	defer d()
	return int64(C.mblink_state_get_int(s.app.handle, v))
}
func (s *State) GetDouble(name string) float64 {
	v, d := cString(name)
	defer d()
	return float64(C.mblink_state_get_double(s.app.handle, v))
}
func (s *State) GetString(name string) string {
	v, d := cString(name)
	defer d()
	return C.GoString(C.mblink_state_get_string(s.app.handle, v))
}
func (s *State) GetLength(name string) int {
	v, d := cString(name)
	defer d()
	return int(C.mblink_state_get_length(s.app.handle, v))
}
func (s *State) GetJSONText(name string) string {
	v, d := cString(name)
	defer d()
	return takeOwnedString(C.mblink_state_get_json(s.app.handle, v))
}
func (s *State) GetJSON(name string) any           { return parseJSONText(s.GetJSONText(name)) }
func (s *State) Decode(name string, out any) error { return decodeJSONText(s.GetJSONText(name), out) }
func (s *State) GetAtText(name string, index int) string {
	v, d := cString(name)
	defer d()
	return takeOwnedString(C.mblink_state_get_at(s.app.handle, v, C.int(index)))
}
func (s *State) GetAt(name string, index int) any { return parseJSONText(s.GetAtText(name, index)) }
func (s *State) DecodeAt(name string, index int, out any) error {
	return decodeJSONText(s.GetAtText(name, index), out)
}
func (s *State) GetKeyText(name, key string) string {
	nv, nd := cString(name)
	defer nd()
	kv, kd := cString(key)
	defer kd()
	return takeOwnedString(C.mblink_state_get_key(s.app.handle, nv, kv))
}
func (s *State) GetKey(name, key string) any { return parseJSONText(s.GetKeyText(name, key)) }
func (s *State) DecodeKey(name, key string, out any) error {
	return decodeJSONText(s.GetKeyText(name, key), out)
}
func (s *State) Value(name string) any { return s.GetJSON(name) }

func (s *State) ProcessQueue() error { return checkRC(C.mblink_process_queue(s.app.handle)) }
func (s *State) QueueSize() int      { return int(C.mblink_queue_size(s.app.handle)) }
func (s *State) SetMergeMode(enable bool) {
	C.mblink_state_set_merge_mode(s.app.handle, boolToC(enable))
}

func (s *State) Batch() *StateBatch {
	C.mblink_state_batch_begin(s.app.handle)
	return &StateBatch{handle: s.app.handle}
}

type StateBatch struct{ handle C.MBlinkHandle }

func (b *StateBatch) End() {
	if b != nil && b.handle != nil {
		C.mblink_state_batch_end(b.handle)
		b.handle = nil
	}
}
