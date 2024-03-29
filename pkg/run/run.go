package run

import (
	"fmt"
	"io"
	"olang/pkg/eval"
	"olang/pkg/lexer"
	"olang/pkg/object"
	"olang/pkg/parser"
)

// Run starts the repl to read and run a line at a time
func Run(in io.Reader, out io.Writer, name string, path string, stop <-chan struct{}) {
	env := object.NewEnvironment()

	l := lexer.WithReader(in, name, path)
	p := parser.New(l)

	program := p.ParseProgram()

	if len(program.Statements) == 0 {
		return
	}

	if len(p.Errors()) != 0 {
		printErrors(out, p.Errors())
		return
	}

	eval.OutStream = out
	result := eval.Eval(program, env, stop)

	if result != nil && result.Type() != object.NilType {
		fmt.Fprint(out, result)
	}
}

func printErrors(out io.Writer, errors []parser.Error) {
	for _, err := range errors {
		fmt.Fprintln(out, err.Pos, ":", err.Str)
	}
}
