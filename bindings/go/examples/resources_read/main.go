//go:build windows

package main

import (
	"log"
	"os"
	"path/filepath"

	"mblink-go/mblink"
)

func main() {
	base := filepath.Join(os.TempDir(), "mblink-go-resource-read-example")
	inputDir := filepath.Join(base, "input")
	packageFile := filepath.Join(base, "assets.mbk")
	must(os.RemoveAll(base))
	must(os.MkdirAll(inputDir, 0o755))
	must(os.WriteFile(filepath.Join(inputDir, "message.txt"), []byte("hello from resource package\n"), 0o644))

	must(mblink.CompileResources(inputDir, packageFile, ""))
	file, err := mblink.LoadResourceFile(packageFile, "input/message.txt", "")
	if err != nil {
		log.Fatal(err)
	}

	log.Printf("flags = %d", file.Flags())
	if file.Flags()&mblink.RESOURCE_FLAG_BYTECODE != 0 {
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
