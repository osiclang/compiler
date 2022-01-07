package i386

import (
	"olang/pkg/asm/objects"
	"olang/pkg/asm/objectsmap"
	"strconv"
)

func addPrintBs(opsSgmnt *objectsmap.OpsSegments) {
	if checkPersistence(objects.PRINT_CLI, *opsSgmnt) {
		return
	}
	newSgmnt := objectsmap.OpsSegment{}
	newSgmnt.Segment = objects.PRINT_CLI
	newSgmnt.Vars = []string{"2"}
	appendOp(&newSgmnt, objects.MOV, []string{objects.EAX, objects.SYS_WRITE})
	appendOp(&newSgmnt, objects.MOV, []string{objects.EBX, objects.STDOUT})
	appendOp(&newSgmnt, objects.MOV, []string{objects.ECX, macrodVarName(1)})
	appendOp(&newSgmnt, objects.MOV, []string{objects.EDX, macrodVarName(2)})
	opsSgmnt.OpsSgmnt = append([]objectsmap.OpsSegment{newSgmnt}, opsSgmnt.OpsSgmnt...)
}

func addCliInputBs(opsSgmnt *objectsmap.OpsSegments) {
	if checkPersistence(objects.INPUT_CLI, *opsSgmnt) {
		return
	}
	newSgmnt := objectsmap.OpsSegment{}
	newSgmnt.Segment = objects.INPUT_CLI
	newSgmnt.Vars = []string{"2"}
	appendOp(&newSgmnt, objects.MOV, []string{objects.EAX, objects.SYS_READ})
	appendOp(&newSgmnt, objects.MOV, []string{objects.EBX, objects.STDIN})
	appendOp(&newSgmnt, objects.MOV, []string{objects.ECX, macrodVarName(1)})
	appendOp(&newSgmnt, objects.MOV, []string{objects.EDX, macrodVarName(2)})
	opsSgmnt.OpsSgmnt = append([]objectsmap.OpsSegment{newSgmnt}, opsSgmnt.OpsSgmnt...)
}

func addMulBs(opsSgmnt *objectsmap.OpsSegments) {
	if checkPersistence(objects.MUL_INT, *opsSgmnt) {
		return
	}
	newSgmnt := objectsmap.OpsSegment{}
	newSgmnt.Segment = objects.MUL_INT
	newSgmnt.Vars = []string{"3"}
	appendOp(&newSgmnt, objects.MOV, []string{objects.AL, macrodVarName(1)})
	appendOp(&newSgmnt, objects.SUB, []string{objects.AL, exclaimString("0")})
	appendOp(&newSgmnt, objects.MOV, []string{objects.BL, macrodVarName(2)})
	appendOp(&newSgmnt, objects.SUB, []string{objects.BL, exclaimString("0")})
	appendOp(&newSgmnt, objects.MUL, []string{objects.BL})
	appendOp(&newSgmnt, objects.ADD, []string{objects.AL, exclaimString("0")})
	appendOp(&newSgmnt, objects.MOV, []string{pointVar(exclaimString("0")), objects.AL})
	opsSgmnt.OpsSgmnt = append([]objectsmap.OpsSegment{newSgmnt}, opsSgmnt.OpsSgmnt...)
}

func addDivBs(opsSgmnt *objectsmap.OpsSegments) {
	if checkPersistence(objects.DIV_INT, *opsSgmnt) {
		return
	}
	newSgmnt := objectsmap.OpsSegment{}
	newSgmnt.Segment = objects.DIV_INT
	newSgmnt.Vars = []string{"3"}
	appendOp(&newSgmnt, objects.MOV, []string{objects.AX, macrodVarName(1)})
	appendOp(&newSgmnt, objects.SUB, []string{objects.AX, exclaimString("0")})
	appendOp(&newSgmnt, objects.MOV, []string{objects.BL, macrodVarName(2)})
	appendOp(&newSgmnt, objects.SUB, []string{objects.BL, exclaimString("0")})
	appendOp(&newSgmnt, objects.DIV, []string{objects.BL})
	appendOp(&newSgmnt, objects.ADD, []string{objects.AX, exclaimString("0")})
	appendOp(&newSgmnt, objects.MOV, []string{pointVar(exclaimString("0")), objects.AX})
	opsSgmnt.OpsSgmnt = append([]objectsmap.OpsSegment{newSgmnt}, opsSgmnt.OpsSgmnt...)
}

func checkPersistence(name string, opsSgmnt objectsmap.OpsSegments) bool {
	for _, opSeq := range opsSgmnt.OpsSgmnt {
		if opSeq.Segment == name {
			return true
		}
	}
	return false
}

func appendOp(opsSeg *objectsmap.OpsSegment, code string, values []string) {
	opsSeg.Ops = append(opsSeg.Ops, objectsmap.Op{
		OpCode: code,
		Vars:   values,
	})
}

func appendOpTSeg(opsSgmnt *objectsmap.OpsSegments, name string, code string, values []string) {
	for i, opSeq := range opsSgmnt.OpsSgmnt {
		if opSeq.Segment == name {
			appendOp(&opsSgmnt.OpsSgmnt[i], code, values)
		}
	}
}

func macrodVarName(num int) string {
	return string("%" + strconv.Itoa(num))
}

func exclaimString(str string) string {
	return "'" + str + "'"
}

func pointVar(str string) string {
	return "[" + str + "]"
}
