//go:build !windows || !cgo

package mblink

type App struct{}

type AppHandle struct{}

type State struct{}

type Shared struct{}

type StateBatch struct{}

type SharedBatch struct{}

type LogView struct{}

type Terminal struct{}

func unsupported() error {
	return newError(0, "mblink Go bindings require Windows with CGO_ENABLED=1 and a C compiler")
}

func Version() string                               { return "" }
func DefaultConfig() Config                         { return Config{} }
func DefaultRuntimeOptions() RuntimeOptions         { return RuntimeOptions{} }
func New(string, int, int) (*App, error)            { return nil, unsupported() }
func NewWithConfig(Config) (*App, error)            { return nil, unsupported() }
func CompileResources(string, string, string) error { return unsupported() }
func LoadResourceFile(string, string, string) (*ResourceFile, error) {
	return nil, unsupported()
}
func (a *App) EnableDevtools() error { return unsupported() }
func (a *App) EnableDevtoolsHttp(DevToolsHttpOptions) (*DevToolsHttpSession, error) {
	return nil, unsupported()
}
func (a *App) DevtoolsHttpSession(DevToolsHttpOptions) (*DevToolsHttpSession, error) {
	return nil, unsupported()
}
func (a *App) DevtoolsHttpStop() error { return unsupported() }
func (s *DevToolsHttpSession) Request(string, any) (map[string]any, error) {
	return nil, unsupported()
}
func (s *DevToolsHttpSession) Stop() error { return unsupported() }
