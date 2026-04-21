package main

import (
    "fmt"
    "net/http"
)

func main() {
    http.HandleFunc("/health", func(w http.ResponseWriter, _ *http.Request) {
        _, _ = w.Write([]byte("ok"))
    })
    fmt.Println("listening on :8080")
    _ = http.ListenAndServe(":8080", nil)
}
