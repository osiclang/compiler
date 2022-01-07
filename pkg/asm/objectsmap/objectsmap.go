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
