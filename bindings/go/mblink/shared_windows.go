//go:build windows

package mblink

/*
#include "mblink.h"
*/
import "C"

func (s *Shared) SetNull(key string) error {
	v, d := cString(key)
	defer d()
	return checkRC(C.mblink_shared_set_null(s.handle, v))
}
func (s *Shared) SetBool(key string, value bool) error {
	v, d := cString(key)
	defer d()
	return checkRC(C.mblink_shared_set_bool(s.handle, v, boolToC(value)))
}
func (s *Shared) SetInt(key string, value int64) error {
	v, d := cString(key)
	defer d()
	return checkRC(C.mblink_shared_set_int(s.handle, v, C.int64_t(value)))
}
func (s *Shared) SetDouble(key string, value float64) error {
	v, d := cString(key)
	defer d()
	return checkRC(C.mblink_shared_set_double(s.handle, v, C.double(value)))
}
func (s *Shared) SetString(key, value string) error {
	kv, kd := cString(key)
	defer kd()
	vv, vd := cString(value)
	defer vd()
	return checkRC(C.mblink_shared_set_string(s.handle, kv, vv))
}
func (s *Shared) SetJSON(key string, value any) error {
	kv, kd := cString(key)
	defer kd()
	jv, jd := cString(mustJSON(value))
	defer jd()
	return checkRC(C.mblink_shared_set_json(s.handle, kv, jv))
}
func (s *Shared) GetBool(key string) bool {
	v, d := cString(key)
	defer d()
	return bool(C.mblink_shared_get_bool(s.handle, v))
}
func (s *Shared) GetInt(key string) int64 {
	v, d := cString(key)
	defer d()
	return int64(C.mblink_shared_get_int(s.handle, v))
}
func (s *Shared) GetDouble(key string) float64 {
	v, d := cString(key)
	defer d()
	return float64(C.mblink_shared_get_double(s.handle, v))
}
func (s *Shared) GetString(key string) string {
	v, d := cString(key)
	defer d()
	return takeOwnedString(C.mblink_shared_get_string(s.handle, v))
}
func (s *Shared) GetJSONText(key string) string {
	v, d := cString(key)
	defer d()
	return takeOwnedString(C.mblink_shared_get_json(s.handle, v))
}
func (s *Shared) GetJSON(key string) any           { return parseJSONText(s.GetJSONText(key)) }
func (s *Shared) Decode(key string, out any) error { return decodeJSONText(s.GetJSONText(key), out) }
func (s *Shared) Type(key string) ValueType {
	v, d := cString(key)
	defer d()
	return ValueType(C.mblink_shared_get_type(s.handle, v))
}
func (s *Shared) Delete(key string) error {
	v, d := cString(key)
	defer d()
	return checkRC(C.mblink_shared_delete(s.handle, v))
}
func (s *Shared) Has(key string) bool {
	v, d := cString(key)
	defer d()
	return bool(C.mblink_shared_has(s.handle, v))
}

func (s *Shared) Batch() *SharedBatch {
	C.mblink_shared_batch_begin(s.handle)
	return &SharedBatch{handle: s.handle}
}

type SharedBatch struct{ handle C.MBlinkSharedHandle }

func (b *SharedBatch) End() {
	if b != nil && b.handle != nil {
		C.mblink_shared_batch_end(b.handle)
		b.handle = nil
	}
}
