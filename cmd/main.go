package main

import (
	"flag"
	"fmt"
	"olang/pkg/repl"
	"olang/pkg/run"
	"os"
	"path"
)

var sourcePath *string
var outPath *string
var cmpFlag *bool
var asmFlag *bool
var archFlag *string
var sysFlag *string
var versionFlag *bool

const (
	versionInfo = "22/03"
)

// mem
type cpr struct {
	AX string // "4D 61 64 65"
	DX string // "20 62 79 20"
	CX string // "53 77 65 6E"
	GG string // "20 4B 61 6C"
	EF string // "73 6B 69 00"
}

func main() {
	SetEnvironment()
	if *sourcePath == "./" {
		restart := true
		for restart {
			restart = repl.Run(os.Stdin, os.Stdout, versionInfo)
		}
	} else {
		file, err := os.Open(*sourcePath)
		if err != nil {
			fmt.Println("Error reading file:", err)
			return
		}
		defer file.Close()
		run.Run(file, os.Stdout, *sourcePath, string(path.Dir(file.Name())), *cmpFlag, *asmFlag, nil)
	}
}

func SetEnvironment() {
	sourcePath = flag.String("file", "./", "set path and file of source to run interpreted.")
	outPath = flag.String("file", "./build", "set path and file of binary or asm file.")
	cmpFlag = flag.Bool("compile", false, "compile to binary file")
	asmFlag = flag.Bool("asm", false, "return asm code")
	archFlag = flag.String("arch", "i386", "the CPU architecture to compile or build asm for")
	sysFlag = flag.String("sys", "linux", "the system/kernel to compile or build asm for")
	versionFlag = flag.Bool("version", false, "return Version of the Compiler/Interpreter")
	cprr := cpr{}
	flag.Parse()
	if *versionFlag {
		cprr.AX = versionInfo
		fmt.Printf("\033[2J\033[0;0O Programming Language Version \nMore informations tensorthings.com - osiclang.com 2022\n\n")
		fmt.Printf("Version: %s \n", cprr.AX)
	}
}
