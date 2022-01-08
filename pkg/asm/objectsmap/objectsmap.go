package objectsmap

type OpsSegments struct {
	OpsSgmnt []OpsSegment
}

type OpsSegment struct {
	Segment string
	Ops     []Op
	Vars    []string
}

type Op struct {
	OpCode string
	Vars   []string
}

type Cpr struct {
	AX string // "4D 61 64 65"
	DX string // "20 62 79 20"
	CX string // "53 77 65 6E"
	GG string // "20 4B 61 6C"
	EF string // "73 6B 69 00"
}
