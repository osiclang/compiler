package eval

import (
	"bufio"
	"fmt"
	"io"
	"io/ioutil"
	"math/rand"
	"olang/pkg/object"
	"olang/pkg/token"
	"os"
	"path/filepath"
	"strings"
	"time"
)

// OutStream out
var OutStream io.Writer = os.Stdout

var builtins = map[string]*object.Builtin{
	"len":        &object.Builtin{Fn: length},
	"first":      &object.Builtin{Fn: first},
	"last":       &object.Builtin{Fn: last},
	"rest":       &object.Builtin{Fn: rest},
	"lead":       &object.Builtin{Fn: lead},
	"push":       &object.Builtin{Fn: push},
	"pop":        &object.Builtin{Fn: pop},
	"alloc":      &object.Builtin{Fn: alloc},
	"set":        &object.Builtin{Fn: set},
	"join":       &object.Builtin{Fn: join},
	"split":      &object.Builtin{Fn: split},
	"println":    &object.Builtin{Fn: println},
	"print":      &object.Builtin{Fn: print},
	"readln":     &object.Builtin{Fn: readln},
	"read":       &object.Builtin{Fn: read},
	"readc":      &object.Builtin{Fn: readc},
	"readall":    &object.Builtin{Fn: readall},
	"readfile":   &object.Builtin{Fn: readfile},
	"writefile":  &object.Builtin{Fn: writefile},
	"createfile": &object.Builtin{Fn: createfile},
	"readdir":    &object.Builtin{Fn: readDir},
	"args":       &object.Builtin{Fn: args},
	"atoi":       &object.Builtin{Fn: atoi},
	"itoa":       &object.Builtin{Fn: itoa},
	"in":         &object.Builtin{Fn: in},
	"out":        &object.Builtin{Fn: out},
	"rand":       &object.Builtin{Fn: random},
	"sleep":      &object.Builtin{Fn: sleep},
}

func sleep(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch t := args[0].(type) {
	case *object.Integer:
		time.Sleep(time.Duration(t.Value) * time.Millisecond)
		return ConstNil
	default:
		return newError(token.Position{}, "wrong arg types")
	}
}

func random(args ...object.Object) object.Object {
	if len(args) == 0 {
		return &object.Float{Value: rand.Float64()}
	} else if len(args) == 2 {
		switch min := args[0].(type) {
		case *object.Integer:
			switch max := args[1].(type) {
			case *object.Integer:
				return &object.Integer{Value: rand.Int63n(max.Value-min.Value) + min.Value}
			default:
				return newError(token.Position{}, "wrong arg types")
			}
		default:
			return newError(token.Position{}, "wrong arg types")
		}
	} else {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '0 or 2'", len(args))
	}
}

func length(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		return &object.Integer{Value: int64(len(arg.Value))}
	case *object.Array:
		return &object.Integer{Value: int64(len(arg.Elements))}
	default:
		return newError(token.Position{}, "argument to 'len' not supported, got '%s'", args[0].Type())
	}
}

func first(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		if len(arg.Value) > 0 {
			return &object.String{Value: string(arg.Value[0])}
		}
		return ConstNil
	case *object.Array:
		if len(arg.Elements) > 0 {
			return arg.Elements[0]
		}
		return ConstNil
	default:
		return newError(token.Position{}, "argument to 'first' not supported, got '%s'", args[0].Type())
	}
}

func last(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		if len(arg.Value) > 0 {
			return &object.String{Value: string(arg.Value[len(arg.Value)-1])}
		}
		return ConstNil
	case *object.Array:
		if len(arg.Elements) > 0 {
			return arg.Elements[len(arg.Elements)-1]
		}
		return ConstNil
	default:
		return newError(token.Position{}, "argument to 'last' not supported, got '%s'", args[0].Type())
	}
}

func rest(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		l := len(arg.Value)
		if l > 0 {
			newStr := make([]byte, l-1, l-1)
			copy(newStr, arg.Value[1:l])
			return &object.String{Value: string(newStr)}
		}
		return ConstNil
	case *object.Array:
		l := len(arg.Elements)
		if l > 0 {
			newElems := make([]object.Object, l-1, l-1)
			copy(newElems, arg.Elements[1:l])
			return &object.Array{Elements: newElems}
		}
		return ConstNil
	default:
		return newError(token.Position{}, "argument to 'rest' not supported, got '%s'", args[0].Type())
	}
}

func lead(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		l := len(arg.Value)
		if l > 0 {
			newStr := make([]byte, l-1, l-1)
			copy(newStr, arg.Value[:l-1])
			return &object.String{Value: string(newStr)}
		}
		return ConstNil
	case *object.Array:
		l := len(arg.Elements)
		if l > 0 {
			newElems := make([]object.Object, l-1, l-1)
			copy(newElems, arg.Elements[:l-1])
			return &object.Array{Elements: newElems}
		}
		return ConstNil
	default:
		return newError(token.Position{}, "argument to 'lead' not supported, got '%s'", args[0].Type())
	}
}

func push(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '2'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		if p, ok := args[1].(*object.String); ok {
			str := arg.Value + p.Value
			return &object.String{Value: str}
		}
		return newError(token.Position{}, "cannot push '%s' to string", args[1].Type())
	case *object.Array:
		l := len(arg.Elements)
		newElems := make([]object.Object, l+1, l+1)
		copy(newElems, arg.Elements)
		newElems[l] = args[1]
		return &object.Array{Elements: newElems}
	default:
		return newError(token.Position{}, "argument to 'push' not supported, got '%s'", args[0].Type())
	}
}

func pop(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		l := len(arg.Value)
		if l > 0 {
			p := arg.Value[l-1]
			arg.Value = arg.Value[:l-1]
			return &object.String{Value: string(p)}
		}
		return ConstNil
	case *object.Array:
		l := len(arg.Elements)
		if l > 0 {
			p := arg.Elements[l-1]
			arg.Elements = arg.Elements[:l-1]
			return p
		}
		return ConstNil
	default:
		return newError(token.Position{}, "argument to 'pop' not supported, got '%s'", args[0].Type())
	}
}

func alloc(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '2'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.Integer:
		if arg.Value >= 0 {
			newArr := make([]object.Object, arg.Value, arg.Value)
			for i := range newArr {
				newArr[i] = args[1]
			}
			return &object.Array{Elements: newArr}
		}
		return ConstNil
	default:
		return newError(token.Position{}, "argument to 'alloc' not supported, got '%s'", arg.Type())
	}
}

func set(args ...object.Object) object.Object {
	if len(args) != 3 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '3'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.Array:
		if i, ok := args[1].(*object.Integer); ok {
			arg.Elements[i.Value] = args[2]
			return ConstNil
		}
		return newError(token.Position{}, "second argument to 'set' not supported, got '%s'", args[1].Type())
	default:
		return newError(token.Position{}, "argument to 'set' not supported, got '%s'", arg.Type())
	}
}

func join(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '2'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.Array:
		if s, ok := args[1].(*object.String); ok {
			parts := make([]string, len(arg.Elements), len(arg.Elements))
			for i := range arg.Elements {
				parts[i] = arg.Elements[i].String()
			}
			return &object.String{Value: strings.Join(parts, s.Value)}
		}
		return newError(token.Position{}, "second argument to 'join' not supported, got '%s'", args[1].Type())
	default:
		return newError(token.Position{}, "argument to 'join' not supported, got '%s'", arg.Type())
	}
}

func split(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '2'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		if s, ok := args[1].(*object.String); ok {

			parts := strings.Split(arg.Value, s.Value)

			elems := make([]object.Object, len(parts), len(parts))
			for i := range parts {
				elems[i] = &object.String{Value: parts[i]}
			}
			return &object.Array{Elements: elems}
		}
		return newError(token.Position{}, "second argument to 'split' not supported, got '%s'", args[1].Type())
	default:
		return newError(token.Position{}, "argument to 'split' not supported, got '%s'", arg.Type())
	}
}

func println(args ...object.Object) object.Object {
	for _, arg := range args {
		fmt.Fprintln(OutStream, arg)
	}
	return ConstNil
}

func print(args ...object.Object) object.Object {
	for _, arg := range args {
		fmt.Fprintln(OutStream, arg)
	}
	return ConstNil
}

func readln(args ...object.Object) object.Object {
	if len(args) != 0 {
		return newError(token.Position{}, "readln does not take any arguments. given '%d'", len(args))
	}

	scanner := bufio.NewScanner(os.Stdin)
	scanner.Scan()

	return &object.String{Value: scanner.Text()}
}

func read(args ...object.Object) object.Object {
	if len(args) != 0 {
		return newError(token.Position{}, "readln does not take any arguments. given '%d'", len(args))
	}

	s := ""
	fmt.Scan(&s)

	return &object.String{Value: s}
}

func readc(args ...object.Object) object.Object {
	if len(args) != 0 {
		return newError(token.Position{}, "readln does not take any arguments. given '%d'", len(args))
	}

	reader := bufio.NewReader(os.Stdin)
	c, e := reader.ReadByte()
	if e != nil {
		return ConstNil
	}

	return &object.String{Value: string(c)}
}

func readall(args ...object.Object) object.Object {
	if len(args) != 0 {
		return newError(token.Position{}, "readln does not take any arguments. given '%d'", len(args))
	}

	s, _ := ioutil.ReadAll(os.Stdin)

	return &object.String{Value: string(s)}
}

func readfile(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "readfile only takes one argument. given '%d'", len(args))
	}
	s, err := ioutil.ReadFile(args[0].String())
	if err != nil {
		return newError(token.Position{}, "readfile could not read file. given '%s'", args[0])
	}

	return &object.String{Value: string(s)}
}

func writefile(args ...object.Object) object.Object {
	if len(args) > 2 || len(args) < 1 {
		return newError(token.Position{}, "writefile only takes two arguments. given '%d'", len(args))
	}
	s := []byte(args[1].String())
	ioutil.WriteFile(args[0].String(), s, 0644)

	return ConstNil
}

func createfile(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "writefile only takes one argument. given '%d'", len(args))
	}
	_, err := os.Create(args[0].String())
	if err != nil {
		return newError(token.Position{}, "createfile could not create file. given '%s'", args[0])
	}

	return ConstNil
}

func readDir(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "readdir only takes two arguments. given '%d'", len(args))
	}
	switch arg := args[1].(type) {
	case *object.Boolean:
		recursive := arg
		b := []string{}

		if recursive.Value {
			filepath.Walk(args[0].String(),
				func(filePath string, info os.FileInfo, err error) error {
					if err != nil {
						return nil
					}
					if !info.IsDir() {
						file, _ := os.Open(filePath)
						b = append(b, string(file.Name()))
					}
					return nil
				})
		} else {
			file, err := os.Open(args[0].String())
			if err != nil {

			}
			defer file.Close()

			list, _ := file.Readdirnames(0) // 0 to read all files and folders
			for _, name := range list {
				b = append(b, name)
			}
		}
		elems := make([]object.Object, len(b), len(b))
		for i := range b {
			elems[i] = &object.String{Value: b[i]}
		}
		return &object.Array{Elements: elems}
	default:
		return newError(token.Position{}, "argument 2 must be bool for recursive readdir, got '%s'", args[1].Type())
	}

}

func args(args ...object.Object) object.Object {
	if len(args) != 0 {
		return newError(token.Position{}, "args() take no arguments. given '%d'", len(args))
	}
	elems := make([]object.Object, len(os.Args), len(os.Args))
	for i := range os.Args {
		elems[i] = &object.String{Value: string(os.Args[i])}
	}
	return &object.Array{Elements: elems}
}

func in(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "in takes one arguments. given '%d'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		f, err := os.Open(arg.Value)
		if err != nil {
			return newError(token.Position{}, err.Error())
		}
		defer f.Close()
		s, _ := ioutil.ReadAll(f)
		return &object.String{Value: string(s)}
	default:
		return newError(token.Position{}, "argument to 'in' not supported, got '%s'", args[0].Type())
	}
}

func out(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "out takes one arguments. given '%d'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		f, err := os.Create(arg.Value)
		if err != nil {
			return newError(token.Position{}, err.Error())
		}
		defer f.Close()
		switch str := args[1].(type) {
		case *object.String:
			f.WriteString(str.Value)
			return nil
		default:
			return newError(token.Position{}, "argument to 'out' not supported, got '%s'", args[0].Type())
		}
	default:
		return newError(token.Position{}, "argument to 'out' not supported, got '%s'", args[0].Type())
	}
}

func atoi(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		l := len(arg.Value)
		if l == 1 {
			return &object.Integer{Value: int64(arg.Value[0])}
		}
		return newError(token.Position{}, "argument to 'atoi must be string with length of 1. Got '%d'", l)
	default:
		return newError(token.Position{}, "argument to 'atoi' not supported, got '%s'", args[0].Type())
	}
}

func itoa(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '1'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.Integer:
		if arg.Value >= 0 && arg.Value < 256 {
			return &object.String{Value: string(byte(arg.Value))}
		}
		return newError(token.Position{}, "argument to 'atoi must be between 0 and 256 Got '%d'", arg.Value)
	default:
		return newError(token.Position{}, "argument to 'atoi' not supported, got '%s'", args[0].Type())
	}
}
