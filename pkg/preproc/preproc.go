package preproc

import (
	"errors"
	"fmt"
	"io/ioutil"
	"path/filepath"
	"strings"
)

var imports []Fl
var err error
var path string

type Fl struct {
	I string
	B []byte
}

func GetDeps(b []byte, p string) ([]byte, error) {
	path = p
	scanForImport(b)
	return assembleDeps(), err
}

func scanForImport(b []byte) {
	s := string(b)
	t := strings.Fields(s)
	for i, token := range t {
		if token == "//_include" {
			alDeps(t[i+1])
		}
	}
}

func alDeps(n string) {
	for i, tokens := range imports {
		if tokens.I == n {
			imports = append(imports[:i], imports[i+1:]...)
		}
	}
	token := Fl{}
	token.I = n
	token.B = readDep(n)
	imports = append([]Fl{token}, imports...)
	scanForImport(token.B)
}

func readDep(n string) []byte {
	data, loadErr := ioutil.ReadFile(filepath.Join(path, n))
	if loadErr != nil {
		err = errors.New(fmt.Sprint("Cannot find ", n))
	}
	return data
}

func assembleDeps() []byte {
	var assemble []byte
	for _, d := range imports {
		assemble = append(assemble, d.B...)
	}
	return assemble
}
