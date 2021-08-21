package main

import (
	"fmt"
	"olang/pkg/repl"
	"olang/pkg/run"
	"os"
)

func main() {
	if len(os.Args) == 1 {
		restart := true
		for restart {
			restart = repl.Run(os.Stdin, os.Stdout)
		}
	} else {
		file, err := os.Open(os.Args[1])
		if err != nil {
			fmt.Println("Error reading file:", err)
			return
		}
		defer file.Close()
		run.Run(file, os.Stdout, os.Args[1], nil)
	}
}
