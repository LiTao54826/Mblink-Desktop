//go:build windows

package main

import (
	"log"
	"os"
	"path/filepath"

	"mbink-go/mbink"
)

func main() {
	base := filepath.Join(os.TempDir(), "mbink-go-resource-read-example")
	inputDir := filepath.Join(base, "input")
	packageFile := filepath.Join(base, "assets.mbk")
	must(os.RemoveAll(base))
	must(os.MkdirAll(inputDir, 0o755))
	must(os.WriteFile(filepath.Join(inputDir, "message.txt"), []byte("hello from resource package\n"), 0o644))

	must(mbink.CompileResources(inputDir, packageFile, ""))
	file, err := mbink.LoadResourceFile(packageFile, "input/message.txt", "")
	if err != nil {
		log.Fatal(err)
	}

	log.Printf("flags = %d", file.Flags())
	if file.Flags()&mbink.RESOURCE_FLAG_BYTECODE != 0 {
		log.Fatal("unexpected bytecode flag")
	}
	text, err := file.UTF8String()
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("content = %q", text)
}

func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}
