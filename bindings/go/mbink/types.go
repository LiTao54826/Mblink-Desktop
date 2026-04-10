package mbink

type ValueType int

const (
	TypeNull ValueType = iota
	TypeBool
	TypeInt
	TypeDouble
	TypeString
	TypeArray
	TypeObject
)

const RESOURCE_FLAG_BYTECODE uint32 = 1

type Config struct {
	Title             string
	Width             int
	Height            int
	Headless          bool
	Borderless        bool
	Transparent       bool
	AlwaysOnTop       bool
	Resizable         bool
	GPU               bool
	Fullscreen        bool
	ResizeBorderWidth int
	MinWidth          int
	MinHeight         int
	MaxWidth          int
	MaxHeight         int
}

type ResourceFile struct {
	data  []byte
	flags uint32
}

func (r *ResourceFile) Data() []byte {
	if r == nil {
		return nil
	}
	return append([]byte(nil), r.data...)
}

func (r *ResourceFile) Flags() uint32 {
	if r == nil {
		return 0
	}
	return r.flags
}

func (r *ResourceFile) IsBytecode() bool {
	return r != nil && r.flags&RESOURCE_FLAG_BYTECODE != 0
}

func (r *ResourceFile) UTF8String() (string, error) {
	if r == nil {
		return "", nil
	}
	return string(r.data), nil
}
