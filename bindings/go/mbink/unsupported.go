//go:build !windows

package mbink

type App struct{}

type AppHandle struct{}

type State struct{}

type Shared struct{}

type StateBatch struct{}

type SharedBatch struct{}

type LogView struct{}

type Terminal struct{}

func unsupported() error { return newError(0, "mbink Go bindings currently support Windows only") }

func Version() string                               { return "" }
func DefaultConfig() Config                         { return Config{} }
func DefaultRuntimeOptions() RuntimeOptions         { return RuntimeOptions{} }
func New(string, int, int) (*App, error)            { return nil, unsupported() }
func NewWithConfig(Config) (*App, error)            { return nil, unsupported() }
func CompileResources(string, string, string) error { return unsupported() }
func LoadResourceFile(string, string, string) (*ResourceFile, error) {
	return nil, unsupported()
}
