//go:build windows

package mbink

/*
#include <stdlib.h>
#include "mbink.h"
*/
import "C"

import (
	"unsafe"
)

func takeOwnedString(ptr *C.char) string {
	if ptr == nil {
		return ""
	}
	defer C.mbink_free(unsafe.Pointer(ptr))
	return C.GoString(ptr)
}

func CompileResources(inputPath, outputFile, encryptionKey string) error {
	iv, id := cString(inputPath)
	defer id()
	ov, od := cString(outputFile)
	defer od()
	kv, kd := cString(encryptionKey)
	defer kd()
	return checkRC(C.mbink_compile_resources(iv, ov, kv))
}

func LoadResourceFile(packageFile, resourcePath, encryptionKey string) (*ResourceFile, error) {
	pv, pd := cString(packageFile)
	defer pd()
	rv, rd := cString(resourcePath)
	defer rd()
	kv, kd := cString(encryptionKey)
	defer kd()
	var data unsafe.Pointer
	var size C.size_t
	var flags C.uint32_t
	if err := checkRC(C.mbink_load_resource_file(pv, rv, kv, &data, &size, &flags)); err != nil {
		return nil, err
	}
	var out []byte
	if data != nil && size > 0 {
		out = C.GoBytes(data, C.int(size))
		C.mbink_free(data)
	}
	return &ResourceFile{data: out, flags: uint32(flags)}, nil
}
